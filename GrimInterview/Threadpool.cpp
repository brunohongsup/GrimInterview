#include "pch.h"
#include "Threadpool.h"

#include "RandomTask.h"

std::unique_ptr<Threadpool> Threadpool::s_pInstance = nullptr;
std::mutex Threadpool::s_mtx;

Threadpool* Threadpool::GetInstance()
{
    if (nullptr == s_pInstance)
    {
        std::lock_guard<std::mutex> lock(s_mtx);
        if (nullptr == s_pInstance)
        {
            s_pInstance = std::unique_ptr<Threadpool>(new Threadpool());
            s_pInstance->init();
        }
    }
    
    return s_pInstance.get();
}

Threadpool::Threadpool()
    : m_shutdown(false)
{
}

Threadpool::~Threadpool()
{
    CleanUp();
}

bool Threadpool::init()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    size_t threadCount = std::thread::hardware_concurrency();
    threadCount -= 4;
    if (threadCount == 0)
        threadCount = 4;

    m_workers.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i)
        m_workers.emplace_back([this] { this->workerLoop(); });

    return true;
}

void Threadpool::CleanUp()
{
    m_shutdown = true;
    m_cv.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_tasks.empty())
    {
        auto task = m_tasks.front();
        m_tasks.pop();
    }
    
    m_workers.clear();
}

bool Threadpool::AddWork(const std::shared_ptr<ITask>& pTask)
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_shutdown)
            return FALSE;
        
        m_tasks.push(pTask);
    }
    
    m_cv.notify_one();
    return true;
}

bool Threadpool::AddWork(const std::function<void()>& work)
{
    const auto task = std::make_shared<RandomTask>(work);
	return AddWork(task);
}

void Threadpool::workerLoop()
{
    while (true)
    {
        std::shared_ptr<ITask> pTask = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_cv.wait(lock, [this] { return m_shutdown || !m_tasks.empty(); });
            if (m_shutdown && m_tasks.empty())
                return;

            pTask = m_tasks.front();
            m_tasks.pop();
        }

        pTask->Run();
    }
}
