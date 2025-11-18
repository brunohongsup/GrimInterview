
// GrimInterviewDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "GrimInterview.h"
#include "GrimInterviewDlg.h"

#include <cmath>
#include <thread>
#include <random>
#include "afxdialogex.h"
#include "RandomTask.h"
#include "Threadpool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(ID_CLEAR, &CGrimInterviewDlg::OnBnClickedClear)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(ID_RANDOM_MOVE, &CGrimInterviewDlg::OnBnClickedRandomMove)
	ON_BN_CLICKED(ID_CONSECUTIVE_RANDOM_MOVE, &CGrimInterviewDlg::OnBnClickedConsecutiveRandomMove)
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

	CString strData;
	pCircleLine->GetWindowTextW(strData);
	m_nCircleLine = _ttoi(strData);

	pRadius->GetWindowTextW(strData);
	m_nRadius = _ttoi(strData);

	m_nCircleCount = 0;


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

void CGrimInterviewDlg::drawCircle(unsigned char* fm, const int centerX, const int centerY, const int nRadius, const unsigned char nGray)
{
	const int nPitch = m_image.GetPitch();
	for (int j = centerY - nRadius; j < centerY + nRadius; j++)
	{
		for (int i = centerX - nRadius; i < centerX + nRadius; i++)
		{
			if (IsInImage(i, j))
			{
				if (IsInCircle(i, j, centerX, centerY, nRadius))
					fm[j * nPitch + i] = nGray;
			}
		}
	}
}

void CGrimInterviewDlg::DrawCircleWithThreePoints()
{
	auto firstPairCenter = CPoint((m_circleCenters[0].x + m_circleCenters[1].x) / 2, (m_circleCenters[0].y + m_circleCenters[1].y) / 2);
	auto secondPairCenter = CPoint((m_circleCenters[1].x + m_circleCenters[2].x) / 2, (m_circleCenters[1].y + m_circleCenters[2].y) / 2);
	auto firstDirection = CPoint(m_circleCenters[1].y - m_circleCenters[0].y, m_circleCenters[0].x - m_circleCenters[1].x);
	auto secondDirection = CPoint(m_circleCenters[2].y - m_circleCenters[1].y, m_circleCenters[1].x - m_circleCenters[2].x);

	CPoint circleCenter{};
	bool bRet = GetIntersectionWidthDirections(&circleCenter, firstPairCenter, firstDirection, secondPairCenter, secondDirection);
	if (!bRet)
	{
		
	}

	const int dx = circleCenter.x - m_circleCenters[0].x;
	const int dy = circleCenter.y - m_circleCenters[0].y;
	const int radiusSquared = dx * dx + dy * dy;
	const int radius = static_cast<int>(std::sqrt(static_cast<double>(radiusSquared)));
	auto fm = static_cast<unsigned char*>(m_image.GetBits());
	const int nPitch = m_image.GetPitch();
	for (int j = circleCenter.y - radius - 1; j <= circleCenter.y + radius + 2; j++)
	{
		for (int i = circleCenter.x - radius - 1; i <= circleCenter.x + radius + 2; i++)
		{
			if (IsInImage(i, j))
			{
				const int dx = i - circleCenter.x;
				const int dy = j - circleCenter.y;
				const int distanceSquared = dx * dx + dy * dy;
				if (abs(distanceSquared - radiusSquared) <= static_cast<int>(m_nCircleLine) * radius)
					fm[j * nPitch + i] = s_Gray;
			}
		}
	}
}

void CGrimInterviewDlg::clearImage()
{
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	const int nPitch = m_image.GetPitch();
	unsigned char* fm = static_cast<unsigned char*>(m_image.GetBits());

	for (int j = 0; j < nHeight; j++)
	{
		for (int i = 0; i < nWidth; i++)
		{
			fm[j * nPitch + i] = 0xff;
		}
	}
}

void CGrimInterviewDlg::randomMove()
{
	auto fm = (unsigned  char*)m_image.GetBits();
	const int nPitch = m_image.GetPitch();
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	clearImage();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrWidth(0, m_image.GetWidth() - 1);
	std::uniform_int_distribution<> distrHeight(0, m_image.GetHeight() - 1);

	for (int i = 0; i < 3; i++)
	{
		m_circleCenters[i].x = distrWidth(gen);
		m_circleCenters[i].y = distrHeight(gen);
		drawCircle(fm, m_circleCenters[i].x, m_circleCenters[i].y, m_nRadius, s_Gray);
	}

	DrawCircleWithThreePoints();
	UpdateDisplay();
}

void CGrimInterviewDlg::UpdateDisplay()
{
	PostMessage(WM_UPDATE_DISPLAY);
}

void CGrimInterviewDlg::MoveCircle()
{
	static int nSttX = 0;
	static int nStty = 0;
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	const int nPitch = m_image.GetPitch();
	const int nBPP = m_image.GetBPP();
	unsigned char* fm = static_cast<unsigned char*>(m_image.GetBits());
	if (fm != nullptr)
	{
		

		const int nRadius = 10;
		drawCircle(fm, nSttX, nStty, nRadius, s_Gray);
		++nSttX;
		++nStty;
	}

	UpdateDisplay();
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
	if (m_image.IsNull())
		return false;

	bool bRet = false;
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();

	if (i >= 0 && i < nWidth && j >= 0 && j < nHeight)
		bRet = true;

	return bRet;
}

bool CGrimInterviewDlg::GetIntersection(CPoint* pIntersection, const CPoint& p1, const CPoint& p2, const CPoint& q1, const CPoint& direction2) const
{
	const double x1 = p1.x, y1 = p1.y;
	const double x2 = p2.x, y2 = p2.y;
	const double x3 = q1.x, y3 = q1.y;
	const double x4 = direction2.x, y4 = direction2.y;

	const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	if (denom == 0)
		return false;

	const double px = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / denom;
	const double py = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / denom;

	auto inRange = [](double a, double b, double x) {
		return (x >= min(a, b)) && (x <= max(a, b));
	};

	if (inRange(x1, x2, px) && inRange(y1, y2, py) &&
		inRange(x3, x4, px) && inRange(y3, y4, py))
	{
		if (pIntersection)
		{
			pIntersection->x = static_cast<int>(px);
			pIntersection->y = static_cast<int>(py);
		}

		return true;
	}

	else
		return false;
}

bool CGrimInterviewDlg::GetIntersectionWidthDirections(CPoint* pIntersection,
	const CPoint& p1, const CPoint& direction1,
	const CPoint& p2, const CPoint& direction2) const
{
	const double x1 = p1.x;
	const double y1 = p1.y;
	const double dx1 = direction1.x;
	const double dy1 = direction1.y;
	const double x2 = p2.x, y2 = p2.y;
	const double dx2 = direction2.x, dy2 = direction2.y;
	const double denom = dx1 * dy2 - dy1 * dx2;
	if (denom == 0.0) 
		return false;

	const double t1 = ((x2 - x1) * dy2 - (y2 - y1) * dx2) / denom;
	const double px = x1 + t1 * dx1;
	const double py = y1 + t1 * dy1;

	if (pIntersection) 
	{
		pIntersection->x = static_cast<int>(px);
		pIntersection->y = static_cast<int>(py);
	}

	return true;
}

LRESULT CGrimInterviewDlg::OnUpdateDisplay(WPARAM, LPARAM)
{
	CClientDC dc(this);
	m_image.Draw(dc.m_hDC, 0, 0);
	return 0;
}

void CGrimInterviewDlg::OnBnClickedAction()
{
	CString strData;
	CEdit* pCircleLine = (CEdit*)GetDlgItem(IDC_EDIT_CIRCLE_LINE);
	pCircleLine->GetWindowTextW(strData);
	m_nCircleLine = _ttoi(strData);

	CEdit* pRadius = (CEdit*)GetDlgItem(IDC_EDIT_RADIUS);
	pRadius->GetWindowTextW(strData);
	m_nRadius = _ttoi(strData);

	auto move = [this]
	{
			while (true)
			{
				MoveCircle();
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
	};

	auto threadpool = Threadpool::GetInstance();
	threadpool->AddWork(move);
}

void CGrimInterviewDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CString strData;
	CEdit* pCircleLine = (CEdit*)GetDlgItem(IDC_EDIT_CIRCLE_LINE);
	CEdit* pRadius = (CEdit*)GetDlgItem(IDC_EDIT_RADIUS);

	pCircleLine->GetWindowTextW(strData);
	m_nCircleLine = _ttoi(strData);

	pRadius->GetWindowTextW(strData);
	m_nRadius = _ttoi(strData);

	if (IsInImage(point.x, point.y))
	{
		strData.Format(_T("X : %03d Y : %03d"), point.x, point.y);
		CStatic* wnd = (CStatic*)GetDlgItem(IDC_STATIC_COORDINATE);
		wnd->SetWindowTextW(strData);

		auto task = [this, point]
			{
				if (m_nCircleCount < 3)
				{
					auto fm = (unsigned char*)m_image.GetBits();
					drawCircle(fm, point.x, point.y, static_cast<int>(m_nRadius), s_Gray);
					m_circleCenters[m_nCircleCount++] = point;
					if (m_nCircleCount == 3)
						DrawCircleWithThreePoints();


					UpdateDisplay();
				}

				else
				{
					m_bDrag = true;
				}
			};

		auto threadpool = Threadpool::GetInstance();
		threadpool->AddWork(task);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CGrimInterviewDlg::OnBnClickedClear()
{
	auto fm = (unsigned  char*)m_image.GetBits();
	const int nPitch = m_image.GetPitch();
	const int nWidth = m_image.GetWidth();
	const int nHeight = m_image.GetHeight();
	m_nCircleCount = 0;

	clearImage();
	UpdateDisplay();
}

bool CGrimInterviewDlg::IsCirclePoint(const int i, const int j, const int centerX, const int centerY,
	const int nRadius, const int nLineThickness) const
{
	bool bRet = false;
	const int dx = i - centerX;
	const int dy = j - centerY;
	const int distance = dx * dx + dy * dy;
	const auto gap = abs(distance - nRadius * nRadius);
	if (gap <= nLineThickness* nLineThickness)
		bRet = true;

	return bRet;
}

void CGrimInterviewDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	auto task = [this, point]
		{
			if (m_bDrag)
			{
				for (int idx = 0; idx < 3; idx++)
				{
					const double dblDistanceSquared = (point.x - m_circleCenters[idx].x) * (point.x - m_circleCenters[idx].x) +
						(point.y - m_circleCenters[idx].y) * (point.y - m_circleCenters[idx].y);
					if (dblDistanceSquared < 6 * 6)
						m_circleCenters[idx] = point;
				}

				auto fm = (unsigned  char*)m_image.GetBits();
				const int nPitch = m_image.GetPitch();
				const int nWidth = m_image.GetWidth();
				const int nHeight = m_image.GetHeight();
				for (int j = 0; j < nHeight; j++)
				{
					for (int i = 0; i < nWidth; i++)
						fm[j * nPitch + i] = 0xff;
				}

				for (int idx = 0; idx < 3; idx++)
					drawCircle(fm, m_circleCenters[idx].x, m_circleCenters[idx].y, static_cast<int>(m_nRadius), s_Gray);

				DrawCircleWithThreePoints();
				UpdateDisplay();
			}

		};
	
	auto threadpool = Threadpool::GetInstance();
	threadpool->AddWork(task);

	CDialogEx::OnMouseMove(nFlags, point);
}

void CGrimInterviewDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bDrag = false;
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CGrimInterviewDlg::OnBnClickedRandomMove()
{
	if (m_nCircleCount < 3)
		return;

	auto threadpool = Threadpool::GetInstance();
	threadpool->AddWork([this]{this->randomMove();});
}

void CGrimInterviewDlg::OnBnClickedConsecutiveRandomMove()
{
	auto consecutive = [this]
		{
			for (size_t i = 0; i < 10; i++)
			{
				this->randomMove();
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		};

	auto threadpool = Threadpool::GetInstance();
	threadpool->AddWork(consecutive);
}
