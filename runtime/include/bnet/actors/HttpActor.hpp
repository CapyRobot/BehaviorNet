// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/ActionResult.hpp>
#include <bnet/Actor.hpp>
#include <bnet/Token.hpp>
#include <bnet/service/HttpService.hpp>

#include <memory>
#include <string>

namespace bnet::actors {

/// @brief HTTP client actor for making REST API calls.
///
/// Supports GET, POST, PUT, DELETE operations with token parameter expansion.
/// Parameters in URLs and bodies can reference token data using @token{key} syntax.
class HttpActor : public ActorBase
{
public:
    explicit HttpActor(std::shared_ptr<service::HttpService> httpService);
    HttpActor(std::shared_ptr<service::HttpService> httpService, const ActorParams& params);

    /// @brief Expand @token{key} patterns in a string.
    [[nodiscard]] std::string expandTokenParams(const std::string& input, const Token& token) const;

    /// @brief Action: HTTP GET request.
    /// Token data: url (required), headers (optional)
    /// Result: status_code, response_body, response_headers
    ActionResult get(Token& token);

    /// @brief Action: HTTP POST request.
    /// Token data: url, body (required), headers (optional)
    ActionResult post(Token& token);

    /// @brief Action: HTTP PUT request.
    /// Token data: url, body (required), headers (optional)
    ActionResult put(Token& token);

    /// @brief Action: HTTP DELETE request.
    /// Token data: url (required), headers (optional)
    ActionResult del(Token& token);

    /// @brief Set base URL for relative paths.
    void setBaseUrl(const std::string& baseUrl) { baseUrl_ = baseUrl; }

    /// @brief Set default headers for all requests.
    void setDefaultHeaders(std::map<std::string, std::string> headers)
    {
        defaultHeaders_ = std::move(headers);
    }

private:
    ActionResult doRequest(Token& token, service::HttpMethod method, bool hasBody);
    std::string buildUrl(const std::string& url) const;

    std::shared_ptr<service::HttpService> httpService_;
    std::string baseUrl_;
    std::map<std::string, std::string> defaultHeaders_;
};

} // namespace bnet::actors
