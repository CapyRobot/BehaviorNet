// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bnet/config/ConfigTypes.hpp>
#include <bnet/runtime/RuntimeController.hpp>

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bnet::server {

/// @brief Client connection state.
struct ClientConnection
{
    int socket{-1};
    bool connected{false};
    std::string readBuffer;
};

/// @brief WebSocket server for GUI integration.
///
/// Provides real-time updates to connected GUI clients and handles
/// commands like token injection and state queries.
class WebSocketServer
{
public:
    /// @brief Construct server with runtime controller.
    /// @param runtime The runtime controller to expose.
    /// @param port The port to listen on (default: 8080).
    WebSocketServer(runtime::RuntimeController& runtime, uint16_t port = 8080);

    ~WebSocketServer();

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    /// @brief Start the server (non-blocking, runs in separate thread).
    void start();

    /// @brief Stop the server and close all connections.
    void stop();

    /// @brief Check if server is running.
    [[nodiscard]] bool isRunning() const { return running_; }

    /// @brief Get the port the server is listening on.
    [[nodiscard]] uint16_t port() const { return port_; }

    /// @brief Get the number of connected clients.
    [[nodiscard]] std::size_t clientCount() const;

    /// @brief Broadcast a message to all connected clients.
    void broadcast(const nlohmann::json& message);

private:
    void serverLoop();
    void acceptConnection();
    void handleClient(int clientSocket);
    void processClientMessage(ClientConnection& client, const std::string& message);
    void removeClient(int socket);

    // WebSocket protocol helpers
    bool performHandshake(int socket, const std::string& request);
    std::string readWebSocketFrame(int socket);
    void sendWebSocketFrame(int socket, const std::string& payload);
    std::string computeAcceptKey(const std::string& key);

    // Message handlers
    void handleInjectToken(const nlohmann::json& payload);
    void handleQueryPlace(int clientSocket, const nlohmann::json& payload);
    void handleRequestState(int clientSocket);

    // Event callbacks for RuntimeController
    void setupRuntimeCallbacks();
    void onTokenEnter(const std::string& placeId, const Token& token);
    void onTokenExit(const std::string& placeId, const Token& token);
    void onTransitionFired(const std::string& transitionId, std::uint64_t epoch);

    // Send config to a newly connected client
    void sendConfigToClient(int clientSocket);
    void sendStateSnapshot(int clientSocket);

    // Convert config to JSON for sending to GUI
    nlohmann::json configToJson() const;
    nlohmann::json stateToJson() const;

    runtime::RuntimeController& runtime_;
    uint16_t port_;
    int serverSocket_{-1};
    std::atomic<bool> running_{false};

    std::thread serverThread_;
    std::vector<std::shared_ptr<ClientConnection>> clients_;
    mutable std::mutex clientsMutex_;
};

} // namespace bnet::server
