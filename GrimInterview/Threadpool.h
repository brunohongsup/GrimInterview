#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

#include "ITask.h"

class Threadpool
{
public:
	friend class TaskManager;
	
	friend class CLogMgr;

	static Threadpool* GetInstance();

	virtual ~Threadpool();

	void CleanUp();

	bool AddWork(const std::shared_ptr<ITask>& pTask);

	bool AddWork(const std::function<void()>& work);

private:
	Threadpool();

	bool init();

	static std::unique_ptr<Threadpool> s_pInstance;
	
	static std::mutex s_mtx;

	std::vector<std::thread> m_workers;
	
	std::queue<std::shared_ptr<ITask>> m_tasks;
	
	std::mutex m_queueMutex;
	
	std::condition_variable m_cv;
	
	std::atomic<bool> m_shutdown;

	void workerLoop();
};
