#pragma once

#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace mcpi::storage::nbt {

enum class Endian {
    Little,
    Big,
};

enum class Type : std::uint8_t {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10,
    IntArray = 11,
    LongArray = 12,
};

struct Tag {
    Type type = Type::End;
    std::int8_t byte_value = 0;
    std::int16_t short_value = 0;
    std::int32_t int_value = 0;
    std::int64_t long_value = 0;
    float float_value = 0.0F;
    double double_value = 0.0;
    std::vector<std::int8_t> byte_array_value;
    std::string string_value;
    Type list_type = Type::End;
    std::vector<Tag> list_value;
    std::map<std::string, Tag> compound_value;
    std::vector<std::int32_t> int_array_value;
    std::vector<std::int64_t> long_array_value;

    static Tag byte(std::int8_t value);
    static Tag short_integer(std::int16_t value);
    static Tag integer(std::int32_t value);
    static Tag long_integer(std::int64_t value);
    static Tag floating(float value);
    static Tag double_floating(double value);
    static Tag byte_array(std::vector<std::int8_t> value);
    static Tag string(std::string value);
    static Tag list(Type element_type, std::vector<Tag> values);
    static Tag compound(std::map<std::string, Tag> values);
    static Tag int_array(std::vector<std::int32_t> value);
    static Tag long_array(std::vector<std::int64_t> value);

    bool operator==(const Tag&) const = default;
};

struct Limits {
    std::size_t max_depth = 64U;
    std::size_t max_container_elements = 1'048'576U;
    std::size_t max_array_bytes = 16U * 1024U * 1024U;
};

bool write_named(
    std::ostream& output,
    std::string_view name,
    const Tag& tag,
    Endian endian,
    const Limits& limits = {});

bool read_named(
    std::istream& input,
    std::string& name,
    Tag& tag,
    Endian endian,
    const Limits& limits = {});

} // namespace mcpi::storage::nbt
