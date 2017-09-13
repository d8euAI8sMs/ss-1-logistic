
// LogisticDlg.h : header file
//

#pragma once


#include <util/common/plot/PlotStatic.h>

// CLogisticDlg dialog
class CLogisticDlg : public CDialogEx
{
// Construction
public:
	CLogisticDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_LOGISTIC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    PlotStatic mFunctionPlotCtrl;
    PlotStatic mBifurcPlotCtrl;
    double x0;
    double R;
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    int mBitmapWidth;
    int mBitmapHeight;
    BOOL mDrawBitmap;
    int mIterations;
    int mMinIterations;
    afx_msg void OnBnClickedCheck1();
    void TraceDiagram(double y_eps, int x_eps, double x_max,
                      std::function<void(plot::point<double> src, plot::point<double> dst1, plot::point<double> dst2, int n_interesting_points)> callback);
    double mMaxR;
    BOOL mDrawTrace;
    plot::point<double> bif1, bif2, bif3;
    afx_msg void OnBnClickedCheck2();
    afx_msg void OnBnClickedButton3();
    CString mBif1Str;
    CString mBif2Str;
    CString mBif3Str;
    int mBifurcThr;
    double mMaxBifurcR;
};
