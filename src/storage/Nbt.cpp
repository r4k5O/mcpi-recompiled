#include "storage/Nbt.hpp"

#include <bit>
#include <cstddef>
#include <limits>
#include <ostream>
#include <istream>
#include <type_traits>
#include <utility>

namespace mcpi::storage::nbt {
namespace {

struct Budget {
    const Limits& limits;
    std::size_t elements = 0U;

    bool charge(std::size_t count) noexcept {
        if (count > limits.max_container_elements -
                        std::min(elements, limits.max_container_elements)) {
            return false;
        }
        elements += count;
        return true;
    }
};

bool valid_type(std::uint8_t raw) noexcept {
    return raw <= static_cast<std::uint8_t>(Type::LongArray);
}

template <typename UInt>
bool write_unsigned(std::ostream& output, UInt value, Endian endian) {
    static_assert(std::is_unsigned_v<UInt>);
    for (std::size_t index = 0; index < sizeof(UInt); ++index) {
        const std::size_t byte_index =
            endian == Endian::Little ? index : sizeof(UInt) - 1U - index;
        const auto byte = static_cast<unsigned char>(
            (value >> (byte_index * 8U)) & static_cast<UInt>(0xffU));
        output.put(static_cast<char>(byte));
        if (!output) {
            return false;
        }
    }
    return true;
}

template <typename UInt>
bool read_unsigned(std::istream& input, UInt& value, Endian endian) {
    static_assert(std::is_unsigned_v<UInt>);
    value = 0;
    for (std::size_t index = 0; index < sizeof(UInt); ++index) {
        char raw = 0;
        if (!input.get(raw)) {
            return false;
        }
        const std::size_t byte_index =
            endian == Endian::Little ? index : sizeof(UInt) - 1U - index;
        value |= static_cast<UInt>(static_cast<unsigned char>(raw))
                 << (byte_index * 8U);
    }
    return true;
}

bool write_string(std::ostream& output, std::string_view value, Endian endian) {
    if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }
    if (!write_unsigned(output, static_cast<std::uint16_t>(value.size()), endian)) {
        return false;
    }
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
    return static_cast<bool>(output);
}

bool read_string(std::istream& input, std::string& value, Endian endian) {
    std::uint16_t length = 0;
    if (!read_unsigned(input, length, endian)) {
        return false;
    }
    value.assign(length, '\0');
    if (length == 0U) {
        return true;
    }
    input.read(value.data(), static_cast<std::streamsize>(length));
    return input.gcount() == static_cast<std::streamsize>(length);
}

bool write_payload(
    std::ostream& output,
    const Tag& tag,
    Endian endian,
    const Limits& limits,
    std::size_t depth);

bool write_named_impl(
    std::ostream& output,
    std::string_view name,
    const Tag& tag,
    Endian endian,
    const Limits& limits,
    std::size_t depth) {
    if (tag.type == Type::End || depth > limits.max_depth) {
        return false;
    }
    output.put(static_cast<char>(static_cast<std::uint8_t>(tag.type)));
    return static_cast<bool>(output) &&
           write_string(output, name, endian) &&
           write_payload(output, tag, endian, limits, depth);
}

bool write_payload(
    std::ostream& output,
    const Tag& tag,
    Endian endian,
    const Limits& limits,
    std::size_t depth) {
    if (depth > limits.max_depth) {
        return false;
    }

    switch (tag.type) {
    case Type::End:
        return false;
    case Type::Byte:
        output.put(static_cast<char>(static_cast<std::uint8_t>(tag.byte_value)));
        return static_cast<bool>(output);
    case Type::Short:
        return write_unsigned(
            output, static_cast<std::uint16_t>(tag.short_value), endian);
    case Type::Int:
        return write_unsigned(
            output, static_cast<std::uint32_t>(tag.int_value), endian);
    case Type::Long:
        return write_unsigned(
            output, static_cast<std::uint64_t>(tag.long_value), endian);
    case Type::Float:
        return write_unsigned(
            output, std::bit_cast<std::uint32_t>(tag.float_value), endian);
    case Type::Double:
        return write_unsigned(
            output, std::bit_cast<std::uint64_t>(tag.double_value), endian);
    case Type::ByteArray: {
        if (tag.byte_array_value.size() > limits.max_array_bytes ||
            tag.byte_array_value.size() >
                static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        if (!write_unsigned(
                output,
                static_cast<std::uint32_t>(tag.byte_array_value.size()),
                endian)) {
            return false;
        }
        for (const auto value : tag.byte_array_value) {
            output.put(static_cast<char>(static_cast<std::uint8_t>(value)));
            if (!output) {
                return false;
            }
        }
        return true;
    }
    case Type::String:
        return write_string(output, tag.string_value, endian);
    case Type::List: {
        if (tag.list_value.size() > limits.max_container_elements ||
            tag.list_value.size() >
                static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        if (!tag.list_value.empty() && tag.list_type == Type::End) {
            return false;
        }
        output.put(static_cast<char>(static_cast<std::uint8_t>(tag.list_type)));
        if (!output || !write_unsigned(
                output,
                static_cast<std::uint32_t>(tag.list_value.size()),
                endian)) {
            return false;
        }
        for (const auto& child : tag.list_value) {
            if (child.type != tag.list_type ||
                !write_payload(output, child, endian, limits, depth + 1U)) {
                return false;
            }
        }
        return true;
    }
    case Type::Compound: {
        if (tag.compound_value.size() > limits.max_container_elements) {
            return false;
        }
        for (const auto& [name, child] : tag.compound_value) {
            if (!write_named_impl(
                    output, name, child, endian, limits, depth + 1U)) {
                return false;
            }
        }
        output.put(static_cast<char>(static_cast<std::uint8_t>(Type::End)));
        return static_cast<bool>(output);
    }
    case Type::IntArray: {
        if (tag.int_array_value.size() > limits.max_container_elements ||
            tag.int_array_value.size() > limits.max_array_bytes / sizeof(std::int32_t) ||
            tag.int_array_value.size() >
                static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        if (!write_unsigned(
                output,
                static_cast<std::uint32_t>(tag.int_array_value.size()),
                endian)) {
            return false;
        }
        for (const auto value : tag.int_array_value) {
            if (!write_unsigned(output, static_cast<std::uint32_t>(value), endian)) {
                return false;
            }
        }
        return true;
    }
    case Type::LongArray: {
        if (tag.long_array_value.size() > limits.max_container_elements ||
            tag.long_array_value.size() > limits.max_array_bytes / sizeof(std::int64_t) ||
            tag.long_array_value.size() >
                static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        if (!write_unsigned(
                output,
                static_cast<std::uint32_t>(tag.long_array_value.size()),
                endian)) {
            return false;
        }
        for (const auto value : tag.long_array_value) {
            if (!write_unsigned(output, static_cast<std::uint64_t>(value), endian)) {
                return false;
            }
        }
        return true;
    }
    }
    return false;
}

bool read_payload(
    std::istream& input,
    Type type,
    Tag& tag,
    Endian endian,
    Budget& budget,
    std::size_t depth);

bool read_named_impl(
    std::istream& input,
    std::string& name,
    Tag& tag,
    Endian endian,
    Budget& budget,
    std::size_t depth) {
    char raw_type = 0;
    if (!input.get(raw_type)) {
        return false;
    }
    const auto type_value = static_cast<std::uint8_t>(raw_type);
    if (!valid_type(type_value) || type_value == 0U) {
        return false;
    }
    if (!read_string(input, name, endian)) {
        return false;
    }
    return read_payload(
        input, static_cast<Type>(type_value), tag, endian, budget, depth);
}

bool read_payload(
    std::istream& input,
    Type type,
    Tag& tag,
    Endian endian,
    Budget& budget,
    std::size_t depth) {
    if (depth > budget.limits.max_depth) {
        return false;
    }

    tag = {};
    tag.type = type;

    switch (type) {
    case Type::End:
        return false;
    case Type::Byte: {
        char raw = 0;
        if (!input.get(raw)) {
            return false;
        }
        tag.byte_value = std::bit_cast<std::int8_t>(
            static_cast<std::uint8_t>(static_cast<unsigned char>(raw)));
        return true;
    }
    case Type::Short: {
        std::uint16_t value = 0;
        if (!read_unsigned(input, value, endian)) {
            return false;
        }
        tag.short_value = std::bit_cast<std::int16_t>(value);
        return true;
    }
    case Type::Int: {
        std::uint32_t value = 0;
        if (!read_unsigned(input, value, endian)) {
            return false;
        }
        tag.int_value = std::bit_cast<std::int32_t>(value);
        return true;
    }
    case Type::Long: {
        std::uint64_t value = 0;
        if (!read_unsigned(input, value, endian)) {
            return false;
        }
        tag.long_value = std::bit_cast<std::int64_t>(value);
        return true;
    }
    case Type::Float: {
        std::uint32_t value = 0;
        if (!read_unsigned(input, value, endian)) {
            return false;
        }
        tag.float_value = std::bit_cast<float>(value);
        return true;
    }
    case Type::Double: {
        std::uint64_t value = 0;
        if (!read_unsigned(input, value, endian)) {
            return false;
        }
        tag.double_value = std::bit_cast<double>(value);
        return true;
    }
    case Type::ByteArray: {
        std::uint32_t raw_length = 0;
        if (!read_unsigned(input, raw_length, endian)) {
            return false;
        }
        const auto signed_length = std::bit_cast<std::int32_t>(raw_length);
        if (signed_length < 0) {
            return false;
        }
        const auto length = static_cast<std::size_t>(signed_length);
        if (length > budget.limits.max_array_bytes || !budget.charge(length)) {
            return false;
        }
        tag.byte_array_value.resize(length);
        for (auto& value : tag.byte_array_value) {
            char raw = 0;
            if (!input.get(raw)) {
                return false;
            }
            value = std::bit_cast<std::int8_t>(
                static_cast<std::uint8_t>(static_cast<unsigned char>(raw)));
        }
        return true;
    }
    case Type::String:
        return read_string(input, tag.string_value, endian);
    case Type::List: {
        char raw_element_type = 0;
        std::uint32_t raw_length = 0;
        if (!input.get(raw_element_type) || !read_unsigned(input, raw_length, endian)) {
            return false;
        }
        const auto type_value = static_cast<std::uint8_t>(raw_element_type);
        if (!valid_type(type_value)) {
            return false;
        }
        const auto signed_length = std::bit_cast<std::int32_t>(raw_length);
        if (signed_length < 0) {
            return false;
        }
        const auto length = static_cast<std::size_t>(signed_length);
        if (length > budget.limits.max_container_elements ||
            !budget.charge(length) ||
            (length != 0U && type_value == 0U)) {
            return false;
        }
        tag.list_type = static_cast<Type>(type_value);
        tag.list_value.reserve(length);
        for (std::size_t index = 0; index < length; ++index) {
            Tag child;
            if (!read_payload(
                    input,
                    tag.list_type,
                    child,
                    endian,
                    budget,
                    depth + 1U)) {
                return false;
            }
            tag.list_value.push_back(std::move(child));
        }
        return true;
    }
    case Type::Compound: {
        while (true) {
            char raw_child_type = 0;
            if (!input.get(raw_child_type)) {
                return false;
            }
            const auto type_value = static_cast<std::uint8_t>(raw_child_type);
            if (!valid_type(type_value)) {
                return false;
            }
            if (type_value == 0U) {
                return true;
            }
            if (!budget.charge(1U)) {
                return false;
            }
            std::string child_name;
            if (!read_string(input, child_name, endian)) {
                return false;
            }
            Tag child;
            if (!read_payload(
                    input,
                    static_cast<Type>(type_value),
                    child,
                    endian,
                    budget,
                    depth + 1U)) {
                return false;
            }
            tag.compound_value.insert_or_assign(
                std::move(child_name), std::move(child));
        }
    }
    case Type::IntArray: {
        std::uint32_t raw_length = 0;
        if (!read_unsigned(input, raw_length, endian)) {
            return false;
        }
        const auto signed_length = std::bit_cast<std::int32_t>(raw_length);
        if (signed_length < 0) {
            return false;
        }
        const auto length = static_cast<std::size_t>(signed_length);
        if (length > budget.limits.max_container_elements ||
            length > budget.limits.max_array_bytes / sizeof(std::int32_t) ||
            !budget.charge(length)) {
            return false;
        }
        tag.int_array_value.resize(length);
        for (auto& value : tag.int_array_value) {
            std::uint32_t raw = 0;
            if (!read_unsigned(input, raw, endian)) {
                return false;
            }
            value = std::bit_cast<std::int32_t>(raw);
        }
        return true;
    }
    case Type::LongArray: {
        std::uint32_t raw_length = 0;
        if (!read_unsigned(input, raw_length, endian)) {
            return false;
        }
        const auto signed_length = std::bit_cast<std::int32_t>(raw_length);
        if (signed_length < 0) {
            return false;
        }
        const auto length = static_cast<std::size_t>(signed_length);
        if (length > budget.limits.max_container_elements ||
            length > budget.limits.max_array_bytes / sizeof(std::int64_t) ||
            !budget.charge(length)) {
            return false;
        }
        tag.long_array_value.resize(length);
        for (auto& value : tag.long_array_value) {
            std::uint64_t raw = 0;
            if (!read_unsigned(input, raw, endian)) {
                return false;
            }
            value = std::bit_cast<std::int64_t>(raw);
        }
        return true;
    }
    }
    return false;
}

} // namespace

Tag Tag::byte(std::int8_t value) {
    Tag tag;
    tag.type = Type::Byte;
    tag.byte_value = value;
    return tag;
}

Tag Tag::short_integer(std::int16_t value) {
    Tag tag;
    tag.type = Type::Short;
    tag.short_value = value;
    return tag;
}

Tag Tag::integer(std::int32_t value) {
    Tag tag;
    tag.type = Type::Int;
    tag.int_value = value;
    return tag;
}

Tag Tag::long_integer(std::int64_t value) {
    Tag tag;
    tag.type = Type::Long;
    tag.long_value = value;
    return tag;
}

Tag Tag::floating(float value) {
    Tag tag;
    tag.type = Type::Float;
    tag.float_value = value;
    return tag;
}

Tag Tag::double_floating(double value) {
    Tag tag;
    tag.type = Type::Double;
    tag.double_value = value;
    return tag;
}

Tag Tag::byte_array(std::vector<std::int8_t> value) {
    Tag tag;
    tag.type = Type::ByteArray;
    tag.byte_array_value = std::move(value);
    return tag;
}

Tag Tag::string(std::string value) {
    Tag tag;
    tag.type = Type::String;
    tag.string_value = std::move(value);
    return tag;
}

Tag Tag::list(Type element_type, std::vector<Tag> values) {
    Tag tag;
    tag.type = Type::List;
    tag.list_type = element_type;
    tag.list_value = std::move(values);
    return tag;
}

Tag Tag::compound(std::map<std::string, Tag> values) {
    Tag tag;
    tag.type = Type::Compound;
    tag.compound_value = std::move(values);
    return tag;
}

Tag Tag::int_array(std::vector<std::int32_t> value) {
    Tag tag;
    tag.type = Type::IntArray;
    tag.int_array_value = std::move(value);
    return tag;
}

Tag Tag::long_array(std::vector<std::int64_t> value) {
    Tag tag;
    tag.type = Type::LongArray;
    tag.long_array_value = std::move(value);
    return tag;
}

bool write_named(
    std::ostream& output,
    std::string_view name,
    const Tag& tag,
    Endian endian,
    const Limits& limits) {
    return write_named_impl(output, name, tag, endian, limits, 0U);
}

bool read_named(
    std::istream& input,
    std::string& name,
    Tag& tag,
    Endian endian,
    const Limits& limits) {
    Budget budget{limits};
    return read_named_impl(input, name, tag, endian, budget, 0U);
}

} // namespace mcpi::storage::nbt
