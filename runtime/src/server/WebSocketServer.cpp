// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/server/WebSocketServer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <regex>
#include <sstream>

// SHA-1 implementation for WebSocket handshake
// Based on RFC 3174

namespace bnet::server {

namespace {

// Simple SHA-1 implementation for WebSocket handshake
class SHA1
{
public:
    static constexpr size_t kDigestSize = 20;

    void update(const unsigned char* data, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
        {
            buffer_[bufferLen_++] = data[i];
            if (bufferLen_ == 64)
            {
                processBlock();
                totalLen_ += 512;
                bufferLen_ = 0;
            }
        }
    }

    void finalize(unsigned char* digest)
    {
        totalLen_ += bufferLen_ * 8;

        // Padding
        buffer_[bufferLen_++] = 0x80;
        if (bufferLen_ > 56)
        {
            while (bufferLen_ < 64)
                buffer_[bufferLen_++] = 0;
            processBlock();
            bufferLen_ = 0;
        }
        while (bufferLen_ < 56)
            buffer_[bufferLen_++] = 0;

        // Length in bits (big endian)
        for (int i = 7; i >= 0; --i)
            buffer_[bufferLen_++] = static_cast<unsigned char>((totalLen_ >> (i * 8)) & 0xFF);

        processBlock();

        // Output
        for (int i = 0; i < 5; ++i)
        {
            digest[i * 4 + 0] = static_cast<unsigned char>((h_[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<unsigned char>((h_[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<unsigned char>((h_[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<unsigned char>(h_[i] & 0xFF);
        }
    }

private:
    void processBlock()
    {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (static_cast<uint32_t>(buffer_[i * 4]) << 24) |
                   (static_cast<uint32_t>(buffer_[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(buffer_[i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(buffer_[i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i)
        {
            w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3], e = h_[4];

        for (int i = 0; i < 80; ++i)
        {
            uint32_t f, k;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rotl(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }

        h_[0] += a;
        h_[1] += b;
        h_[2] += c;
        h_[3] += d;
        h_[4] += e;
    }

    static uint32_t rotl(uint32_t x, int n)
    {
        return (x << n) | (x >> (32 - n));
    }

    uint32_t h_[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    unsigned char buffer_[64] = {};
    size_t bufferLen_ = 0;
    uint64_t totalLen_ = 0;
};

// Base64 encoding table
constexpr char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64Encode(const unsigned char* data, size_t len)
{
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3)
    {
        unsigned int n = static_cast<unsigned int>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<unsigned int>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<unsigned int>(data[i + 2]);

        result.push_back(kBase64Chars[(n >> 18) & 0x3F]);
        result.push_back(kBase64Chars[(n >> 12) & 0x3F]);
        result.push_back((i + 1 < len) ? kBase64Chars[(n >> 6) & 0x3F] : '=');
        result.push_back((i + 2 < len) ? kBase64Chars[n & 0x3F] : '=');
    }

    return result;
}

} // namespace

WebSocketServer::WebSocketServer(runtime::RuntimeController& runtime, uint16_t port)
    : runtime_(runtime)
    , port_(port)
{
    setupRuntimeCallbacks();
}

WebSocketServer::~WebSocketServer()
{
    stop();
}

void WebSocketServer::start()
{
    if (running_)
    {
        return;
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        throw std::runtime_error("Failed to create server socket");
    }

    // Allow reuse of address
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
    }

    if (listen(serverSocket_, 10) < 0)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }

    running_ = true;
    serverThread_ = std::thread([this]() { serverLoop(); });
}

void WebSocketServer::stop()
{
    if (!running_)
    {
        return;
    }

    running_ = false;

    // Close server socket to unblock accept()
    if (serverSocket_ >= 0)
    {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : clients_)
        {
            if (client->socket >= 0)
            {
                close(client->socket);
                client->socket = -1;
            }
        }
        clients_.clear();
    }

    if (serverThread_.joinable())
    {
        serverThread_.join();
    }
}

std::size_t WebSocketServer::clientCount() const
{
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
}

void WebSocketServer::broadcast(const nlohmann::json& message)
{
    std::string payload = message.dump();
    std::lock_guard<std::mutex> lock(clientsMutex_);

    for (auto& client : clients_)
    {
        if (client->connected && client->socket >= 0)
        {
            sendWebSocketFrame(client->socket, payload);
        }
    }
}

void WebSocketServer::serverLoop()
{
    fd_set readfds;
    timeval timeout{};

    while (running_)
    {
        FD_ZERO(&readfds);
        FD_SET(serverSocket_, &readfds);

        int maxfd = serverSocket_;

        // Add client sockets
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            for (const auto& client : clients_)
            {
                if (client->socket >= 0)
                {
                    FD_SET(client->socket, &readfds);
                    maxfd = std::max(maxfd, client->socket);
                }
            }
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &timeout);

        if (activity < 0 && errno != EINTR)
        {
            break;
        }

        if (!running_)
        {
            break;
        }

        // Check for new connections
        if (FD_ISSET(serverSocket_, &readfds))
        {
            acceptConnection();
        }

        // Check for client data
        std::vector<int> toRemove;
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            for (auto& client : clients_)
            {
                if (client->socket >= 0 && FD_ISSET(client->socket, &readfds))
                {
                    std::string frame = readWebSocketFrame(client->socket);
                    if (frame.empty())
                    {
                        toRemove.push_back(client->socket);
                    }
                    else
                    {
                        processClientMessage(*client, frame);
                    }
                }
            }
        }

        for (int sock : toRemove)
        {
            removeClient(sock);
        }
    }
}

void WebSocketServer::acceptConnection()
{
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSocket < 0)
    {
        return;
    }

    // Read HTTP upgrade request
    std::array<char, 4096> buffer{};
    ssize_t bytesRead = recv(clientSocket, buffer.data(), buffer.size() - 1, 0);
    if (bytesRead <= 0)
    {
        close(clientSocket);
        return;
    }

    std::string request(buffer.data(), bytesRead);

    if (!performHandshake(clientSocket, request))
    {
        close(clientSocket);
        return;
    }

    auto client = std::make_shared<ClientConnection>();
    client->socket = clientSocket;
    client->connected = true;

    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_.push_back(client);
    }

    // Send config and initial state to new client
    sendConfigToClient(clientSocket);
    sendStateSnapshot(clientSocket);
}

bool WebSocketServer::performHandshake(int socket, const std::string& request)
{
    // Extract Sec-WebSocket-Key
    std::regex keyRegex("Sec-WebSocket-Key: ([^\r\n]+)");
    std::smatch match;

    if (!std::regex_search(request, match, keyRegex))
    {
        return false;
    }

    std::string clientKey = match[1].str();
    std::string acceptKey = computeAcceptKey(clientKey);

    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << acceptKey << "\r\n"
             << "\r\n";

    std::string responseStr = response.str();
    send(socket, responseStr.c_str(), responseStr.size(), 0);

    return true;
}

std::string WebSocketServer::computeAcceptKey(const std::string& key)
{
    static const std::string kMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string combined = key + kMagicString;

    SHA1 sha1;
    sha1.update(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size());

    unsigned char hash[SHA1::kDigestSize];
    sha1.finalize(hash);

    return base64Encode(hash, SHA1::kDigestSize);
}

std::string WebSocketServer::readWebSocketFrame(int socket)
{
    std::array<unsigned char, 2> header{};
    ssize_t bytesRead = recv(socket, header.data(), 2, 0);
    if (bytesRead != 2)
    {
        return "";
    }

    // bool fin = (header[0] & 0x80) != 0;  // Not used for simple frames
    int opcode = header[0] & 0x0F;

    // Connection close
    if (opcode == 0x08)
    {
        return "";
    }

    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    if (payloadLen == 126)
    {
        std::array<unsigned char, 2> extLen{};
        recv(socket, extLen.data(), 2, 0);
        payloadLen = (static_cast<uint64_t>(extLen[0]) << 8) | extLen[1];
    }
    else if (payloadLen == 127)
    {
        std::array<unsigned char, 8> extLen{};
        recv(socket, extLen.data(), 8, 0);
        payloadLen = 0;
        for (int i = 0; i < 8; ++i)
        {
            payloadLen = (payloadLen << 8) | extLen[i];
        }
    }

    std::array<unsigned char, 4> mask{};
    if (masked)
    {
        recv(socket, mask.data(), 4, 0);
    }

    std::string payload(payloadLen, '\0');
    size_t totalRead = 0;
    while (totalRead < payloadLen)
    {
        bytesRead = recv(socket, &payload[totalRead], payloadLen - totalRead, 0);
        if (bytesRead <= 0)
        {
            return "";
        }
        totalRead += bytesRead;
    }

    // Unmask
    if (masked)
    {
        for (size_t i = 0; i < payloadLen; ++i)
        {
            payload[i] ^= mask[i % 4];
        }
    }

    return payload;
}

void WebSocketServer::sendWebSocketFrame(int socket, const std::string& payload)
{
    std::vector<unsigned char> frame;

    // FIN + text opcode
    frame.push_back(0x81);

    // Payload length (not masked from server)
    if (payload.size() <= 125)
    {
        frame.push_back(static_cast<unsigned char>(payload.size()));
    }
    else if (payload.size() <= 65535)
    {
        frame.push_back(126);
        frame.push_back(static_cast<unsigned char>((payload.size() >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(payload.size() & 0xFF));
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i)
        {
            frame.push_back(static_cast<unsigned char>((payload.size() >> (i * 8)) & 0xFF));
        }
    }

    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());

    send(socket, frame.data(), frame.size(), 0);
}

void WebSocketServer::processClientMessage(ClientConnection& client, const std::string& message)
{
    try
    {
        auto json = nlohmann::json::parse(message);
        std::string type = json.value("type", "");

        if (type == "inject_token")
        {
            handleInjectToken(json["payload"]);
        }
        else if (type == "query_place")
        {
            handleQueryPlace(client.socket, json["payload"]);
        }
        else if (type == "request_state")
        {
            handleRequestState(client.socket);
        }
    }
    catch (const std::exception& e)
    {
        // Ignore malformed messages
    }
}

void WebSocketServer::removeClient(int socket)
{
    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_.erase(
        std::remove_if(clients_.begin(), clients_.end(),
            [socket](const std::shared_ptr<ClientConnection>& c)
            {
                if (c->socket == socket)
                {
                    close(c->socket);
                    return true;
                }
                return false;
            }),
        clients_.end());
}

void WebSocketServer::handleInjectToken(const nlohmann::json& payload)
{
    std::string entrypointId = payload.value("entrypointId", "");
    if (entrypointId.empty())
    {
        return;
    }

    Token token;
    if (payload.contains("data"))
    {
        token.data() = payload["data"];
    }

    runtime_.injectToken(entrypointId, std::move(token));
}

void WebSocketServer::handleQueryPlace(int clientSocket, const nlohmann::json& payload)
{
    std::string placeId = payload.value("placeId", "");
    if (placeId.empty())
    {
        return;
    }

    auto tokens = runtime_.getPlaceTokens(placeId);

    nlohmann::json response;
    response["type"] = "place_tokens";
    response["payload"]["placeId"] = placeId;
    response["payload"]["tokens"] = nlohmann::json::array();

    for (const auto& [id, data] : tokens)
    {
        nlohmann::json tokenInfo;
        tokenInfo["id"] = id;
        tokenInfo["data"] = data;
        response["payload"]["tokens"].push_back(tokenInfo);
    }

    sendWebSocketFrame(clientSocket, response.dump());
}

void WebSocketServer::handleRequestState(int clientSocket)
{
    sendStateSnapshot(clientSocket);
}

void WebSocketServer::setupRuntimeCallbacks()
{
    runtime_.setOnTokenEnter([this](const std::string& placeId, const Token& token)
    {
        onTokenEnter(placeId, token);
    });

    runtime_.setOnTokenExit([this](const std::string& placeId, const Token& token)
    {
        onTokenExit(placeId, token);
    });

    runtime_.setOnTransitionFired([this](const std::string& transitionId, std::uint64_t epoch)
    {
        onTransitionFired(transitionId, epoch);
    });
}

void WebSocketServer::onTokenEnter(const std::string& placeId, const Token& token)
{
    nlohmann::json message;
    message["type"] = "token_entered";
    message["payload"]["placeId"] = placeId;
    message["payload"]["token"]["data"] = token.data();
    broadcast(message);
}

void WebSocketServer::onTokenExit(const std::string& placeId, const Token& token)
{
    nlohmann::json message;
    message["type"] = "token_exited";
    message["payload"]["placeId"] = placeId;
    broadcast(message);
}

void WebSocketServer::onTransitionFired(const std::string& transitionId, std::uint64_t epoch)
{
    nlohmann::json message;
    message["type"] = "transition_fired";
    message["payload"]["transitionId"] = transitionId;
    message["payload"]["epoch"] = epoch;
    broadcast(message);
}

void WebSocketServer::sendConfigToClient(int clientSocket)
{
    nlohmann::json message;
    message["type"] = "config";
    message["payload"] = configToJson();
    sendWebSocketFrame(clientSocket, message.dump());
}

void WebSocketServer::sendStateSnapshot(int clientSocket)
{
    nlohmann::json message;
    message["type"] = "state_snapshot";
    message["payload"] = stateToJson();
    sendWebSocketFrame(clientSocket, message.dump());
}

nlohmann::json WebSocketServer::configToJson() const
{
    const auto& config = runtime_.getNetConfig();
    nlohmann::json json;

    // Convert actors
    json["actors"] = nlohmann::json::array();
    for (const auto& actor : config.actors)
    {
        nlohmann::json actorJson;
        actorJson["id"] = actor.id;
        json["actors"].push_back(actorJson);
    }

    // Convert actions
    json["actions"] = nlohmann::json::array();
    for (const auto& action : config.actions)
    {
        nlohmann::json actionJson;
        actionJson["id"] = action.id;
        actionJson["requiredActors"] = action.requiredActors;
        json["actions"].push_back(actionJson);
    }

    // Convert places
    json["places"] = nlohmann::json::array();
    for (const auto& place : config.places)
    {
        nlohmann::json placeJson;
        placeJson["id"] = place.id;

        switch (place.type)
        {
            case config::PlaceType::Plain:
                placeJson["type"] = "plain";
                break;
            case config::PlaceType::Entrypoint:
                placeJson["type"] = "entrypoint";
                break;
            case config::PlaceType::ResourcePool:
                placeJson["type"] = "resourcePool";
                break;
            case config::PlaceType::WaitWithTimeout:
                placeJson["type"] = "waitWithTimeout";
                break;
            case config::PlaceType::Action:
                placeJson["type"] = "action";
                if (auto* params = std::get_if<config::ActionPlaceParams>(&place.params))
                {
                    placeJson["params"]["actionId"] = params->actionId;
                }
                break;
            case config::PlaceType::ExitLogger:
                placeJson["type"] = "exitLogger";
                break;
        }

        json["places"].push_back(placeJson);
    }

    // Convert transitions
    json["transitions"] = nlohmann::json::array();
    for (const auto& trans : config.transitions)
    {
        nlohmann::json transJson;
        transJson["from"] = trans.from;
        transJson["to"] = nlohmann::json::array();
        for (const auto& arc : trans.to)
        {
            nlohmann::json arcJson;
            arcJson["to"] = arc.to;
            if (arc.tokenFilter.has_value())
            {
                arcJson["tokenFilter"] = *arc.tokenFilter;
            }
            transJson["to"].push_back(arcJson);
        }
        if (trans.priority.has_value())
        {
            transJson["priority"] = *trans.priority;
        }
        json["transitions"].push_back(transJson);
    }

    // Include GUI metadata if present
    if (!config.guiMetadata.is_null())
    {
        json["guiMetadata"] = config.guiMetadata;
    }

    return json;
}

nlohmann::json WebSocketServer::stateToJson() const
{
    nlohmann::json json;

    // Get stats
    auto stats = runtime_.stats();
    json["stats"]["epoch"] = stats.epoch;
    json["stats"]["transitionsFired"] = stats.transitionsFired;
    json["stats"]["tokensProcessed"] = stats.tokensProcessed;
    json["stats"]["activeTokens"] = stats.activeTokens;

    // Get tokens per place
    json["places"] = nlohmann::json::object();
    const auto& config = runtime_.getNetConfig();
    for (const auto& placeConfig : config.places)
    {
        auto tokens = runtime_.getPlaceTokens(placeConfig.id);
        nlohmann::json placeState;
        placeState["tokens"] = nlohmann::json::array();
        for (const auto& [id, data] : tokens)
        {
            nlohmann::json tokenInfo;
            tokenInfo["id"] = id;
            tokenInfo["data"] = data;
            placeState["tokens"].push_back(tokenInfo);
        }
        json["places"][placeConfig.id] = placeState;
    }

    return json;
}

} // namespace bnet::server
