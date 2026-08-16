/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibConcurrency/Log.h>
#include <LibConcurrency/ThreadPool.h>

namespace Concurrency {

ThreadPool::ThreadPool(u32 thread_count)
{
    m_workers.reserve(thread_count);
    for (u32 i = 0; i < thread_count; i++) {
        m_workers.emplace_back([this](std::stop_token const& stop_token) {
            worker(stop_token);
        });
    }

    OA_LOG_DEBUG(Log::Threads, "Started {} worker threads", thread_count);
}

ThreadPool::ThreadPool(std::function<void()> thread_init_callback, u32 thread_count)
{
    m_workers.reserve(thread_count);
    for (u32 i = 0; i < thread_count; i++) {
        m_workers.emplace_back([this, callback = thread_init_callback](std::stop_token const& stop_token) {
            callback();
            worker(stop_token);
        });
    }

    OA_LOG_DEBUG(Log::Threads, "Started {} worker threads", thread_count);
}

ThreadPool::~ThreadPool()
{
    OA_LOG_DEBUG(Log::Threads, "Stopping {} worker threads", m_workers.size());

    for (auto& worker : m_workers) {
        worker.request_stop();
    }
    m_thread_pool_cv.notify_all();
}

void ThreadPool::submit(Job job)
{
    {
        std::unique_lock const lock(m_worker_mutex);
        m_job_queue.push(std::move(job));
    }
    m_thread_pool_cv.notify_one();
}

void ThreadPool::worker(std::stop_token const& stop_token)
{
    while (!stop_token.stop_requested()) {
        Job job {};

        {
            std::unique_lock lock(m_worker_mutex);
            m_thread_pool_cv.wait(lock, [&] {
                return !m_job_queue.empty() || stop_token.stop_requested();
            });
            if (stop_token.stop_requested()) {
                OA_LOG_TRACE(Log::Threads, "Worker exiting, stop was requested");
                break;
            }

            job = std::move(m_job_queue.front());
            m_job_queue.pop();
        }
        job();
    }
}

}
