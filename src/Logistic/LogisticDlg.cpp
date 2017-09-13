
// LogisticDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Logistic.h"
#include "LogisticDlg.h"
#include "afxdialogex.h"

#include <util/common/math/common.h>
#include <util/common/plot/common.h>

#include <list>
#include <map>

#include "plot.h"

#define SAMPLE_COUNT 64
#define BITMAP_WIDTH 1000
#define BITMAP_HEIGHT 500

using namespace common;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLogisticDlg dialog

simple_list_plot function_plot = simple_list_plot::curve(RGB(150, 150, 150), 2);

double logistic_fn(double x, double r)
{
    return x * r * (1 - x);
}

CBitmap bifurc_bitmap;
bool bifurc_bitmap_visibility = true;
plot::world_t bifurc_world{2, 4, 0, 1};

// to reduce a number of ordinates in dataset
struct quantized_t
{
    double y;

    quantized_t(double y = 0) : y(y) { }
};

bool operator < (const quantized_t & q, const quantized_t & other)  { return (other.y - q.y) >= 1e-5;    }

bool operator == (const quantized_t & q, const quantized_t & other) { return abs(other.y - q.y) <= 1e-5; }

bool operator != (const quantized_t & q, const quantized_t & other) { return abs(other.y - q.y) > 1e-5;  }

struct multisample_t
{
    double x;
    std::map < quantized_t, bool > ordinates;
    // don't split onto dataset and internal tracer state holder
};

std::vector<multisample_t> samples;

CLogisticDlg::CLogisticDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLogisticDlg::IDD, pParent)
    , x0(0.9)
    , R(3.36)
    , mBitmapWidth(BITMAP_WIDTH)
    , mBitmapHeight(BITMAP_HEIGHT)
    , mDrawBitmap(TRUE)
    , mIterations(100)
    , mMinIterations(100)
    , mMaxR(3.55)
    , mDrawTrace(FALSE)
    , mBif1Str(_T(""))
    , mBif2Str(_T(""))
    , mBif3Str(_T(""))
    , mBifurcThr(8)
    , mMaxBifurcR(3.46)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    mFunctionPlotCtrl.plot_layer.with(
        (plot::plot_builder() << function_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 1)
        .with_y_ticks(0, 5, 1)
        .build()
    );
    mBifurcPlotCtrl.plot_layer.with(
        plot::plot_builder()
        .with_layer(std::make_unique<plot::bitmap_drawable>(&bifurc_bitmap, &bifurc_bitmap_visibility))
        .with_custom([this] (CDC &dc, const plot::viewport &bounds) {
            if (!mDrawTrace) return;
            auto pen = plot::palette::pen(RGB(0,190,0), 2);
            dc.SelectObject(pen.get());
            std::vector < plot::point<double> > interesting;
            TraceDiagram(1e-2, 5, mMaxR, [this, &dc, &bounds, &interesting]
                         (plot::point<double> src, plot::point<double> dst1, plot::point<double> dst2, int n_interesting_points)
            {
                if (dst1.x != 0)
                {
                    dc.MoveTo(bounds.world_to_screen().xy(src));
                    dc.LineTo(bounds.world_to_screen().xy(dst1));
                }
                if (dst2.x != 0)
                {
                    dc.MoveTo(bounds.world_to_screen().xy(src));
                    dc.LineTo(bounds.world_to_screen().xy(dst2));
                }
                if ((dst1.x != 0) && (dst2.x != 0) && (n_interesting_points > mBifurcThr))
                {
                    auto brush = plot::palette::brush(RGB(180, 0, 0));
                    auto point = bounds.world_to_screen().xy(src);
                    RECT rect;
                    rect.left = point.x - 2; rect.top = point.y - 2;
                    rect.right = point.x + 2; rect.bottom = point.y + 2;
                    dc.FillRect(&rect, brush.get());
                    interesting.push_back(src);
                }
            });
            const int x_quants = 100, y_quants = 100;
            const double stop_at = mMaxBifurcR;
            std::vector<std::vector<plot::point<double>>> x_projection(x_quants), y_projection(y_quants);
            for each (auto &p in interesting)
            {
                if (p.x > stop_at) continue;
                x_projection[(p.x - bifurc_world.xmin) / (bifurc_world.xmax - bifurc_world.xmin) * x_quants].push_back(p);
                y_projection[(p.y - bifurc_world.ymin) / (bifurc_world.ymax - bifurc_world.ymin) * y_quants].push_back(p);
            }
            
            int current_region = -1;
            for (int i = 0; i < x_projection.size(); ++i)
            {
                auto &x_prj = x_projection[i];
                if (!x_prj.empty() && (current_region != -1))
                {
                    auto &region = x_projection[current_region];
                    region.insert(region.end(), x_prj.begin(), x_prj.end());
                    x_prj.clear();
                }
                else if (!x_prj.empty())
                {
                    current_region = i;
                }
                else
                {
                    current_region = -1;
                }
            }
            current_region = -1;
            for (int i = 0; i < y_projection.size(); ++i)
            {
                auto &x_prj = y_projection[i];
                if (!x_prj.empty() && (current_region != -1))
                {
                    auto &region = y_projection[current_region];
                    region.insert(region.end(), x_prj.begin(), x_prj.end());
                    x_prj.clear();
                }
                else if (!x_prj.empty())
                {
                    current_region = i;
                }
                else
                {
                    current_region = -1;
                }
            }
            for each (auto &x_prj in x_projection)
            {
                if (!x_prj.empty())
                {
                    for each (auto &y_prj in y_projection)
                    {
                        if (!y_prj.empty())
                        {
                            bool found = false;
                            for each (auto &p in x_prj)
                            {
                                for each (auto &p2 in y_prj)
                                {
                                    if ((p.x == p2.x) && (p.y == p2.y))
                                    {
                                        if (bif1.x == 0) bif1 = p;
                                        else if (bif2.x == 0) bif2 = p;
                                        else if (bif3.x == 0) bif3 = p;
                                        found = true;
                                        break;
                                    }
                                }
                                if (found) break;
                            }
                        }
                    }
                }
            }
            return;
        })
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 20, 2)
        .with_y_ticks(0, 10, 2)
        .in_world(bifurc_world)
        .build()
    );
}

void CLogisticDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FUNCTION, mFunctionPlotCtrl);
    DDX_Control(pDX, IDC_BIFURC, mBifurcPlotCtrl);
    DDX_Text(pDX, IDC_EDIT1, x0);
    DDX_Text(pDX, IDC_EDIT2, R);
    DDX_Text(pDX, IDC_EDIT5, mBitmapWidth);
    DDX_Text(pDX, IDC_EDIT6, mBitmapHeight);
    DDX_Check(pDX, IDC_CHECK1, mDrawBitmap);
    DDX_Text(pDX, IDC_EDIT4, mIterations);
    DDX_Text(pDX, IDC_EDIT3, mMinIterations);
    DDX_Text(pDX, IDC_EDIT9, mMaxR);
    DDX_Check(pDX, IDC_CHECK2, mDrawTrace);
    DDX_Text(pDX, IDC_BIF1, mBif1Str);
    DDX_Text(pDX, IDC_BIF2, mBif2Str);
    DDX_Text(pDX, IDC_BIF3, mBif3Str);
    DDX_Text(pDX, IDC_EDIT7, mBifurcThr);
    DDX_Text(pDX, IDC_EDIT8, mMaxBifurcR);
}

BEGIN_MESSAGE_MAP(CLogisticDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CLogisticDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CLogisticDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_CHECK1, &CLogisticDlg::OnBnClickedCheck1)
    ON_BN_CLICKED(IDC_CHECK2, &CLogisticDlg::OnBnClickedCheck2)
    ON_BN_CLICKED(IDC_BUTTON3, &CLogisticDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CLogisticDlg message handlers

BOOL CLogisticDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLogisticDlg::OnPaint()
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
HCURSOR CLogisticDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CLogisticDlg::OnBnClickedButton1()
{
    UpdateData(TRUE);

    sampled_t function_sampled = allocate_sampled(SAMPLE_COUNT, 1);

    double xx = x0;
    map(function_sampled, [this, &xx] (size_t, double) { return (xx = logistic_fn(xx, R)); });
    setup(function_plot, function_sampled);
    mFunctionPlotCtrl.RedrawWindow();
    
    free_sampled(function_sampled);
}

void CLogisticDlg::OnBnClickedButton2()
{
    UpdateData(TRUE);

    CDC memDC; memDC.CreateCompatibleDC(NULL);
    if (bifurc_bitmap.m_hObject == NULL)
    {
        bifurc_bitmap.CreateBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, 1, 1, NULL);
    }
    memDC.SelectObject(&bifurc_bitmap);
    memDC.FillSolidRect(0, 0, BITMAP_WIDTH, BITMAP_HEIGHT, 0);

    CDC workingDC; workingDC.CreateCompatibleDC(NULL);
    CBitmap working_bitmap; working_bitmap.CreateBitmap(mBitmapWidth, mBitmapHeight, 1, 1, NULL);
    workingDC.SelectObject(&working_bitmap);

    samples.clear();
    samples.resize(mBitmapWidth);

    const double rmin = bifurc_world.xmin, rmax = bifurc_world.xmax;
    const double xmin = bifurc_world.ymin, xmax = bifurc_world.ymax;
    for (int m = 0; m < mIterations; ++m)
    {
        double x0 = (double)rand() / RAND_MAX;
        for (size_t j = 0; j < mBitmapWidth; ++j)
        {
            double r = rmin + (rmax - rmin) * j / (double)mBitmapWidth;
            double x = x0;
            for (int k = 0; k < mMinIterations; ++k, x = logistic_fn(x, r))
                ;
            int i = mBitmapHeight - x * mBitmapHeight;
            workingDC.SetPixel(j, i, RGB(255,255,255));
            memDC.SetPixel(min((double) j / mBitmapWidth * BITMAP_WIDTH, BITMAP_WIDTH), BITMAP_HEIGHT - min(x * BITMAP_HEIGHT, BITMAP_HEIGHT), RGB(255,255,255));
            samples[j].x = r;
            samples[j].ordinates.insert({ x, false });
        }
    }

    CImage image;
    image.Attach(working_bitmap);
    image.Save(_T("bifurc.bmp"), Gdiplus::ImageFormatBMP);
    
    workingDC.SelectObject((CBitmap *) NULL);
    workingDC.DeleteDC();
    memDC.SelectObject((CBitmap *) NULL);
    memDC.DeleteDC();

    mDrawTrace = FALSE;
    UpdateData(FALSE);

    mBifurcPlotCtrl.RedrawWindow();
}


void CLogisticDlg::OnBnClickedCheck1()
{
    UpdateData(TRUE);
    bifurc_bitmap_visibility = mDrawBitmap;
    mBifurcPlotCtrl.RedrawWindow();
}

void CLogisticDlg::TraceDiagram(double y_eps, int x_eps, double x_max,
                                std::function<void(plot::point<double> src, plot::point<double> dst1, plot::point<double> dst2, int n_interesting_points)> callback)
{
    if (samples.empty() || samples.front().ordinates.empty()) return;
    for (std::vector < multisample_t >::iterator it0 = samples.begin(); it0 < samples.end(); ++it0)
    {
        for (std::map < quantized_t, bool >::iterator it = it0->ordinates.begin(); it != it0->ordinates.end(); ++it)
        {
            it->second = false;
        }
    }
    std::list<std::pair<int, quantized_t>> stack;
    for each (auto a in samples.front().ordinates)
    {
        stack.emplace_back(0, a.first);
    }
    while (!stack.empty())
    {
        auto p = stack.back(); stack.pop_back();
        if (samples[p.first].x > x_max) continue;
        plot::point<double> top, bottom;
        quantized_t begin = { p.second.y - y_eps }, end = { p.second.y + y_eps };
        int n_interesting_points = 0;
        for (int i = 1; i <= x_eps; ++i)
        {
            if ((p.first + i >= samples.size()) || ((top.y != 0) && (bottom.y != 0))) break;
            std::pair<const quantized_t, bool> *last = 0;
            for (std::map<quantized_t, bool>::iterator it = samples[p.first + i].ordinates.begin(); it != samples[p.first + i].ordinates.end(); ++it)
            {
                if ((begin < it->first) && (it->first < end))
                {
                    ++n_interesting_points;
                    if (it->second) continue;
                    if (top.y == 0)
                    {
                        top = { samples[p.first + i].x, it->first.y };
                        stack.push_back({ p.first + i, top.y });
                        it->second = true;
                    }
                    else
                    {
                        if (bottom.y == 0)
                        {
                            std::map<quantized_t, bool>::iterator it2 = it;
                            if ((++it2) == samples[p.first + i].ordinates.end())
                            {
                                bottom = { samples[p.first + i].x, it->first.y };
                                stack.push_back({ p.first + i, bottom.y });
                                it->second = true;
                            }
                        }
                    }
                    last = &*it;
                }
                else if (((last != 0) && (top.y != last->first.y)) && (bottom.y == 0))
                {
                    if (it->second) continue;
                    bottom = { samples[p.first + i].x, last->first.y };
                    stack.push_back({ p.first + i, bottom.y });
                    last->second = true;
                }
            }
        }
        callback({ samples[p.first].x, p.second.y }, top, bottom, n_interesting_points);
    }
}

void CLogisticDlg::OnBnClickedCheck2()
{
    UpdateData(TRUE);
    if (mDrawTrace)
    {
        bif1 = bif2 = bif3 = {};
        mBif1Str = mBif2Str = mBif3Str = "";
    }
    mBifurcPlotCtrl.RedrawWindow();
    if (mDrawTrace)
    {
        if (bif1.x != 0) mBif1Str.Format(_T("%.2lf;%.2lf"), bif1.x, bif1.y);
        if (bif2.x != 0) mBif2Str.Format(_T("| %.2lf;%.2lf |"), bif2.x, bif2.y);
        if (bif3.x != 0) mBif3Str.Format(_T("%.2lf;%.2lf"), bif3.x, bif3.y);
        UpdateData(FALSE);
    }
}


void CLogisticDlg::OnBnClickedButton3()
{
    UpdateData(TRUE);
    mDrawTrace = TRUE;
    if (mDrawTrace)
    {
        bif1 = bif2 = bif3 = {};
        mBif1Str = mBif2Str = mBif3Str = "";
    }
    mBifurcPlotCtrl.RedrawWindow();
    UpdateData(FALSE);
    if (mDrawTrace)
    {
        if (bif1.x != 0) mBif1Str.Format(_T("%.2lf;%.2lf"), bif1.x, bif1.y);
        if (bif2.x != 0) mBif2Str.Format(_T("| %.2lf;%.2lf |"), bif2.x, bif2.y);
        if (bif3.x != 0) mBif3Str.Format(_T("%.2lf;%.2lf"), bif3.x, bif3.y);
        UpdateData(FALSE);
    }
}
