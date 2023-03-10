/*
 * Copyright (C) 2023 Eduardo Rocha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BEHAVIOR_NET_CPP_PLACE_HPP_
#define BEHAVIOR_NET_CPP_PLACE_HPP_

#include "behavior_net/Action.hpp"
#include "behavior_net/Common.hpp"
#include "behavior_net/Token.hpp"

#include <3rd_party/nlohmann/json.hpp>
#include <list>
#include <optional>
#include <unordered_map>

namespace capybot
{
namespace bnet
{

class Place // add factory namespace
{
public:
    using SharedPtr = std::shared_ptr<Place>;
    using IdMap = std::map<std::string, SharedPtr>;

    static IdMap createPlaces(nlohmann::json const& netConfig) // why shared_ptr?
    {
        auto placeConfigs = netConfig.at("places");
        IdMap placePtrs;
        for (auto&& placeConfig : placeConfigs)
        {
            // TODO: ensure no repeated ids
            placePtrs.emplace(placeConfig.at("place_id").get<std::string>(), std::make_shared<Place>(placeConfig));
        }
        return placePtrs;
    }

    static void createActions(ThreadPool& tp, nlohmann::json const actionsConfig, IdMap& places)
    {
        for (auto&& config : actionsConfig)
        {
            places.at(config["place"])
                ->setAssociatedAction(tp, Action::Factory::typeFromStr(config["type"]), config["params"]);
        }
    }

    Place(nlohmann::json config) : m_id(config.at("place_id").get<std::string>()) {}

    void setAssociatedAction(ThreadPool& tp, Action::Factory::ActionType type, nlohmann::json const parameters)
    {
        m_action = Action::Factory::create(tp, type, parameters);
    }

    void insertToken(Token token)
    {
        token.setCurrentPlace(m_id);
        if (isPassive())
        {
            m_tokensAvailable.push_back(token);
            m_tokensAvailableResult.push_back(ACTION_EXEC_STATUS_COMPLETED_SUCCESS);
        }
        else
        {
            m_tokensBusy.push_back(token);
        }
    }

    std::optional<Token> consumeToken(ActionExecutionStatusBitmask resultsAccepted = 0U)
    {
        std::optional<Token> token{};
        if (resultsAccepted)
        {
            auto itRes = m_tokensAvailableResult.begin();
            for (auto it = m_tokensAvailable.begin(); it != m_tokensAvailable.end(); it++, itRes++)
            {
                if (*itRes & resultsAccepted)
                {
                    token = *it;
                    m_tokensAvailable.erase(it);
                    m_tokensAvailableResult.erase(itRes);
                    break;
                }
            }
        }
        else
        {
            token = m_tokensAvailable.front();
            m_tokensAvailable.pop_front();
            m_tokensAvailableResult.pop_front();
        }
        return token;
    }

    void executeActionAsync()
    {
        if (!isPassive())
        {
            m_action->executeAsync(getTokensBusy());
        }
    }

    void checkActionResults()
    {
        if (!isPassive())
        {
            const auto actionResults = m_action->getEpochResults();

            for (auto&& result : actionResults)
            {
                std::list<Token>* dest;
                if (result.status == ACTION_EXEC_STATUS_COMPLETED_SUCCESS ||
                    result.status == ACTION_EXEC_STATUS_COMPLETED_FAILURE ||
                    result.status == ACTION_EXEC_STATUS_COMPLETED_ERROR) // TODO: helper function
                {
                }
                else // action is not done yet
                {
                    continue;
                }

                for (decltype(m_tokensBusy)::iterator it = m_tokensBusy.begin(); it != m_tokensBusy.end(); it++)
                {
                    if (result.tokenId == it->getUniqueId())
                    {
                        auto mv = it++;
                        m_tokensAvailable.splice(m_tokensAvailable.end(), m_tokensBusy, mv);
                        m_tokensAvailableResult.push_back(result.status);
                    }
                }
            }
        }
    }

    bool isPassive() const { return m_action == nullptr; }
    std::string const& getId() const { return m_id; }

    uint32_t getNumberTokensBusy() const { return m_tokensBusy.size(); }
    uint32_t getNumberTokensTotal() const { return m_tokensBusy.size() + m_tokensAvailable.size(); }
    uint32_t getNumberTokensAvailable(ActionExecutionStatusBitmask status = 0U) const
    {
        if (status)
        {
            return std::count_if(m_tokensAvailableResult.begin(), m_tokensAvailableResult.end(),
                                 [&status](ActionExecutionStatus s) { return s & status; });
        }
        return m_tokensAvailable.size();
    }

    std::list<Token> const& getTokensBusy() const { return m_tokensBusy; }

private:
    std::string m_id;
    Action::UniquePtr m_action;

    // TODO: sending references of tokens deep down the action stack is problematic.
    // One erroneous copy somewhere and we end up with a reference to a temporary object
    std::list<Token> m_tokensAvailable; // ready to be consumed
    std::list<ActionExecutionStatus>
        m_tokensAvailableResult; // stores the results associated with available tokens
                                 // TODO: these two should belong to a single private struct

    std::list<Token> m_tokensBusy; // either in action exec or waiting for exec
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_PLACE_HPP_