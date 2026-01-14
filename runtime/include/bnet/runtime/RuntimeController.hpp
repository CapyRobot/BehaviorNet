// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/Token.hpp>
#include <bnet/config/ConfigParser.hpp>
#include <bnet/core/Net.hpp>
#include <bnet/core/Place.hpp>
#include <bnet/execution/ActionExecutor.hpp>
#include <bnet/places/PlaceType.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace bnet::runtime {

/// @brief Callback for logging events.
using LogCallback = std::function<void(const std::string& message)>;

/// @brief Callback for token events.
using TokenEventCallback = std::function<void(const std::string& placeId, const Token& token)>;

/// @brief Callback for transition events.
using TransitionEventCallback = std::function<void(const std::string& transitionId, std::uint64_t epoch)>;

/// @brief Current state of the runtime.
enum class RuntimeState
{
    Stopped,
    Starting,
    Running,
    Stopping,
    Error
};

/// @brief Statistics about runtime execution.
struct RuntimeStats
{
    std::uint64_t epoch{0};
    std::uint64_t transitionsFired{0};
    std::uint64_t tokensProcessed{0};
    std::size_t activeTokens{0};
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastTickTime;
};

/// @brief Main orchestrator for BehaviorNet execution.
///
/// Manages the execution loop, token injection, and coordination
/// between the Petri net engine and action execution system.
class RuntimeController
{
public:
    RuntimeController();
    ~RuntimeController();

    RuntimeController(const RuntimeController&) = delete;
    RuntimeController& operator=(const RuntimeController&) = delete;

    /// @brief Load configuration from parsed config.
    bool loadConfig(const config::NetConfig& config);

    /// @brief Load configuration from JSON string.
    bool loadConfigString(const std::string& json);

    /// @brief Load configuration from file.
    bool loadConfigFile(const std::string& path);

    /// @brief Start the execution loop.
    void start();

    /// @brief Stop the execution loop.
    void stop();

    /// @brief Perform a single execution tick.
    void tick();

    /// @brief Inject a token at an entrypoint place.
    /// @return Token ID if successful, 0 otherwise.
    core::TokenId injectToken(const std::string& entrypointId, Token token);

    /// @brief Get current runtime state.
    [[nodiscard]] RuntimeState state() const { return state_; }

    /// @brief Get current statistics.
    [[nodiscard]] RuntimeStats stats() const;

    /// @brief Get the underlying net.
    [[nodiscard]] core::Net& net() { return net_; }
    [[nodiscard]] const core::Net& net() const { return net_; }

    /// @brief Get the action executor.
    [[nodiscard]] execution::ActionExecutor& executor() { return executor_; }

    /// @brief Set tick interval for automatic execution.
    void setTickInterval(std::chrono::milliseconds interval) { tickInterval_ = interval; }

    /// @brief Set log callback.
    void setLogCallback(LogCallback callback) { logCallback_ = std::move(callback); }

    /// @brief Set callback for token entry events.
    void setOnTokenEnter(TokenEventCallback callback) { onTokenEnter_ = std::move(callback); }

    /// @brief Set callback for token exit events.
    void setOnTokenExit(TokenEventCallback callback) { onTokenExit_ = std::move(callback); }

    /// @brief Set callback for transition fired events.
    void setOnTransitionFired(TransitionEventCallback callback) { onTransitionFired_ = std::move(callback); }

    /// @brief Get tokens at a specific place.
    /// @return Vector of token info (id, data) pairs.
    [[nodiscard]] std::vector<std::pair<core::TokenId, nlohmann::json>> getPlaceTokens(const std::string& placeId) const;

    /// @brief Get the loaded net configuration.
    [[nodiscard]] const config::NetConfig& getNetConfig() const { return loadedConfig_; }

    /// @brief Register an action invoker for a given action ID.
    void registerAction(const std::string& actionId, execution::ActionInvoker invoker);

    /// @brief Get list of errors from loading/execution.
    [[nodiscard]] const std::vector<std::string>& errors() const { return errors_; }

private:
    void runLoop();
    void processTick();
    void processEntrypoints();
    void processTransitions();
    void processPlaceTypes();
    void processNewTokensAtPlaces(const std::vector<std::pair<std::string, core::Subplace>>& places);
    void log(const std::string& message);

    bool createNetFromConfig(const config::NetConfig& config);
    void createPlaceType(const config::PlaceConfig& placeConfig, core::Place& place);

    core::Net net_;
    execution::ActionExecutor executor_;

    std::unordered_map<std::string, std::unique_ptr<places::PlaceType>> placeTypes_;
    std::unordered_map<std::string, execution::ActionInvoker> actionInvokers_;

    std::atomic<RuntimeState> state_{RuntimeState::Stopped};
    RuntimeStats stats_;
    std::vector<std::string> errors_;

    std::chrono::milliseconds tickInterval_{10};
    LogCallback logCallback_;
    TokenEventCallback onTokenEnter_;
    TokenEventCallback onTokenExit_;
    TransitionEventCallback onTransitionFired_;
    config::NetConfig loadedConfig_;

    std::thread runThread_;
    std::mutex mutex_;
};

} // namespace bnet::runtime
