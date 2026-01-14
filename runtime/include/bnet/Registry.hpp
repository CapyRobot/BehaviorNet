// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Actor.hpp"
#include "ActionResult.hpp"
#include "Token.hpp"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace bnet {

struct ActionInfo
{
    std::string id;           ///< e.g., "user::move_to_location"
    std::string actorTypeId;  ///< e.g., "user::Vehicle"
    bool requiresToken;       ///< true if action needs Token input
};

struct ActorTypeInfo
{
    std::string id;  ///< e.g., "user::Vehicle"
    ActorFactory factory;
    std::vector<std::string> actionIds;
};

/// @brief Registry for actor types and actions.
///
/// Populated at startup via BNET_REGISTER_* macros. Thread-safe singleton.
class Registry
{
public:
    static Registry& instance()
    {
        static Registry registry;
        return registry;
    }

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    void registerActor(std::string_view typeId, ActorFactory factory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string id(typeId);
        if (actorTypes_.find(id) != actorTypes_.end())
        {
            throw std::runtime_error("Actor type already registered: " + id);
        }
        actorTypes_[id] = ActorTypeInfo{id, std::move(factory), {}};
    }

    void registerAction(
        std::string_view actionId,
        std::string_view actorTypeId,
        ActionFunc func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string aid(actionId);
        std::string tid(actorTypeId);

        if (actions_.find(aid) != actions_.end())
        {
            throw std::runtime_error("Action already registered: " + aid);
        }

        actions_[aid] = ActionInfo{aid, tid, false};
        actionFuncs_[aid] = std::move(func);

        auto it = actorTypes_.find(tid);
        if (it != actorTypes_.end())
        {
            it->second.actionIds.push_back(aid);
        }
    }

    void registerActionWithToken(
        std::string_view actionId,
        std::string_view actorTypeId,
        ActionWithTokenFunc func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string aid(actionId);
        std::string tid(actorTypeId);

        if (actionsWithToken_.find(aid) != actionsWithToken_.end())
        {
            throw std::runtime_error("Action already registered: " + aid);
        }

        actions_[aid] = ActionInfo{aid, tid, true};
        actionsWithToken_[aid] = std::move(func);

        auto it = actorTypes_.find(tid);
        if (it != actorTypes_.end())
        {
            it->second.actionIds.push_back(aid);
        }
    }

    [[nodiscard]] std::unique_ptr<ActorBase> createActor(
        std::string_view typeId,
        const ActorParams& params) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = actorTypes_.find(std::string(typeId));
        if (it == actorTypes_.end())
        {
            throw std::runtime_error("Unknown actor type: " + std::string(typeId));
        }
        return it->second.factory(params);
    }

    [[nodiscard]] ActionResult invokeAction(
        std::string_view actionId,
        ActorBase& actor) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = actionFuncs_.find(std::string(actionId));
        if (it == actionFuncs_.end())
        {
            if (actionsWithToken_.find(std::string(actionId)) != actionsWithToken_.end())
            {
                throw std::runtime_error("Action requires token input: " + std::string(actionId));
            }
            throw std::runtime_error("Unknown action: " + std::string(actionId));
        }
        return it->second(actor);
    }

    [[nodiscard]] ActionResult invokeAction(
        std::string_view actionId,
        ActorBase& actor,
        const Token& token) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = actionsWithToken_.find(std::string(actionId));
        if (it == actionsWithToken_.end())
        {
            auto itNoToken = actionFuncs_.find(std::string(actionId));
            if (itNoToken != actionFuncs_.end())
            {
                return itNoToken->second(actor);
            }
            throw std::runtime_error("Unknown action: " + std::string(actionId));
        }
        return it->second(actor, token);
    }

    [[nodiscard]] bool hasActorType(std::string_view typeId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return actorTypes_.find(std::string(typeId)) != actorTypes_.end();
    }

    [[nodiscard]] bool hasAction(std::string_view actionId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return actions_.find(std::string(actionId)) != actions_.end();
    }

    [[nodiscard]] const ActionInfo& getActionInfo(std::string_view actionId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = actions_.find(std::string(actionId));
        if (it == actions_.end())
        {
            throw std::runtime_error("Unknown action: " + std::string(actionId));
        }
        return it->second;
    }

    [[nodiscard]] const ActorTypeInfo& getActorTypeInfo(std::string_view typeId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = actorTypes_.find(std::string(typeId));
        if (it == actorTypes_.end())
        {
            throw std::runtime_error("Unknown actor type: " + std::string(typeId));
        }
        return it->second;
    }

    [[nodiscard]] std::vector<std::string> getActorTypeIds() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        ids.reserve(actorTypes_.size());
        for (const auto& [id, _] : actorTypes_)
        {
            ids.push_back(id);
        }
        return ids;
    }

    [[nodiscard]] std::vector<std::string> getActionIds() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> ids;
        ids.reserve(actions_.size());
        for (const auto& [id, _] : actions_)
        {
            ids.push_back(id);
        }
        return ids;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        actorTypes_.clear();
        actions_.clear();
        actionFuncs_.clear();
        actionsWithToken_.clear();
    }

private:
    Registry() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ActorTypeInfo> actorTypes_;
    std::unordered_map<std::string, ActionInfo> actions_;
    std::unordered_map<std::string, ActionFunc> actionFuncs_;
    std::unordered_map<std::string, ActionWithTokenFunc> actionsWithToken_;
};

namespace detail {

struct ActorRegistrar
{
    ActorRegistrar(std::string_view typeId, ActorFactory factory)
    {
        Registry::instance().registerActor(typeId, std::move(factory));
    }
};

struct ActionRegistrar
{
    ActionRegistrar(
        std::string_view actionId,
        std::string_view actorTypeId,
        ActionFunc func)
    {
        Registry::instance().registerAction(actionId, actorTypeId, std::move(func));
    }
};

struct ActionWithTokenRegistrar
{
    ActionWithTokenRegistrar(
        std::string_view actionId,
        std::string_view actorTypeId,
        ActionWithTokenFunc func)
    {
        Registry::instance().registerActionWithToken(actionId, actorTypeId, std::move(func));
    }
};

} // namespace detail

} // namespace bnet

// Registration Macros ----------------------------------------------------------

#define BNET_CONCAT_IMPL(a, b) a##b
#define BNET_CONCAT(a, b) BNET_CONCAT_IMPL(a, b)
#define BNET_UNIQUE_NAME(prefix) BNET_CONCAT(prefix, __COUNTER__)

/// @brief Register an actor type with the runtime.
#define BNET_REGISTER_ACTOR(ActorClass, TypeId)                                    \
    static ::bnet::detail::ActorRegistrar BNET_UNIQUE_NAME(bnet_actor_reg_)(       \
        "user::" TypeId,                                                           \
        [](const ::bnet::ActorParams& params) -> std::unique_ptr<::bnet::ActorBase> { \
            return std::make_unique<ActorClass>(params);                           \
        })

/// @brief Register an action (no token input).
#define BNET_REGISTER_ACTOR_ACTION(ActorClass, Method, ActionId)                   \
    static ::bnet::detail::ActionRegistrar BNET_UNIQUE_NAME(bnet_action_reg_)(     \
        "user::" ActionId,                                                         \
        "user::" #ActorClass,                                                      \
        [](::bnet::ActorBase& actor) -> ::bnet::ActionResult {                     \
            return static_cast<ActorClass&>(actor).Method();                       \
        })

/// @brief Register an action with token input.
#define BNET_REGISTER_ACTOR_ACTION_WITH_TOKEN(ActorClass, Method, ActionId)        \
    static ::bnet::detail::ActionWithTokenRegistrar BNET_UNIQUE_NAME(bnet_action_tok_reg_)( \
        "user::" ActionId,                                                         \
        "user::" #ActorClass,                                                      \
        [](::bnet::ActorBase& actor, const ::bnet::Token& token) -> ::bnet::ActionResult { \
            return static_cast<ActorClass&>(actor).Method(token);                  \
        })
