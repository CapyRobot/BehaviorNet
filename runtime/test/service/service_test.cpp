// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/service/HttpService.hpp>

#include <cassert>
#include <iostream>

void testHttpRequest()
{
    std::cout << "Testing HttpRequest..." << std::endl;

    bnet::service::HttpRequest req;
    req.url = "https://api.example.com/data";
    req.method = bnet::service::HttpMethod::POST;
    req.headers["Content-Type"] = "application/json";
    req.body = "{\"key\": \"value\"}";

    assert(req.url == "https://api.example.com/data");
    assert(req.method == bnet::service::HttpMethod::POST);
    assert(req.headers.size() == 1);
    assert(req.body.has_value());

    std::cout << "  PASSED" << std::endl;
}

void testHttpResponse()
{
    std::cout << "Testing HttpResponse..." << std::endl;

    bnet::service::HttpResponse success{200, {}, "{}", ""};
    assert(success.isSuccess());
    assert(!success.isError());

    bnet::service::HttpResponse notFound{404, {}, "Not Found", ""};
    assert(!notFound.isSuccess());
    assert(notFound.isError());

    bnet::service::HttpResponse serverError{500, {}, "", "Internal Server Error"};
    assert(!serverError.isSuccess());
    assert(serverError.isError());

    bnet::service::HttpResponse networkError{0, {}, "", "Connection failed"};
    assert(!networkError.isSuccess());
    assert(networkError.isError());

    std::cout << "  PASSED" << std::endl;
}

void testMockHttpService()
{
    std::cout << "Testing MockHttpService..." << std::endl;

    bnet::service::MockHttpService service;

    // Without expectations, returns 404
    bnet::service::HttpRequest req;
    req.url = "https://api.example.com/users";
    auto response = service.request(req);
    assert(response.statusCode == 404);

    // Add expectation
    service.expect("/users", bnet::service::HttpResponse{200, {}, "[{\"id\":1}]", ""});

    response = service.request(req);
    assert(response.statusCode == 200);
    assert(response.body == "[{\"id\":1}]");
    assert(service.requestCount() == 2);

    std::cout << "  PASSED" << std::endl;
}

void testMockHttpServiceMethodMatching()
{
    std::cout << "Testing MockHttpService method matching..." << std::endl;

    bnet::service::MockHttpService service;

    service.expect("/users", bnet::service::HttpMethod::GET,
        bnet::service::HttpResponse{200, {}, "GET response", ""});
    service.expect("/users", bnet::service::HttpMethod::POST,
        bnet::service::HttpResponse{201, {}, "POST response", ""});

    bnet::service::HttpRequest getReq;
    getReq.url = "https://api.example.com/users";
    getReq.method = bnet::service::HttpMethod::GET;

    auto getResponse = service.request(getReq);
    assert(getResponse.statusCode == 200);
    assert(getResponse.body == "GET response");

    bnet::service::HttpRequest postReq;
    postReq.url = "https://api.example.com/users";
    postReq.method = bnet::service::HttpMethod::POST;

    auto postResponse = service.request(postReq);
    assert(postResponse.statusCode == 201);
    assert(postResponse.body == "POST response");

    std::cout << "  PASSED" << std::endl;
}

void testMockHttpServiceAsync()
{
    std::cout << "Testing MockHttpService async..." << std::endl;

    bnet::service::MockHttpService service;
    service.expect("/data", bnet::service::HttpResponse{200, {}, "async data", ""});

    bool callbackInvoked = false;
    bnet::service::HttpResponse receivedResponse;

    bnet::service::HttpRequest req;
    req.url = "https://api.example.com/data";

    service.requestAsync(req, [&](bnet::service::HttpResponse resp)
    {
        callbackInvoked = true;
        receivedResponse = std::move(resp);
    });

    assert(service.hasPending());
    assert(!callbackInvoked);

    service.poll();

    assert(!service.hasPending());
    assert(callbackInvoked);
    assert(receivedResponse.statusCode == 200);
    assert(receivedResponse.body == "async data");

    std::cout << "  PASSED" << std::endl;
}

void testMockHttpServiceDefaultResponse()
{
    std::cout << "Testing MockHttpService default response..." << std::endl;

    bnet::service::MockHttpService service;
    service.setDefaultResponse(bnet::service::HttpResponse{503, {}, "Service Unavailable", ""});

    bnet::service::HttpRequest req;
    req.url = "https://unknown.example.com/endpoint";

    auto response = service.request(req);
    assert(response.statusCode == 503);
    assert(response.body == "Service Unavailable");

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Service Tests ===" << std::endl;

    testHttpRequest();
    testHttpResponse();
    testMockHttpService();
    testMockHttpServiceMethodMatching();
    testMockHttpServiceAsync();
    testMockHttpServiceDefaultResponse();

    std::cout << "\n=== All service tests passed ===" << std::endl;
    return 0;
}
