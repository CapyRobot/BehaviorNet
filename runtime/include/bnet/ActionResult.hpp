// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace bnet {

class ErrorRegistry;

/// @brief Result of an action execution.
///
/// Actions return success(), failure(), inProgress(), or error<E>().
/// Errors use the exception hierarchy, enabling type-based filtering.
class ActionResult
{
public:
    enum class Status
    {
        Success,
        Failure,
        InProgress,
        Error
    };

    ActionResult() : status_(Status::Success) {}

    [[nodiscard]] static ActionResult success()
    {
        return ActionResult(Status::Success);
    }

    [[nodiscard]] static ActionResult failure()
    {
        return ActionResult(Status::Failure);
    }

    [[nodiscard]] static ActionResult failure(std::string_view message)
    {
        ActionResult result(Status::Failure);
        result.failureMessage_ = std::string(message);
        return result;
    }

    [[nodiscard]] static ActionResult inProgress()
    {
        return ActionResult(Status::InProgress);
    }

    /// @brief Create an error result from an exception type.
    template<typename E, typename... Args>
    [[nodiscard]] static ActionResult error(Args&&... args)
    {
        ActionResult result(Status::Error);
        result.error_ = std::make_exception_ptr(E(std::forward<Args>(args)...));
        return result;
    }

    /// @brief Create an error result from a string message (uses std::runtime_error).
    [[nodiscard]] static ActionResult errorMessage(std::string_view message)
    {
        ActionResult result(Status::Error);
        result.error_ = std::make_exception_ptr(std::runtime_error(std::string(message)));
        return result;
    }

    /// @brief Create an error result from an existing exception_ptr.
    [[nodiscard]] static ActionResult fromException(std::exception_ptr eptr)
    {
        ActionResult result(Status::Error);
        result.error_ = std::move(eptr);
        return result;
    }

    [[nodiscard]] Status status() const { return status_; }
    [[nodiscard]] bool isSuccess() const { return status_ == Status::Success; }
    [[nodiscard]] bool isFailure() const { return status_ == Status::Failure; }
    [[nodiscard]] bool isInProgress() const { return status_ == Status::InProgress; }
    [[nodiscard]] bool isError() const { return status_ == Status::Error; }
    [[nodiscard]] bool isTerminal() const { return status_ != Status::InProgress; }

    [[nodiscard]] std::exception_ptr exception() const { return error_; }

    [[noreturn]] void rethrow() const
    {
        std::rethrow_exception(error_);
    }

    /// @brief Check if the error matches a specific type (respects inheritance).
    template<typename E>
    [[nodiscard]] bool isErrorType() const
    {
        if (!isError() || !error_) return false;
        try
        {
            std::rethrow_exception(error_);
        }
        catch (const E&)
        {
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /// @brief Get the error as a specific type.
    /// @throws std::bad_cast if error is not of type E.
    template<typename E>
    [[nodiscard]] const E& getError() const
    {
        try
        {
            std::rethrow_exception(error_);
        }
        catch (const E&)
        {
            throw;
        }
        throw std::bad_cast();
    }

    [[nodiscard]] std::string errorMessage() const
    {
        if (!isError() || !error_) return "";
        try
        {
            std::rethrow_exception(error_);
        }
        catch (const std::exception& e)
        {
            return e.what();
        }
        catch (...)
        {
            return "Unknown error";
        }
    }

    [[nodiscard]] std::string errorTypeName() const;

    [[nodiscard]] const std::string& failureMessage() const { return failureMessage_; }

private:
    explicit ActionResult(Status status) : status_(status) {}

    Status status_;
    std::exception_ptr error_;
    std::string failureMessage_;
};

} // namespace bnet
