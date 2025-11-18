
// GrimInterviewDlg.h : header file
//

#pragma once
#define WM_UPDATE_DISPLAY (WM_USER + 1)

// CGrimInterviewDlg dialog
class CGrimInterviewDlg : public CDialogEx
{
// Construction
public:
	CGrimInterviewDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GRIMINTERVIEW_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	BOOL OnInitDialog() override;
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCreateImage();

	afx_msg void OnBnClickedAction();

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void OnBnClickedClear();

	afx_msg LRESULT OnUpdateDisplay(WPARAM, LPARAM);

	void UpdateDisplay();

	void MoveCircle();

	bool IsInCircle(const int i, const int j, const int centerX, const int centerY, const int nRadius) const;

	bool IsInImage(const int i, const int j) const;

private:

	CImage m_image;

	size_t m_nRadius;

	size_t m_nCircleLine;

	size_t m_nCircleCount;

	constexpr static int s_Gray = 80;

	void drawCircle(unsigned char* fm, const int i, const int j, const int nRadius, unsigned char nGray);
	
};
