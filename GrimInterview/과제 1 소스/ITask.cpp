#include "pch.h"
#include "ITask.h"

void ITask::LogThreadId()
{
	DWORD dwCurThreadId = GetCurrentThreadId();
	CString strLog;
	strLog.Format(_T("Task Thread Id is %d\n"), dwCurThreadId);
}
