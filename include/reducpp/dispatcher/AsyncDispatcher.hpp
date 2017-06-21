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

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace reducpp {

template <typename Store>
class AsyncDispatcher {
public:
    using StoreType = Store;
    using ActionType = typename StoreType::ActionType;

    AsyncDispatcher(StoreType& store) :
        store_(store),
        keepRunning_(true),
        executor_(&AsyncDispatcher::execFunction, this)
    {}

    ~AsyncDispatcher()
    {
        keepRunning_ = false;
        queueCv_.notify_all();
        executor_.join();
    }

    void dispatch(ActionType action)
    {
        std::lock_guard<std::mutex> _(queueMutex_);
        actionQueue_.push_back(std::move(action));
        queueCv_.notify_all();
    }
private:
    void execFunction()
    {
        while (keepRunning_) {
            std::unique_lock<std::mutex> l(queueMutex_);
            queueCv_.wait(l, [this]() -> bool { return !keepRunning_ || !actionQueue_.empty();});
            if (!keepRunning_) {
                break;
            }
            ActionType action = std::move(actionQueue_.front());
            actionQueue_.pop_front();
            l.unlock();

            store_.performDispatch_(std::move(action));
        }
    }

    StoreType& store_;
    std::deque<ActionType> actionQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::atomic<bool> keepRunning_;
    std::thread executor_;
};

} // namespace reducpp
