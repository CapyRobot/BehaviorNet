// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include <bnet/Token.hpp>
#include <bnet/actors/DataStoreActor.hpp>
#include <bnet/actors/HttpActor.hpp>
#include <bnet/service/HttpService.hpp>

#include <cassert>
#include <iostream>
#include <memory>

void testDataStoreBasic()
{
    std::cout << "Testing DataStoreActor basic operations..." << std::endl;

    bnet::actors::DataStoreActor store;

    assert(store.size() == 0);
    assert(!store.has("key1"));

    store.set("key1", "value1");
    assert(store.size() == 1);
    assert(store.has("key1"));
    assert(store.get("key1") == "value1");

    store.set("key2", 42);
    assert(store.get("key2") == 42);

    store.set("key3", nlohmann::json{{"nested", "object"}});
    assert(store.get("key3")["nested"] == "object");

    // Get with default
    assert(store.getOr("nonexistent", "default") == "default");
    assert(store.getOr("key1", "default") == "value1");

    // Keys
    auto keys = store.keys();
    assert(keys.size() == 3);

    // Remove
    assert(store.remove("key1"));
    assert(!store.has("key1"));
    assert(!store.remove("key1"));

    // Clear
    store.clear();
    assert(store.size() == 0);

    std::cout << "  PASSED" << std::endl;
}

void testDataStoreJson()
{
    std::cout << "Testing DataStoreActor JSON operations..." << std::endl;

    bnet::actors::DataStoreActor store;
    store.set("a", 1);
    store.set("b", "hello");

    auto json = store.toJson();
    assert(json["a"] == 1);
    assert(json["b"] == "hello");

    bnet::actors::DataStoreActor store2;
    store2.fromJson(json);
    assert(store2.get("a") == 1);
    assert(store2.get("b") == "hello");

    std::cout << "  PASSED" << std::endl;
}

void testDataStoreActions()
{
    std::cout << "Testing DataStoreActor actions..." << std::endl;

    bnet::actors::DataStoreActor store;

    // Test setValue
    bnet::Token token1;
    token1.setData("key", "test_key");
    token1.setData("value", "test_value");

    auto result1 = store.setValue(token1);
    assert(result1.isSuccess());
    assert(store.get("test_key") == "test_value");

    // Test getValue
    bnet::Token token2;
    token2.setData("key", "test_key");

    auto result2 = store.getValue(token2);
    assert(result2.isSuccess());
    assert(token2.getData("result") == "test_value");

    // Test hasKey
    bnet::Token token3;
    token3.setData("key", "test_key");

    auto result3 = store.hasKey(token3);
    assert(result3.isSuccess());
    assert(token3.getData("exists") == true);

    // Test removeKey
    bnet::Token token4;
    token4.setData("key", "test_key");

    auto result4 = store.removeKey(token4);
    assert(result4.isSuccess());
    assert(token4.getData("removed") == true);
    assert(!store.has("test_key"));

    std::cout << "  PASSED" << std::endl;
}

void testDataStoreActionErrors()
{
    std::cout << "Testing DataStoreActor action errors..." << std::endl;

    bnet::actors::DataStoreActor store;

    // Missing key
    bnet::Token token1;
    auto result1 = store.setValue(token1);
    assert(result1.isFailure());

    // Missing value
    bnet::Token token2;
    token2.setData("key", "test");
    auto result2 = store.setValue(token2);
    assert(result2.isFailure());

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorTokenExpansion()
{
    std::cout << "Testing HttpActor token expansion..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    bnet::actors::HttpActor http(mockHttp);

    bnet::Token token;
    token.setData("user_id", "123");
    token.setData("name", "test_user");
    token.setData("count", 42);

    auto expanded1 = http.expandTokenParams("/users/@token{user_id}", token);
    assert(expanded1 == "/users/123");

    auto expanded2 = http.expandTokenParams("/users/@token{user_id}/name/@token{name}", token);
    assert(expanded2 == "/users/123/name/test_user");

    auto expanded3 = http.expandTokenParams("count=@token{count}", token);
    assert(expanded3 == "count=42");

    // Unknown key keeps original
    auto expanded4 = http.expandTokenParams("@token{unknown}", token);
    assert(expanded4 == "@token{unknown}");

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorGet()
{
    std::cout << "Testing HttpActor GET..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    mockHttp->expect("/users/123", bnet::service::HttpResponse{
        200, {{"Content-Type", "application/json"}}, R"({"id":123,"name":"John"})", ""
    });

    bnet::actors::HttpActor http(mockHttp);

    bnet::Token token;
    token.setData("url", "https://api.example.com/users/123");

    auto result = http.get(token);
    assert(result.isSuccess());
    assert(token.getData("status_code") == 200);
    assert(token.hasData("response_json"));
    assert(token.getData("response_json")["id"] == 123);

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorPost()
{
    std::cout << "Testing HttpActor POST..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    mockHttp->expect("/users", bnet::service::HttpMethod::POST, bnet::service::HttpResponse{
        201, {}, R"({"id":456})", ""
    });

    bnet::actors::HttpActor http(mockHttp);

    bnet::Token token;
    token.setData("url", "https://api.example.com/users");
    token.setData("body", R"({"name":"Jane"})");

    auto result = http.post(token);
    assert(result.isSuccess());
    assert(token.getData("status_code") == 201);

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorWithBaseUrl()
{
    std::cout << "Testing HttpActor with base URL..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    mockHttp->expect("/users", bnet::service::HttpResponse{200, {}, "[]", ""});

    bnet::actors::HttpActor http(mockHttp);
    http.setBaseUrl("https://api.example.com");

    bnet::Token token;
    token.setData("url", "/users");

    auto result = http.get(token);
    assert(result.isSuccess());

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorError()
{
    std::cout << "Testing HttpActor error handling..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    mockHttp->expect("/error", bnet::service::HttpResponse{500, {}, "", "Server Error"});

    bnet::actors::HttpActor http(mockHttp);

    bnet::Token token;
    token.setData("url", "https://api.example.com/error");

    auto result = http.get(token);
    assert(result.isFailure());
    assert(token.getData("status_code") == 500);

    std::cout << "  PASSED" << std::endl;
}

void testHttpActorMissingUrl()
{
    std::cout << "Testing HttpActor missing URL..." << std::endl;

    auto mockHttp = std::make_shared<bnet::service::MockHttpService>();
    bnet::actors::HttpActor http(mockHttp);

    bnet::Token token;
    // No URL set

    auto result = http.get(token);
    assert(result.isFailure());

    std::cout << "  PASSED" << std::endl;
}

int main()
{
    std::cout << "=== BehaviorNet Actors Tests ===" << std::endl;

    testDataStoreBasic();
    testDataStoreJson();
    testDataStoreActions();
    testDataStoreActionErrors();
    testHttpActorTokenExpansion();
    testHttpActorGet();
    testHttpActorPost();
    testHttpActorWithBaseUrl();
    testHttpActorError();
    testHttpActorMissingUrl();

    std::cout << "\n=== All actors tests passed ===" << std::endl;
    return 0;
}
