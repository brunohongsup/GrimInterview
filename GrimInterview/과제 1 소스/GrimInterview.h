
// GrimInterview.h : main header file for the PROJECT_NAME application
//

#pragma once
#include <afxinet.h>
#include <memory>

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CGrimInterviewApp:
// See GrimInterview.cpp for the implementation of this class
//

class CGrimInterviewApp : public CWinApp
{
public:
	CGrimInterviewApp();

// Overrides
public:
	BOOL InitInstance() override;

	std::shared_ptr<CInternetSession> GetSession() {return m_pSession;}

// Implementation

	DECLARE_MESSAGE_MAP()

private:
	std::shared_ptr<CInternetSession> m_pSession;
};

extern CGrimInterviewApp theApp;
