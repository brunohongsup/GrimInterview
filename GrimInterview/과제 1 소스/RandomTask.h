#pragma once
#include <functional>
#include "ITask.h"

class RandomTask : public ITask
{ 
public:
	RandomTask(const std::function<void()>& work);

	void Run() override;

	~RandomTask() override  = default;
	
private:
	std::function<void()> m_work;
	
};
