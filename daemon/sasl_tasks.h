/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#pragma once

#include <cbsasl/server.h>
#include <platform/sized_buffer.h>
#include <string>
#include "task.h"

class Connection;
class Cookie;

/**
 * The SaslAuthTask is the abstract base class used during SASL
 * authentication (which is being run by the executor service)
 */
class SaslAuthTask : public Task {
public:
    SaslAuthTask() = delete;

    SaslAuthTask(const SaslAuthTask&) = delete;

    SaslAuthTask(Cookie& cookie_,
                 Connection& connection_,
                 const std::string& mechanism_,
                 const std::string& challenge_);

    void notifyExecutionComplete() override;

    cb::sasl::Error getError() const {
        return response.first;
    }

    cb::const_char_buffer getResponse() const {
        return response.second;
    }

protected:
    Cookie& cookie;
    Connection& connection;
    std::string mechanism;
    std::string challenge;
    std::pair<cb::sasl::Error, cb::const_char_buffer> response{
            cb::sasl::Error::FAIL, {}};
};
