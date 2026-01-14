// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/service/HttpService.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace bnet::testing {

/// @brief A configurable test HTTP server for integration testing.
///
/// This server doesn't actually bind to a network port. Instead, it provides
/// an HttpService implementation that routes requests through configured handlers.
/// This allows deterministic testing without actual network calls.
///
/// Usage:
/// ```cpp
/// TestHttpServer server;
/// server.route("GET", "/users", [](const Request& req) {
///     return Response{200, {}, R"([{"id":1}])"};
/// });
/// auto httpService = server.createService();
/// ```
class TestHttpServer
{
public:
    struct Request
    {
        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::string body;
        std::map<std::string, std::string> queryParams;
    };

    struct Response
    {
        int statusCode{200};
        std::map<std::string, std::string> headers;
        std::string body;
    };

    using Handler = std::function<Response(const Request&)>;

    TestHttpServer() = default;
    ~TestHttpServer() = default;

    /// @brief Register a route handler.
    void route(const std::string& method, const std::string& path, Handler handler);

    /// @brief Register a GET route.
    void get(const std::string& path, Handler handler);

    /// @brief Register a POST route.
    void post(const std::string& path, Handler handler);

    /// @brief Register a PUT route.
    void put(const std::string& path, Handler handler);

    /// @brief Register a DELETE route.
    void del(const std::string& path, Handler handler);

    /// @brief Set a default handler for unmatched routes.
    void setDefaultHandler(Handler handler) { defaultHandler_ = std::move(handler); }

    /// @brief Set a handler that's called before each request (for logging, delays, etc.).
    void setPreRequestHook(std::function<void(const Request&)> hook) { preRequestHook_ = std::move(hook); }

    /// @brief Set a simulated network delay.
    void setDelay(std::chrono::milliseconds delay) { delay_ = delay; }

    /// @brief Create an HttpService that routes to this server.
    [[nodiscard]] std::shared_ptr<service::HttpService> createService();

    /// @brief Get the number of requests received.
    [[nodiscard]] std::size_t requestCount() const { return requestCount_; }

    /// @brief Get the last request received.
    [[nodiscard]] const Request& lastRequest() const { return lastRequest_; }

    /// @brief Get all requests received.
    [[nodiscard]] const std::vector<Request>& requests() const { return requests_; }

    /// @brief Clear request history.
    void clearHistory();

    /// @brief Process a request (used internally by the service).
    Response handleRequest(const Request& req);

    /// @brief Convert HttpMethod to string (used by TestHttpService).
    std::string methodToString(service::HttpMethod method) const;

    /// @brief Parse URL into path and query params (used by TestHttpService).
    std::pair<std::string, std::map<std::string, std::string>> parseUrl(const std::string& url) const;

private:
    struct Route
    {
        std::string method;
        std::string pathPattern;
        Handler handler;
    };

    std::vector<Route> routes_;
    Handler defaultHandler_;
    std::function<void(const Request&)> preRequestHook_;
    std::chrono::milliseconds delay_{0};

    mutable std::mutex mutex_;
    std::size_t requestCount_{0};
    Request lastRequest_;
    std::vector<Request> requests_;

    bool matchPath(const std::string& pattern, const std::string& path,
                   std::map<std::string, std::string>& params) const;
};

/// @brief HttpService implementation that routes to a TestHttpServer.
class TestHttpService : public service::HttpService
{
public:
    explicit TestHttpService(TestHttpServer& server) : server_(server) {}

    service::HttpResponse request(const service::HttpRequest& req) override;
    void requestAsync(const service::HttpRequest& req, service::HttpCallback callback) override;
    void poll() override;
    [[nodiscard]] bool hasPending() const override;

private:
    TestHttpServer& server_;
    std::vector<std::pair<service::HttpRequest, service::HttpCallback>> pending_;
    mutable std::mutex mutex_;
};

} // namespace bnet::testing
