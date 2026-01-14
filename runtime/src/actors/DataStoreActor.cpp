// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/actors/DataStoreActor.hpp"

namespace bnet::actors {

DataStoreActor::DataStoreActor(const ActorParams& params)
{
    // Initialize from params if provided
    if (params.has("initial_data"))
    {
        try
        {
            auto data = nlohmann::json::parse(params.get("initial_data"));
            fromJson(data);
        }
        catch (...)
        {
            // Ignore parse errors in initial data
        }
    }
}

void DataStoreActor::set(const std::string& key, nlohmann::json value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    store_[key] = std::move(value);
}

nlohmann::json DataStoreActor::get(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end())
    {
        return nlohmann::json();
    }
    return it->second;
}

nlohmann::json DataStoreActor::getOr(const std::string& key, nlohmann::json defaultValue) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end())
    {
        return defaultValue;
    }
    return it->second;
}

bool DataStoreActor::has(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.find(key) != store_.end();
}

bool DataStoreActor::remove(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.erase(key) > 0;
}

void DataStoreActor::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    store_.clear();
}

std::vector<std::string> DataStoreActor::keys() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(store_.size());
    for (const auto& [key, value] : store_)
    {
        result.push_back(key);
    }
    return result;
}

std::size_t DataStoreActor::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.size();
}

nlohmann::json DataStoreActor::toJson() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json result = nlohmann::json::object();
    for (const auto& [key, value] : store_)
    {
        result[key] = value;
    }
    return result;
}

void DataStoreActor::fromJson(const nlohmann::json& data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    store_.clear();
    if (data.is_object())
    {
        for (auto& [key, value] : data.items())
        {
            store_[key] = value;
        }
    }
}

ActionResult DataStoreActor::setValue(Token& token)
{
    try
    {
        if (!token.hasData("key"))
        {
            return ActionResult::failure("Missing 'key' in token data");
        }
        if (!token.hasData("value"))
        {
            return ActionResult::failure("Missing 'value' in token data");
        }

        auto key = token.getData("key").get<std::string>();
        auto value = token.getData("value");
        set(key, value);

        return ActionResult::success();
    }
    catch (const std::exception& e)
    {
        return ActionResult::errorMessage(e.what());
    }
}

ActionResult DataStoreActor::getValue(Token& token)
{
    try
    {
        if (!token.hasData("key"))
        {
            return ActionResult::failure("Missing 'key' in token data");
        }

        auto key = token.getData("key").get<std::string>();
        auto value = get(key);

        token.setData("result", value);
        return ActionResult::success();
    }
    catch (const std::exception& e)
    {
        return ActionResult::errorMessage(e.what());
    }
}

ActionResult DataStoreActor::hasKey(Token& token)
{
    try
    {
        if (!token.hasData("key"))
        {
            return ActionResult::failure("Missing 'key' in token data");
        }

        auto key = token.getData("key").get<std::string>();
        token.setData("exists", has(key));

        return ActionResult::success();
    }
    catch (const std::exception& e)
    {
        return ActionResult::errorMessage(e.what());
    }
}

ActionResult DataStoreActor::removeKey(Token& token)
{
    try
    {
        if (!token.hasData("key"))
        {
            return ActionResult::failure("Missing 'key' in token data");
        }

        auto key = token.getData("key").get<std::string>();
        bool removed = remove(key);
        token.setData("removed", removed);

        return ActionResult::success();
    }
    catch (const std::exception& e)
    {
        return ActionResult::errorMessage(e.what());
    }
}

} // namespace bnet::actors
