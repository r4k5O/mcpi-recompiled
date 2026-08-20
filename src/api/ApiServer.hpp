#pragma once

#include "api/Command.hpp"
#include "api/CommandResult.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace mcpi::api {

class ApiServer {
public:
    using Handler = std::function<std::optional<std::string>(const Command&)>;
    using ResultHandler = std::function<CommandResult(const Command&)>;

    explicit ApiServer(std::uint16_t port = 4711);
    ~ApiServer();

    ApiServer(const ApiServer&) = delete;
    ApiServer& operator=(const ApiServer&) = delete;
    ApiServer(ApiServer&&) = delete;
    ApiServer& operator=(ApiServer&&) = delete;

    bool start(Handler handler);
    bool start(ResultHandler handler);
    void stop();

    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] std::uint16_t port() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mcpi::api
