// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/Error.hpp"

namespace bnet {

void ErrorRegistry::registerBuiltinErrors()
{
    registerError<error::Error>("bnet::error::Error");
    registerError<error::RuntimeError>("bnet::error::RuntimeError");
    registerError<error::NetworkError>("bnet::error::NetworkError");
    registerError<error::TimeoutError>("bnet::error::TimeoutError");
    registerError<error::ConnectionError>("bnet::error::ConnectionError");
    registerError<error::ResourceError>("bnet::error::ResourceError");
    registerError<error::ActorNotFoundError>("bnet::error::ActorNotFoundError");
    registerError<error::ResourceUnavailableError>("bnet::error::ResourceUnavailableError");
    registerError<error::ActionError>("bnet::error::ActionError");
    registerError<error::ActionCancelledError>("bnet::error::ActionCancelledError");
    registerError<error::RetriesExhaustedError>("bnet::error::RetriesExhaustedError");
    registerError<error::ValidationError>("bnet::error::ValidationError");
    registerError<error::ConfigError>("bnet::error::ConfigError");
}

} // namespace bnet
