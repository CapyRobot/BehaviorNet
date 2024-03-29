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

#pragma once

#include <3rd_party/taskflow/taskflow.hpp>
#include <behavior_net/Types.hpp>
#include <utils/Logger.hpp>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>

namespace capybot
{
namespace bnet
{

/**
 * @brief Prototype as a simple thread pool for executing async actions.
 *
 */
class ThreadPool
{
    static constexpr const char* MODULE_TAG{"ThreadPool"};

public:
    /**
     * @brief Task element to be executed by the thread pool.
     *
     */
    class Task
    {
        static constexpr const char* MODULE_TAG{"ThreadPool::Task"};

    public:
        Task(std::function<ActionExecutionStatus()> func)
            : m_func(func)
            , m_return(ActionExecutionStatus::NOT_STARTED)
            , m_started(false)
            , m_done(false)
        {
        }

        /// Execute task synchronously - this call will block
        void executeSync()
        {
            {
                std::lock_guard<std::mutex> lk(m_mtx); // probably unnecessary
                m_started = true;
                m_done = false;
            }
            try
            {
                m_return = m_func();
            }
            catch (std::exception& e)
            {
                LOG(ERROR) << "executeSync: std::exception thrown by m_func(). Returning error action status. error = "
                           << e.what() << log::endl;
                m_return = ActionExecutionStatus::ERROR;
            }
            catch (...)
            {
                LOG(ERROR) << "executeSync: unknown exception thrown by m_func(). Returning error action status."
                           << log::endl;
                m_return = ActionExecutionStatus::ERROR;
            }
            {
                std::lock_guard<std::mutex> lk(m_mtx);
                m_done = true;
            }
            m_waitCondition.notify_all();
        }

        /// Get return value after completion
        /// @return action execution status
        ActionExecutionStatus getStatus(uint32_t timeoutUs = 0U) const
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            if (!m_started)
                return ActionExecutionStatus::NOT_STARTED;
            if (m_done)
                return m_return;

            if (timeoutUs)
            {
                if (m_waitCondition.wait_for(lk, std::chrono::microseconds(timeoutUs), [this] { return m_done; }))
                    return m_return;
            }
            return ActionExecutionStatus::QUERRY_TIMEOUT;
        }

    private:
        const std::function<ActionExecutionStatus()> m_func;
        ActionExecutionStatus m_return;
        bool m_started;
        bool m_done;
        mutable std::condition_variable m_waitCondition;
        mutable std::mutex m_mtx;
    };

    ThreadPool(uint32_t numberOfThreads = std::thread::hardware_concurrency())
        : m_executor(numberOfThreads)
    {
    }

    ~ThreadPool()
    {
        m_stopped.store(true);
        LOG(DEBUG) << "[ThreadPool::~ThreadPool] Stopping ThreadPoll; waiting for unfinished tasks ..." << log::endl;
        m_executor.wait_for_all();
        LOG(DEBUG) << "[ThreadPool::~ThreadPool] Stopping ThreadPoll ... done." << log::endl;
    }

    /// @brief add task to the thread pool queue for execution
    void executeAsync(Task& task)
    {
        if (m_stopped.load()) // ignore new tasks while on destruction
        {
            return;
        }
        m_executor.silent_async([&task] { task.executeSync(); });
    }

private:
    std::atomic_bool m_stopped{false};
    tf::Executor m_executor;
};

} // namespace bnet
} // namespace capybot
