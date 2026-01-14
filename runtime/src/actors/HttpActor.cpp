// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/actors/HttpActor.hpp"

#include <regex>

namespace bnet::actors {

HttpActor::HttpActor(std::shared_ptr<service::HttpService> httpService)
    : httpService_(std::move(httpService))
{
}

HttpActor::HttpActor(std::shared_ptr<service::HttpService> httpService, const ActorParams& params)
    : httpService_(std::move(httpService))
{
    if (params.has("base_url"))
    {
        baseUrl_ = params.get("base_url");
    }
}

std::string HttpActor::expandTokenParams(const std::string& input, const Token& token) const
{
    std::string result = input;
    std::regex pattern(R"(@token\{([^}]+)\})");
    std::smatch match;

    std::string::const_iterator searchStart = result.cbegin();
    std::string output;

    while (std::regex_search(searchStart, result.cend(), match, pattern))
    {
        output += std::string(searchStart, match[0].first);

        std::string key = match[1].str();
        if (token.hasData(key))
        {
            const auto& value = token.getData(key);
            if (value.is_string())
            {
                output += value.get<std::string>();
            }
            else
            {
                output += value.dump();
            }
        }
        else
        {
            // Keep original if key not found
            output += match[0].str();
        }

        searchStart = match[0].second;
    }
    output += std::string(searchStart, result.cend());

    return output;
}

ActionResult HttpActor::get(Token& token)
{
    return doRequest(token, service::HttpMethod::GET, false);
}

ActionResult HttpActor::post(Token& token)
{
    return doRequest(token, service::HttpMethod::POST, true);
}

ActionResult HttpActor::put(Token& token)
{
    return doRequest(token, service::HttpMethod::PUT, true);
}

ActionResult HttpActor::del(Token& token)
{
    return doRequest(token, service::HttpMethod::DELETE, false);
}

ActionResult HttpActor::doRequest(Token& token, service::HttpMethod method, bool hasBody)
{
    try
    {
        if (!httpService_)
        {
            return ActionResult::errorMessage("HTTP service not configured");
        }

        if (!token.hasData("url"))
        {
            return ActionResult::failure("Missing 'url' in token data");
        }

        service::HttpRequest req;
        req.method = method;

        // Build URL with base and expand token params
        std::string url = token.getData("url").get<std::string>();
        url = expandTokenParams(url, token);
        req.url = buildUrl(url);

        // Apply default headers
        req.headers = defaultHeaders_;

        // Apply token headers
        if (token.hasData("headers"))
        {
            const auto& headers = token.getData("headers");
            if (headers.is_object())
            {
                for (auto& [key, value] : headers.items())
                {
                    req.headers[key] = value.is_string() ? value.get<std::string>() : value.dump();
                }
            }
        }

        // Apply body if needed
        if (hasBody && token.hasData("body"))
        {
            const auto& body = token.getData("body");
            std::string bodyStr = body.is_string() ? body.get<std::string>() : body.dump();
            req.body = expandTokenParams(bodyStr, token);
        }

        // Apply timeout if specified
        if (token.hasData("timeout_ms"))
        {
            req.timeout = std::chrono::milliseconds(token.getData("timeout_ms").get<int>());
        }

        // Make the request
        auto response = httpService_->request(req);

        // Store response in token
        token.setData("status_code", response.statusCode);
        token.setData("response_body", response.body);

        nlohmann::json responseHeaders = nlohmann::json::object();
        for (const auto& [key, value] : response.headers)
        {
            responseHeaders[key] = value;
        }
        token.setData("response_headers", responseHeaders);

        if (!response.errorMessage.empty())
        {
            token.setData("error_message", response.errorMessage);
        }

        // Try to parse response as JSON
        try
        {
            auto jsonBody = nlohmann::json::parse(response.body);
            token.setData("response_json", jsonBody);
        }
        catch (...)
        {
            // Not JSON, that's fine
        }

        // Determine success based on status code
        if (response.isSuccess())
        {
            return ActionResult::success();
        }
        else if (response.statusCode == 0)
        {
            return ActionResult::errorMessage(response.errorMessage.empty() ? "Network error" : response.errorMessage);
        }
        else
        {
            return ActionResult::failure("HTTP " + std::to_string(response.statusCode));
        }
    }
    catch (const std::exception& e)
    {
        return ActionResult::errorMessage(e.what());
    }
}

std::string HttpActor::buildUrl(const std::string& url) const
{
    if (baseUrl_.empty())
    {
        return url;
    }
    if (url.find("://") != std::string::npos)
    {
        // Absolute URL
        return url;
    }
    // Relative URL
    if (!baseUrl_.empty() && baseUrl_.back() == '/' && !url.empty() && url.front() == '/')
    {
        return baseUrl_ + url.substr(1);
    }
    if (!baseUrl_.empty() && baseUrl_.back() != '/' && !url.empty() && url.front() != '/')
    {
        return baseUrl_ + "/" + url;
    }
    return baseUrl_ + url;
}

} // namespace bnet::actors
