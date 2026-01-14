// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace bnet::error {

/// @brief Base class for all BehaviorNet errors.
///
/// Config-based error filtering uses C++ inheritance automatically.
/// Users can throw errors or return them via ActionResult::error<E>().
class Error : public std::exception
{
public:
    explicit Error(std::string message) : message_(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept override
    {
        return message_.c_str();
    }

    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;

protected:
    std::string message_;
};

// Runtime Errors ---------------------------------------------------------------

class RuntimeError : public Error
{
public:
    using Error::Error;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::RuntimeError";
    }
};

class NetworkError : public RuntimeError
{
public:
    using RuntimeError::RuntimeError;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::NetworkError";
    }
};

class TimeoutError : public NetworkError
{
public:
    explicit TimeoutError(std::string message, int timeoutSeconds = 0)
        : NetworkError(std::move(message)), timeoutSeconds_(timeoutSeconds) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::TimeoutError";
    }

    [[nodiscard]] int timeoutSeconds() const noexcept { return timeoutSeconds_; }

private:
    int timeoutSeconds_;
};

class ConnectionError : public NetworkError
{
public:
    explicit ConnectionError(std::string message, std::string endpoint = "")
        : NetworkError(std::move(message)), endpoint_(std::move(endpoint)) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ConnectionError";
    }

    [[nodiscard]] const std::string& endpoint() const noexcept { return endpoint_; }

private:
    std::string endpoint_;
};

// Resource Errors --------------------------------------------------------------

class ResourceError : public RuntimeError
{
public:
    using RuntimeError::RuntimeError;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ResourceError";
    }
};

class ActorNotFoundError : public ResourceError
{
public:
    explicit ActorNotFoundError(std::string actorType)
        : ResourceError("Actor not found: " + actorType), actorType_(std::move(actorType)) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ActorNotFoundError";
    }

    [[nodiscard]] const std::string& actorType() const noexcept { return actorType_; }

private:
    std::string actorType_;
};

class ResourceUnavailableError : public ResourceError
{
public:
    explicit ResourceUnavailableError(std::string resourceType)
        : ResourceError("Resource unavailable: " + resourceType),
          resourceType_(std::move(resourceType)) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ResourceUnavailableError";
    }

    [[nodiscard]] const std::string& resourceType() const noexcept { return resourceType_; }

private:
    std::string resourceType_;
};

// Action Errors ----------------------------------------------------------------

class ActionError : public RuntimeError
{
public:
    using RuntimeError::RuntimeError;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ActionError";
    }
};

class ActionCancelledError : public ActionError
{
public:
    explicit ActionCancelledError(std::string reason = "Action cancelled")
        : ActionError(std::move(reason)) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ActionCancelledError";
    }
};

class RetriesExhaustedError : public ActionError
{
public:
    explicit RetriesExhaustedError(std::string actionName, int attempts)
        : ActionError(actionName + " failed after " + std::to_string(attempts) + " attempts"),
          actionName_(std::move(actionName)), attempts_(attempts) {}

    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::RetriesExhaustedError";
    }

    [[nodiscard]] const std::string& actionName() const noexcept { return actionName_; }
    [[nodiscard]] int attempts() const noexcept { return attempts_; }

private:
    std::string actionName_;
    int attempts_;
};

// Validation Errors ------------------------------------------------------------

class ValidationError : public Error
{
public:
    using Error::Error;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ValidationError";
    }
};

class ConfigError : public ValidationError
{
public:
    using ValidationError::ValidationError;
    [[nodiscard]] std::string_view typeName() const noexcept override
    {
        return "bnet::error::ConfigError";
    }
};

} // namespace bnet::error

namespace bnet {

/// @brief Registry for error types, enabling config-based error filtering.
///
/// Inheritance is handled automatically by C++ exception catching.
class ErrorRegistry
{
public:
    static ErrorRegistry& instance()
    {
        static ErrorRegistry registry;
        return registry;
    }

    template<typename E>
    void registerError(std::string_view typeName)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string name(typeName);
        matchers_.insert_or_assign(name, [](const std::exception_ptr& eptr) -> bool {
            try
            {
                std::rethrow_exception(eptr);
            }
            catch (const E&)
            {
                return true;
            }
            catch (...)
            {
                return false;
            }
        });
    }

    [[nodiscard]] bool matches(
        const std::exception_ptr& eptr,
        std::string_view typeFilter) const
    {
        if (!eptr) return false;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = matchers_.find(std::string(typeFilter));
        if (it != matchers_.end())
        {
            return it->second(eptr);
        }
        return false;
    }

    [[nodiscard]] std::string getTypeName(const std::exception_ptr& eptr) const
    {
        if (!eptr) return "";

        try
        {
            std::rethrow_exception(eptr);
        }
        catch (const error::Error& e)
        {
            return std::string(e.typeName());
        }
        catch (const std::exception&)
        {
            return "std::exception";
        }
        catch (...)
        {
            return "unknown";
        }
    }

private:
    ErrorRegistry() { registerBuiltinErrors(); }
    void registerBuiltinErrors();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::function<bool(const std::exception_ptr&)>> matchers_;
};

} // namespace bnet
