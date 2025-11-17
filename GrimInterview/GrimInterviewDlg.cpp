
// GrimInterviewDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "GrimInterview.h"
#include "GrimInterviewDlg.h"

#include <thread>

#include "afxdialogex.h"
#include "RandomTask.h"
#include "Threadpool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGrimInterviewDlg dialog



CGrimInterviewDlg::CGrimInterviewDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GRIMINTERVIEW_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGrimInterviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGrimInterviewDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_CREATE_IMAGE, &CGrimInterviewDlg::OnBnClickedCreateImage)
	ON_BN_CLICKED(ID_ACTION, &CGrimInterviewDlg::OnBnClickedAction)
	ON_MESSAGE(WM_UPDATE_DISPLAY, &OnUpdateDisplay)
END_MESSAGE_MAP()

// CGrimInterviewDlg message handlers

BOOL CGrimInterviewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	CEdit* pCircleLine = (CEdit*)GetDlgItem(IDC_EDIT_CIRCLE_LINE);
	pCircleLine->SetWindowTextW(_T("3"));

	CEdit* pRadius = (CEdit*)GetDlgItem(IDC_EDIT_RADIUS);
	pRadius->SetWindowTextW(_T("10"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGrimInterviewDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGrimInterviewDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGrimInterviewDlg::OnBnClickedCreateImage()
{
	const int nWidth = 640;
	const int nHeight = 480;
	const int nBpp = 8;
	bool bRet = m_image.Create(nWidth, nHeight, nBpp);
	m_image.GetWidth();
	m_image.GetHeight();
	m_image.GetBPP();
	if (nBpp == 8)
	{
		static RGBQUAD palette[256];
		for (int i = 0; i < 256; i++)
		{
			palette[i].rgbRed = i;
			palette[i].rgbGreen = i;
			palette[i].rgbBlue = i;
			palette[i].rgbReserved = 0;
		}

		m_image.SetColorTable(0, 256, palette);
	}

	auto fm = (unsigned  char*)m_image.GetBits();
	const int nPitch = m_image.GetPitch();
	for (int j = 0; j < nHeight; j++)
	{
		for (int i = 0; i < nWidth; i++)
			fm[j * nPitch + i] = 0xff;
	}

	CClientDC dc(this);
	m_image.Draw(dc.m_hDC, 0, 0);
}

void CGrimInterviewDlg::drawCircle(unsigned char* fm, const int x, const int y, const int nRadius, const unsigned char nGray)
{
	const int nCenterX = x + nRadius;
	const int nCenterY = y + nRadius;
	const int nPitch = m_image.GetPitch();
	for (int j = y; j < y + nRadius* 2; j++)
	{
		for (int i =x; i < x + nRadius* 2; i++)
		{
			if (IsInCircle(i, j, nCenterX, nCenterY, nRadius))
				fm[j * nPitch + i] = nGray;
		}
	}
}

void CGrimInterviewDlg::UpdateDisplay()
{
	CClientDC dc(this);
	m_image.Draw(dc.m_hDC, 0, 0);
}

void CGrimInterviewDlg::MoveCircle()
{
	static int nSttX = 0;
	static int nStty = 0;
	const int nGray = 80;
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	const int nPitch = m_image.GetPitch();
	const int nBPP = m_image.GetBPP();
	unsigned char* fm = static_cast<unsigned char*>(m_image.GetBits());
	if (fm != nullptr)
	{
		for (int j = 0; j < nHeight; j++)
		{
			for (int i = 0; i < nWidth; i++)
			{
				fm[j * nPitch + i] = 0xff;
			}
		}

		const int nRadius = 10;
		drawCircle(fm, nSttX, nStty, nRadius, nGray);
		++nSttX;
		++nStty;
	}
	
	PostMessage(WM_UPDATE_DISPLAY);
}

bool CGrimInterviewDlg::IsInCircle(const int i, const int j, const int centerX, const int centerY,
	const int nRadius) const
{
	bool bRet = false;
	const double dx = i - centerX;
	const double dy = j - centerY;
	const double distance = dx * dx + dy * dy;
	if (distance < nRadius * nRadius)
		bRet = true;

	return bRet;
}

bool CGrimInterviewDlg::IsInImage(const int i, const int j) const
{
	bool bRet = false;
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	if (i >= 0 && i < nWidth && j >= 0 && j < nHeight)
		bRet = true;

	return bRet;
}

LRESULT CGrimInterviewDlg::OnUpdateDisplay(WPARAM, LPARAM)
{
	UpdateDisplay();
	return 0;
}

void CGrimInterviewDlg::OnBnClickedAction()
{
	auto move = [this]
		{
			while (true)
			{
				MoveCircle();
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
			}

		};

	auto threadpool = Threadpool::GetInstance();
	auto task = std::make_shared<RandomTask>(move);
	threadpool->AddWork(task);
}
