
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

struct graph_node_t
{
	plot::point < double > point;
	std::vector < graph_node_t * > successors;
};

class weak_less_double
{
private: double eps;
public: weak_less_double(double eps) : eps(eps) { }
public: bool operator () (const double &d1, const double &d2) { return (d2 - d1) >= eps; }
};

std::vector < std::vector < graph_node_t > > graph;

CLogisticDlg::CLogisticDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLogisticDlg::IDD, pParent)
    , x0(0.9)
    , R(3.36)
    , mBitmapWidth(BITMAP_WIDTH)
    , mBitmapHeight(BITMAP_HEIGHT)
    , mDrawBitmap(TRUE)
    , mIterations(100)
    , mMinIterations(100)
    , mDrawTrace(FALSE)
    , mBif1Str(_T(""))
    , mBif2Str(_T(""))
    , mBif3Str(_T(""))
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
            std::vector < const graph_node_t * > stack;
            for each (auto &node in graph[0])
            {
                stack.push_back(&node);
            }
            while (!stack.empty())
            {
                const graph_node_t * node = stack.back(); stack.pop_back();
                for each (auto next in node->successors)
                {
                    dc.MoveTo(bounds.world_to_screen().xy(node->point));
                    dc.LineTo(bounds.world_to_screen().xy(next->point));
                    stack.push_back(next);
                }
            }
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
    DDX_Check(pDX, IDC_CHECK2, mDrawTrace);
    DDX_Text(pDX, IDC_BIF1, mBif1Str);
    DDX_Text(pDX, IDC_BIF2, mBif2Str);
    DDX_Text(pDX, IDC_BIF3, mBif3Str);
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

	std::set < double, weak_less_double > points(weak_less_double(bifurc_world.height() * 1e-4));
	graph.clear();
	graph.resize(mBitmapWidth);

    plot::viewport bifurc_bounds = { { 0, BITMAP_WIDTH, 0, BITMAP_HEIGHT }, bifurc_world };
    plot::viewport working_bounds = { { 0, mBitmapWidth, 0, mBitmapHeight }, bifurc_world };

    for (size_t j = 0; j < mBitmapWidth; ++j)
    {
        double r = working_bounds.screen_to_world().x(j);
        int iterations = 0;
        int old_points_size;
        do
        {
            old_points_size = points.size();
		    for (int m = 0; m < 100; ++m, ++iterations) // magic 100
		    {
		    	double x = 0;
                while (x == 0)
                {
                    x = (double)rand() / RAND_MAX;
		    	    for (int k = 0; k < mMinIterations; ++k, x = logistic_fn(x, r))
		    	    	;
                }
		    	int i = working_bounds.world_to_screen().y(x);
		    	workingDC.SetPixel(j, i, RGB(255, 255, 255));
		    	memDC.SetPixel(bifurc_bounds.world_to_screen().x(r), bifurc_bounds.world_to_screen().y(x), RGB(255, 255, 255));
		    	points.insert(x);
		    }
        } while ((iterations < mIterations) && (points.size() >= old_points_size + 10)); // magic 10 = 100 / 10
        graph[j].resize(points.size());
        int k = 0;
		for each (double x in points)
		{
            graph[j][k].point = { r, x };
			graph_node_t &current = graph[j][k];
			if (j != 0)
			{
				double distance = std::numeric_limits<double>::infinity();
				graph_node_t *closest_node = nullptr;
				for (std::vector < graph_node_t >::iterator it = graph[j - 1].begin(); it != graph[j - 1].end(); ++it)
				{
					double d;
					if ((d = abs(it->point.y - x)) < distance)
					{
						distance = d;
						closest_node = &*it;
					}
				}
				if (closest_node != nullptr)
				{
					closest_node->successors.push_back(&current);
				}
			}
            ++k;
		}
		points.clear();
    }

    CImage image;
    image.Attach(working_bitmap);
    image.Save(_T("bifurc.bmp"), Gdiplus::ImageFormatBMP);
    
    memDC.SelectObject((CBitmap *) NULL);
    memDC.DeleteDC();

    auto black_brush = plot::palette::brush(0);
    workingDC.FillRect(&(RECT)working_bounds.screen, black_brush.get());
    
    auto white_pen = plot::palette::pen(RGB(255, 255, 255));
    workingDC.SelectObject(white_pen.get());

    std::vector < const graph_node_t * > stack;
    for each (auto &node in graph[0])
    {
        stack.push_back(&node);
    }
    while (!stack.empty())
    {
        const graph_node_t * node = stack.back(); stack.pop_back();
        for each (auto next in node->successors)
        {
            workingDC.MoveTo(working_bounds.world_to_screen().xy(node->point));
            workingDC.LineTo(working_bounds.world_to_screen().xy(next->point));
            stack.push_back(next);
        }
    }
    
    image.Save(_T("bifurc-traced.bmp"), Gdiplus::ImageFormatBMP);

    workingDC.SelectObject((CBitmap *) NULL);
    workingDC.DeleteDC();

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
