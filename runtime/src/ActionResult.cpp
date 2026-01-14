// Copyright 2024 BehaviorNet Authors
// SPDX-License-Identifier: Apache-2.0

#include "bnet/ActionResult.hpp"
#include "bnet/Error.hpp"

namespace bnet {

std::string ActionResult::errorTypeName() const
{
    if (!isError() || !error_) return "";
    return ErrorRegistry::instance().getTypeName(error_);
}

} // namespace bnet
