// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/service/HttpService.hpp"

#include <algorithm>

namespace bnet::service {

HttpResponse MockHttpService::request(const HttpRequest& req)
{
    ++requestCount_;
    return findResponse(req);
}

void MockHttpService::requestAsync(const HttpRequest& req, HttpCallback callback)
{
    ++requestCount_;
    pending_.emplace_back(req, std::move(callback));
}

void MockHttpService::poll()
{
    auto pending = std::move(pending_);
    pending_.clear();

    for (auto& [req, callback] : pending)
    {
        if (callback)
        {
            callback(findResponse(req));
        }
    }
}

bool MockHttpService::hasPending() const
{
    return !pending_.empty();
}

void MockHttpService::expect(std::string urlPattern, HttpResponse response)
{
    expectations_.push_back(Expectation{std::move(urlPattern), std::nullopt, std::move(response)});
}

void MockHttpService::expect(std::string urlPattern, HttpMethod method, HttpResponse response)
{
    expectations_.push_back(Expectation{std::move(urlPattern), method, std::move(response)});
}

void MockHttpService::clearExpectations()
{
    expectations_.clear();
}

HttpResponse MockHttpService::findResponse(const HttpRequest& req)
{
    // Find matching expectation (reverse order - last added wins)
    for (auto it = expectations_.rbegin(); it != expectations_.rend(); ++it)
    {
        // Simple pattern matching - check if URL contains the pattern
        if (req.url.find(it->urlPattern) != std::string::npos)
        {
            if (!it->method.has_value() || it->method.value() == req.method)
            {
                return it->response;
            }
        }
    }

    // Return default or 404
    if (defaultResponse_.has_value())
    {
        return defaultResponse_.value();
    }

    return HttpResponse{404, {}, "Not Found", ""};
}

} // namespace bnet::service
