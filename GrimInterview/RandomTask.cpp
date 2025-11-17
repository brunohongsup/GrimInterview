#include "pch.h"
#include "RandomTask.h"

RandomTask::RandomTask(const std::function<void()>& work)
	: m_work(work)
{
	
}

void RandomTask::Run()
{
	m_work();
}
