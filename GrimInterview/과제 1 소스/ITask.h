#pragma once

class ITask
{
public:
	virtual void Run() = 0;
	
	virtual void LogThreadId();

	virtual ~ITask() = default;

	ITask() = default;
};