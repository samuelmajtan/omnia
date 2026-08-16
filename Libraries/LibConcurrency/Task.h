/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cassert>
#include <coroutine>
#include <functional>
#include <optional>
#include <utility>

#include <Common/Noncopyable.h>
#include <LibConcurrency/Log.h>

namespace Concurrency {

template<typename T>
class TaskPromise {
public:
    void return_value(T&& value)
    {
        m_result = std::move(value);
    }

    auto result() const -> T const&
    {
        assert(m_result.has_value() && "Result is not available.");
        return m_result.value();
    }
private:
    std::optional<T> m_result;
};

template<>
class TaskPromise<void> {
public:
    void return_void()
    {
    }

    void result() const
    {
    }
};

template<typename T = void>
class Task {
    OA_MAKE_NONCOPYABLE(Task);

public:
    class Promise : public TaskPromise<T> {
    public:
        auto get_return_object() -> Task
        {
            return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        auto initial_suspend() -> std::suspend_always
        {
            return {};
        }

        auto final_suspend() noexcept
        {
            struct FinalAwaiter {
                bool await_ready() const noexcept { return false; }
                auto await_suspend(std::coroutine_handle<Promise> handle) const noexcept -> std::coroutine_handle<>
                {
                    auto continuation = handle.promise().m_continuation;
                    return continuation ? continuation : std::noop_coroutine();
                }
                void await_resume() const noexcept { }
            };
            return FinalAwaiter {};
        }

        void unhandled_exception()
        {
            OA_LOG_FATAL(Log::Scheduler, "A coroutine terminated with an unhandled exception, aborting");
            std::terminate();
        }

    private:
        std::coroutine_handle<> m_continuation;

        friend class Task;
    };

    using promise_type = Promise;

    explicit Task(std::coroutine_handle<promise_type> handle)
        : m_handle(handle)
    {
    }

    ~Task()
    {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    void resume()
    {
        assert(m_handle && "Task is not valid.");
        m_handle.resume();
    }

    auto result() const
    {
        assert(m_handle && "Task is not valid.");
        return m_handle.promise().result();
    }

    auto done() const noexcept -> bool
    {
        assert(m_handle && "Task is not valid.");
        return m_handle.done();
    }

    Task(Task&& other) noexcept
        : m_handle(std::exchange(other.m_handle, {}))
    {
    }

    auto operator=(Task&& other) noexcept -> Task const&
    {
        if (this != &other) {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = std::exchange(other.m_handle, {});
        }
        return *this;
    }

    auto await_ready() const noexcept -> bool
    {
        return false;
    }

    auto await_suspend(std::coroutine_handle<> handle) noexcept -> std::coroutine_handle<>
    {
        m_handle.promise().m_continuation = handle;
        return m_handle;
    }

    auto await_resume() const noexcept -> T
    {
        return m_handle.promise().result();
    }
private:
    std::coroutine_handle<promise_type> m_handle;
};

}
