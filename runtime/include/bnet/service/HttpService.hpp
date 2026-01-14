// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace bnet::service {

/// @brief HTTP method types.
enum class HttpMethod
{
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

/// @brief HTTP request configuration.
struct HttpRequest
{
    std::string url;
    HttpMethod method{HttpMethod::GET};
    std::map<std::string, std::string> headers;
    std::optional<std::string> body;
    std::chrono::milliseconds timeout{30000};
};

/// @brief HTTP response data.
struct HttpResponse
{
    int statusCode{0};
    std::map<std::string, std::string> headers;
    std::string body;
    std::string errorMessage;

    [[nodiscard]] bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
    [[nodiscard]] bool isError() const { return statusCode == 0 || statusCode >= 400; }
};

/// @brief Callback for async HTTP responses.
using HttpCallback = std::function<void(HttpResponse)>;

/// @brief Abstract HTTP service interface.
///
/// Implementations can use libcurl, mock responses, etc.
class HttpService
{
public:
    virtual ~HttpService() = default;

    /// @brief Perform a synchronous HTTP request.
    [[nodiscard]] virtual HttpResponse request(const HttpRequest& req) = 0;

    /// @brief Perform an asynchronous HTTP request.
    virtual void requestAsync(const HttpRequest& req, HttpCallback callback) = 0;

    /// @brief Poll pending async requests.
    virtual void poll() = 0;

    /// @brief Check if there are pending async requests.
    [[nodiscard]] virtual bool hasPending() const = 0;
};

/// @brief Mock HTTP service for testing.
///
/// Allows pre-configuring responses for specific URLs.
class MockHttpService : public HttpService
{
public:
    MockHttpService() = default;

    HttpResponse request(const HttpRequest& req) override;
    void requestAsync(const HttpRequest& req, HttpCallback callback) override;
    void poll() override;
    [[nodiscard]] bool hasPending() const override;

    /// @brief Add expected response for a URL pattern.
    void expect(std::string urlPattern, HttpResponse response);

    /// @brief Add expected response for URL and method.
    void expect(std::string urlPattern, HttpMethod method, HttpResponse response);

    /// @brief Clear all expectations.
    void clearExpectations();

    /// @brief Get number of requests made.
    [[nodiscard]] std::size_t requestCount() const { return requestCount_; }

    /// @brief Set default response for unmatched requests.
    void setDefaultResponse(HttpResponse response) { defaultResponse_ = std::move(response); }

private:
    struct Expectation
    {
        std::string urlPattern;
        std::optional<HttpMethod> method;
        HttpResponse response;
    };

    std::vector<Expectation> expectations_;
    std::vector<std::pair<HttpRequest, HttpCallback>> pending_;
    std::optional<HttpResponse> defaultResponse_;
    std::size_t requestCount_{0};

    HttpResponse findResponse(const HttpRequest& req);
};

} // namespace bnet::service
