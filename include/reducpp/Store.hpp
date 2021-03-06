/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2017, Santiago Alessandri
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "dispatcher/SingleThreadDispatcher.hpp"

namespace reducpp {

template <
    typename State,
    typename Action,
    template<typename St> class Dispatcher = SingleThreadDispatcher>
class Store {
public:
    using StateType = State;
    using ActionType = Action;
    using DispatcherType = Dispatcher<Store>;
    using Reducer = std::function<State(StateType, ActionType)>;
    using NextFunction = std::function<void(ActionType)>;
    using Middleware = std::function<void(Store&, NextFunction, ActionType)>;

    friend class Dispatcher<Store>;

    Store(State state, Reducer reducer, std::vector<Middleware> middleware) :
        dispatcher_(*this),
        state_(std::move(state)),
        reducer_(std::move(reducer)),
        middleware_(std::move(middleware))
    {
        NextFunction applyMiddlewareAndReducer = [this](Action action) {
            state_ = reducer_(std::move(state_), std::move(action));
        };

        for (auto it = middleware_.rbegin(); it != middleware_.rend(); ++it) {
            const auto& m = *it;
            applyMiddlewareAndReducer = [this, &m, applyMiddlewareAndReducer](Action action) {
                m(*this, applyMiddlewareAndReducer, std::move(action));
            };
        }

        performDispatch_ = move(applyMiddlewareAndReducer);
    }

    void dispatch(ActionType action)
    {
        dispatcher_.dispatch(std::move(action));
    }

    const StateType& getState() const
    {
        return state_;
    }

private:
    DispatcherType dispatcher_;
    State state_;
    Reducer reducer_;
    std::vector<Middleware> middleware_;

    std::function<void(Action)> performDispatch_;
};

} // namespace reducpp
