// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/runtime/RuntimeController.hpp"

#include <bnet/places/ActionPlace.hpp>
#include <bnet/places/EntrypointPlace.hpp>
#include <bnet/places/ExitLoggerPlace.hpp>
#include <bnet/places/PlainPlace.hpp>
#include <bnet/places/ResourcePoolPlace.hpp>
#include <bnet/places/WaitWithTimeoutPlace.hpp>

namespace bnet::runtime {

RuntimeController::RuntimeController() = default;

RuntimeController::~RuntimeController()
{
    stop();
}

bool RuntimeController::loadConfig(const config::NetConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    errors_.clear();
    placeTypes_.clear();
    loadedConfig_ = config;

    return createNetFromConfig(config);
}

bool RuntimeController::loadConfigString(const std::string& json)
{
    config::ConfigParser parser;
    auto result = parser.parseString(json);

    if (!result.success)
    {
        for (const auto& err : result.errors)
        {
            errors_.push_back(err.path + ": " + err.message);
        }
        return false;
    }

    return loadConfig(result.config);
}

bool RuntimeController::loadConfigFile(const std::string& path)
{
    config::ConfigParser parser;
    auto result = parser.parseFile(path);

    if (!result.success)
    {
        for (const auto& err : result.errors)
        {
            errors_.push_back(err.path + ": " + err.message);
        }
        return false;
    }

    return loadConfig(result.config);
}

void RuntimeController::start()
{
    if (state_ != RuntimeState::Stopped)
    {
        return;
    }

    state_ = RuntimeState::Starting;
    stats_.startTime = std::chrono::steady_clock::now();
    stats_.epoch = 0;
    stats_.transitionsFired = 0;
    stats_.tokensProcessed = 0;

    state_ = RuntimeState::Running;
    log("Runtime started");

    runThread_ = std::thread([this]() { runLoop(); });
}

void RuntimeController::stop()
{
    if (state_ != RuntimeState::Running)
    {
        return;
    }

    state_ = RuntimeState::Stopping;

    if (runThread_.joinable())
    {
        runThread_.join();
    }

    state_ = RuntimeState::Stopped;
    log("Runtime stopped");
}

void RuntimeController::tick()
{
    std::lock_guard<std::mutex> lock(mutex_);
    processTick();
}

core::TokenId RuntimeController::injectToken(const std::string& entrypointId, Token token)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = placeTypes_.find(entrypointId);
    if (it == placeTypes_.end())
    {
        log("Entrypoint not found: " + entrypointId);
        return 0;
    }

    auto* entryplace = dynamic_cast<places::EntrypointPlace*>(it->second.get());
    if (!entryplace)
    {
        log("Place is not an entrypoint: " + entrypointId);
        return 0;
    }

    auto id = entryplace->inject(std::move(token));
    if (id != 0)
    {
        ++stats_.tokensProcessed;
        log("Token injected at " + entrypointId);
    }

    return id;
}

RuntimeStats RuntimeController::stats() const
{
    RuntimeStats s = stats_;

    // Count active tokens (in places + in-flight actions)
    std::size_t count = 0;
    for (const auto* place : net_.getAllPlaces())
    {
        count += place->tokenCount();
    }
    // Also count tokens in in-flight actions
    count += executor_.inFlightCount();
    s.activeTokens = count;

    return s;
}

std::vector<std::pair<core::TokenId, nlohmann::json>> RuntimeController::getPlaceTokens(const std::string& placeId) const
{
    std::vector<std::pair<core::TokenId, nlohmann::json>> result;

    const auto* place = net_.getPlace(placeId);
    if (!place)
    {
        return result;
    }

    // Get tokens from main queue - getAllTokens already returns (id, json) pairs
    for (const auto& [id, data] : place->tokens().getAllTokens())
    {
        result.emplace_back(id, data);
    }

    // Get tokens from subplaces if enabled
    if (place->hasSubplaces())
    {
        for (auto sub : {core::Subplace::InExecution, core::Subplace::Success, core::Subplace::Failure, core::Subplace::Error})
        {
            for (const auto& [id, data] : place->subplace(sub).getAllTokens())
            {
                result.emplace_back(id, data);
            }
        }
    }

    return result;
}

void RuntimeController::registerAction(const std::string& actionId, execution::ActionInvoker invoker)
{
    actionInvokers_[actionId] = std::move(invoker);
}

void RuntimeController::runLoop()
{
    while (state_ == RuntimeState::Running)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            processTick();
        }
        std::this_thread::sleep_for(tickInterval_);
    }
}

void RuntimeController::processTick()
{
    ++stats_.epoch;
    stats_.lastTickTime = std::chrono::steady_clock::now();

    // Poll action executor
    executor_.poll();

    // Process place types (timeouts, conditions, etc.)
    processPlaceTypes();

    // Fire enabled transitions
    processTransitions();
}

void RuntimeController::processPlaceTypes()
{
    for (auto& [id, placeType] : placeTypes_)
    {
        placeType->tick(stats_.epoch);
    }
}

void RuntimeController::processTransitions()
{
    // Get transitions by priority and fire enabled ones
    auto transitions = net_.getTransitionsByPriority();

    for (auto* trans : transitions)
    {
        if (net_.isEnabled(*trans))
        {
            // Track output places before firing
            std::vector<std::pair<std::string, core::Subplace>> outputPlaces;
            for (const auto& arc : trans->outputArcs())
            {
                auto [place, sub] = net_.resolvePlace(arc.placeId());
                if (place)
                {
                    outputPlaces.emplace_back(place->id(), sub);
                }
            }

            auto result = net_.fire(*trans, stats_.epoch);
            if (result.success)
            {
                ++stats_.transitionsFired;
                log("Fired transition: " + trans->id());

                if (onTransitionFired_)
                {
                    onTransitionFired_(trans->id(), stats_.epoch);
                }

                // Process tokens that entered places via PlaceTypes
                processNewTokensAtPlaces(outputPlaces);
            }
        }
    }
}

void RuntimeController::processNewTokensAtPlaces(
    const std::vector<std::pair<std::string, core::Subplace>>& places)
{
    for (const auto& [placeId, sub] : places)
    {
        auto* place = net_.getPlace(placeId);
        if (!place)
        {
            continue;
        }

        auto it = placeTypes_.find(placeId);
        if (it == placeTypes_.end())
        {
            continue;
        }

        // Get the queue where tokens landed
        core::TokenQueue* queue = nullptr;
        if (sub != core::Subplace::None && place->hasSubplaces())
        {
            queue = &place->subplace(sub);
        }
        else
        {
            queue = &place->tokens();
        }

        // Process each new token (for non-subplace destinations)
        // Only call onTokenEnter for the main queue, not subplaces
        if (sub == core::Subplace::None)
        {
            while (queue->availableCount() > 0)
            {
                auto tokenPair = queue->pop();
                if (tokenPair.has_value())
                {
                    it->second->onTokenEnter(std::move(tokenPair->second));
                }
            }
        }
    }
}

void RuntimeController::log(const std::string& message)
{
    if (logCallback_)
    {
        logCallback_(message);
    }
}

bool RuntimeController::createNetFromConfig(const config::NetConfig& config)
{
    // Create places first - add to net before creating PlaceTypes
    // so that the reference in PlaceType remains valid
    for (const auto& placeConfig : config.places)
    {
        auto place = std::make_unique<core::Place>(placeConfig.id);
        net_.addPlace(std::move(place));
    }

    // Now create PlaceTypes using the places stored in the net
    for (const auto& placeConfig : config.places)
    {
        auto* place = net_.getPlace(placeConfig.id);
        if (place)
        {
            createPlaceType(placeConfig, *place);
        }
    }

    // Create transitions
    int transIndex = 0;
    for (const auto& transConfig : config.transitions)
    {
        std::string transId = "t" + std::to_string(++transIndex);

        core::Transition trans(transId);

        if (transConfig.priority.has_value())
        {
            trans.setPriority(*transConfig.priority);
        }

        // Add input arcs
        for (const auto& fromPlace : transConfig.from)
        {
            core::Arc arc(fromPlace, transId, core::ArcDirection::PlaceToTransition);
            trans.addInputArc(std::move(arc));
        }

        // Add output arcs
        for (const auto& toArc : transConfig.to)
        {
            core::Arc arc(toArc.to, transId, core::ArcDirection::TransitionToPlace);
            if (toArc.tokenFilter.has_value())
            {
                arc.setTokenFilter(*toArc.tokenFilter);
            }
            trans.addOutputArc(std::move(arc));
        }

        net_.addTransition(std::move(trans));
    }

    return true;
}

void RuntimeController::createPlaceType(const config::PlaceConfig& placeConfig, core::Place& place)
{
    std::unique_ptr<places::PlaceType> placeType;

    switch (placeConfig.type)
    {
        case config::PlaceType::Entrypoint:
        {
            placeType = std::make_unique<places::EntrypointPlace>(place);
            break;
        }

        case config::PlaceType::ResourcePool:
        {
            const auto& params = std::get<config::ResourcePoolParams>(placeConfig.params);
            placeType = std::make_unique<places::ResourcePoolPlace>(place, params.initialAvailability);
            break;
        }

        case config::PlaceType::WaitWithTimeout:
        {
            const auto& params = std::get<config::WaitWithTimeoutParams>(placeConfig.params);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(params.timeout);
            placeType = std::make_unique<places::WaitWithTimeoutPlace>(place, duration);
            break;
        }

        case config::PlaceType::Action:
        {
            const auto& params = std::get<config::ActionPlaceParams>(placeConfig.params);

            places::ActionConfig actionConfig;
            actionConfig.actionName = params.actionId;
            actionConfig.retryPolicy.maxRetries = params.retries;
            actionConfig.retryPolicy.timeout = std::chrono::duration_cast<std::chrono::milliseconds>(params.timeoutPerTry);

            auto actionPlace = std::make_unique<places::ActionPlace>(place, actionConfig, executor_);

            // Set invoker if registered
            auto it = actionInvokers_.find(params.actionId);
            if (it != actionInvokers_.end())
            {
                actionPlace->setInvoker(it->second);
            }

            placeType = std::move(actionPlace);
            break;
        }

        case config::PlaceType::ExitLogger:
        {
            auto exitPlace = std::make_unique<places::ExitLoggerPlace>(place);
            exitPlace->setLogger([this](const std::string& placeId, const Token& token)
            {
                log("Token exited at " + placeId);
                if (onTokenExit_)
                {
                    onTokenExit_(placeId, token);
                }
            });
            placeType = std::move(exitPlace);
            break;
        }

        case config::PlaceType::Plain:
        default:
        {
            placeType = std::make_unique<places::PlainPlace>(place);
            break;
        }
    }

    placeTypes_[placeConfig.id] = std::move(placeType);
}

} // namespace bnet::runtime
