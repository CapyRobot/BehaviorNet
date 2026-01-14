// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/service/HttpService.hpp>
#include <bnet/testing/TestHttpServer.hpp>

#include <cassert>
#include <iostream>

void testBasicRouting()
{
    std::cout << "Testing basic routing..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.get("/users", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{200, {}, R"([{"id":1}])"};
    });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/users";
    req.method = bnet::service::HttpMethod::GET;

    auto resp = httpService->request(req);
    assert(resp.statusCode == 200);
    assert(resp.body == R"([{"id":1}])");
    assert(server.requestCount() == 1);

    std::cout << "  PASSED" << std::endl;
}

void testPathParameters()
{
    std::cout << "Testing path parameters..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.get("/users/:id", [](const bnet::testing::TestHttpServer::Request& req)
    {
        auto id = req.queryParams.at("id");
        return bnet::testing::TestHttpServer::Response{200, {}, R"({"id":)" + id + "}"};
    });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/users/42";
    req.method = bnet::service::HttpMethod::GET;

    auto resp = httpService->request(req);
    assert(resp.statusCode == 200);
    assert(resp.body == R"({"id":42})");

    std::cout << "  PASSED" << std::endl;
}

void testQueryParameters()
{
    std::cout << "Testing query parameters..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.get("/search", [](const bnet::testing::TestHttpServer::Request& req)
    {
        auto it = req.queryParams.find("q");
        std::string query = it != req.queryParams.end() ? it->second : "";
        return bnet::testing::TestHttpServer::Response{200, {}, R"({"query":")" + query + "\"}"};
    });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/search?q=test&limit=10";
    req.method = bnet::service::HttpMethod::GET;

    auto resp = httpService->request(req);
    assert(resp.statusCode == 200);
    assert(resp.body == R"({"query":"test"})");

    std::cout << "  PASSED" << std::endl;
}

void testPostWithBody()
{
    std::cout << "Testing POST with body..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.post("/users", [](const bnet::testing::TestHttpServer::Request& req)
    {
        return bnet::testing::TestHttpServer::Response{201, {}, req.body};
    });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/users";
    req.method = bnet::service::HttpMethod::POST;
    req.body = R"({"name":"John"})";

    auto resp = httpService->request(req);
    assert(resp.statusCode == 201);
    assert(resp.body == R"({"name":"John"})");

    std::cout << "  PASSED" << std::endl;
}

void testNotFound()
{
    std::cout << "Testing 404 for unmatched routes..." << std::endl;

    bnet::testing::TestHttpServer server;

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/nonexistent";
    req.method = bnet::service::HttpMethod::GET;

    auto resp = httpService->request(req);
    assert(resp.statusCode == 404);

    std::cout << "  PASSED" << std::endl;
}

void testDefaultHandler()
{
    std::cout << "Testing default handler..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.setDefaultHandler([](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{503, {}, "Service Unavailable"};
    });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/any";
    req.method = bnet::service::HttpMethod::GET;

    auto resp = httpService->request(req);
    assert(resp.statusCode == 503);

    std::cout << "  PASSED" << std::endl;
}

void testAsyncRequest()
{
    std::cout << "Testing async request..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.get("/data", [](const bnet::testing::TestHttpServer::Request&)
    {
        return bnet::testing::TestHttpServer::Response{200, {}, "async data"};
    });

    auto httpService = server.createService();

    bool completed = false;
    bnet::service::HttpResponse receivedResp;

    bnet::service::HttpRequest req;
    req.url = "http://localhost/data";
    req.method = bnet::service::HttpMethod::GET;

    httpService->requestAsync(req, [&](bnet::service::HttpResponse resp)
    {
        completed = true;
        receivedResp = std::move(resp);
    });

    assert(httpService->hasPending());
    httpService->poll();
    assert(!httpService->hasPending());
    assert(completed);
    assert(receivedResp.statusCode == 200);
    assert(receivedResp.body == "async data");

    std::cout << "  PASSED" << std::endl;
}

void testRequestHistory()
{
    std::cout << "Testing request history..." << std::endl;

    bnet::testing::TestHttpServer server;

    server.get("/a", [](const auto&) { return bnet::testing::TestHttpServer::Response{200, {}, "a"}; });
    server.get("/b", [](const auto&) { return bnet::testing::TestHttpServer::Response{200, {}, "b"}; });

    auto httpService = server.createService();

    bnet::service::HttpRequest req1;
    req1.url = "http://localhost/a";
    httpService->request(req1);

    bnet::service::HttpRequest req2;
    req2.url = "http://localhost/b";
    httpService->request(req2);

    assert(server.requestCount() == 2);
    assert(server.requests().size() == 2);
    assert(server.requests()[0].path == "/a");
    assert(server.requests()[1].path == "/b");
    assert(server.lastRequest().path == "/b");

    server.clearHistory();
    assert(server.requestCount() == 0);
    assert(server.requests().empty());

    std::cout << "  PASSED" << std::endl;
}

void testPreRequestHook()
{
    std::cout << "Testing pre-request hook..." << std::endl;

    bnet::testing::TestHttpServer server;

    int hookCalls = 0;
    server.setPreRequestHook([&](const bnet::testing::TestHttpServer::Request&)
    {
        ++hookCalls;
    });

    server.get("/test", [](const auto&) { return bnet::testing::TestHttpServer::Response{200, {}, ""}; });

    auto httpService = server.createService();

    bnet::service::HttpRequest req;
    req.url = "http://localhost/test";
    httpService->request(req);
    httpService->request(req);

    assert(hookCalls == 2);

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Testing Utilities Tests ===" << std::endl;

    testBasicRouting();
    testPathParameters();
    testQueryParameters();
    testPostWithBody();
    testNotFound();
    testDefaultHandler();
    testAsyncRequest();
    testRequestHistory();
    testPreRequestHook();

    std::cout << "\n=== All testing utility tests passed ===" << std::endl;
    return 0;
}
