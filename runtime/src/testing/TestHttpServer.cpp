// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/testing/TestHttpServer.hpp"

#include <regex>
#include <sstream>
#include <thread>

namespace bnet::testing {

void TestHttpServer::route(const std::string& method, const std::string& path, Handler handler)
{
    routes_.push_back(Route{method, path, std::move(handler)});
}

void TestHttpServer::get(const std::string& path, Handler handler)
{
    route("GET", path, std::move(handler));
}

void TestHttpServer::post(const std::string& path, Handler handler)
{
    route("POST", path, std::move(handler));
}

void TestHttpServer::put(const std::string& path, Handler handler)
{
    route("PUT", path, std::move(handler));
}

void TestHttpServer::del(const std::string& path, Handler handler)
{
    route("DELETE", path, std::move(handler));
}

std::shared_ptr<service::HttpService> TestHttpServer::createService()
{
    return std::make_shared<TestHttpService>(*this);
}

void TestHttpServer::clearHistory()
{
    std::lock_guard<std::mutex> lock(mutex_);
    requestCount_ = 0;
    requests_.clear();
    lastRequest_ = Request{};
}

TestHttpServer::Response TestHttpServer::handleRequest(const Request& req)
{
    // Record the request
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++requestCount_;
        lastRequest_ = req;
        requests_.push_back(req);
    }

    // Call pre-request hook
    if (preRequestHook_)
    {
        preRequestHook_(req);
    }

    // Simulate network delay
    if (delay_.count() > 0)
    {
        std::this_thread::sleep_for(delay_);
    }

    // Find matching route
    for (const auto& route : routes_)
    {
        if (route.method == req.method)
        {
            std::map<std::string, std::string> params;
            if (matchPath(route.pathPattern, req.path, params))
            {
                // Create a copy of request with path params merged
                Request reqWithParams = req;
                for (const auto& [key, value] : params)
                {
                    reqWithParams.queryParams[key] = value;
                }
                return route.handler(reqWithParams);
            }
        }
    }

    // Use default handler or return 404
    if (defaultHandler_)
    {
        return defaultHandler_(req);
    }

    return Response{404, {{"Content-Type", "text/plain"}}, "Not Found"};
}

bool TestHttpServer::matchPath(const std::string& pattern, const std::string& path,
                                std::map<std::string, std::string>& params) const
{
    // Simple pattern matching with :param syntax
    // e.g., "/users/:id" matches "/users/123" with params["id"] = "123"

    if (pattern == path)
    {
        return true;
    }

    // Split by /
    auto splitPath = [](const std::string& p) -> std::vector<std::string>
    {
        std::vector<std::string> parts;
        std::istringstream iss(p);
        std::string part;
        while (std::getline(iss, part, '/'))
        {
            if (!part.empty())
            {
                parts.push_back(part);
            }
        }
        return parts;
    };

    auto patternParts = splitPath(pattern);
    auto pathParts = splitPath(path);

    if (patternParts.size() != pathParts.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < patternParts.size(); ++i)
    {
        if (patternParts[i].front() == ':')
        {
            // This is a parameter
            std::string paramName = patternParts[i].substr(1);
            params[paramName] = pathParts[i];
        }
        else if (patternParts[i] != pathParts[i])
        {
            return false;
        }
    }

    return true;
}

std::string TestHttpServer::methodToString(service::HttpMethod method) const
{
    switch (method)
    {
        case service::HttpMethod::GET: return "GET";
        case service::HttpMethod::POST: return "POST";
        case service::HttpMethod::PUT: return "PUT";
        case service::HttpMethod::DELETE: return "DELETE";
        case service::HttpMethod::PATCH: return "PATCH";
    }
    return "GET";
}

std::pair<std::string, std::map<std::string, std::string>> TestHttpServer::parseUrl(const std::string& url) const
{
    std::string path = url;
    std::map<std::string, std::string> queryParams;

    // Remove scheme and host if present
    auto schemePos = url.find("://");
    if (schemePos != std::string::npos)
    {
        auto hostEnd = url.find('/', schemePos + 3);
        if (hostEnd != std::string::npos)
        {
            path = url.substr(hostEnd);
        }
        else
        {
            path = "/";
        }
    }

    // Parse query string
    auto queryPos = path.find('?');
    if (queryPos != std::string::npos)
    {
        std::string queryString = path.substr(queryPos + 1);
        path = path.substr(0, queryPos);

        std::istringstream iss(queryString);
        std::string param;
        while (std::getline(iss, param, '&'))
        {
            auto eqPos = param.find('=');
            if (eqPos != std::string::npos)
            {
                queryParams[param.substr(0, eqPos)] = param.substr(eqPos + 1);
            }
            else
            {
                queryParams[param] = "";
            }
        }
    }

    return {path, queryParams};
}

// TestHttpService implementation

service::HttpResponse TestHttpService::request(const service::HttpRequest& req)
{
    auto [path, queryParams] = server_.parseUrl(req.url);

    TestHttpServer::Request testReq;
    testReq.method = server_.methodToString(req.method);
    testReq.path = path;
    testReq.headers = req.headers;
    testReq.body = req.body.value_or("");
    testReq.queryParams = queryParams;

    auto testResp = server_.handleRequest(testReq);

    service::HttpResponse resp;
    resp.statusCode = testResp.statusCode;
    resp.headers = testResp.headers;
    resp.body = testResp.body;

    return resp;
}

void TestHttpService::requestAsync(const service::HttpRequest& req, service::HttpCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pending_.emplace_back(req, std::move(callback));
}

void TestHttpService::poll()
{
    std::vector<std::pair<service::HttpRequest, service::HttpCallback>> toProcess;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        toProcess = std::move(pending_);
        pending_.clear();
    }

    for (auto& [req, callback] : toProcess)
    {
        auto response = request(req);
        if (callback)
        {
            callback(response);
        }
    }
}

bool TestHttpService::hasPending() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return !pending_.empty();
}

} // namespace bnet::testing
