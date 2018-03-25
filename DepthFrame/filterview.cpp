
#include "stdafx.h"
#include "filterview.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "rectcontour.h"


#ifdef _DEBUG
	#pragma comment(lib, "opencv_core340d.lib")
	//#pragma comment(lib, "opencv_imgcodecs310d.lib")
	//#pragma comment(lib, "opencv_features2d310d.lib")
	//#pragma comment(lib, "opencv_videoio310d.lib")
	//#pragma comment(lib, "opencv_highgui310d.lib")
	#pragma comment(lib, "opencv_imgproc340d.lib")
	//#pragma comment(lib, "opencv_flann310d.lib")
	//#pragma comment(lib, "opencv_xfeatures2d310d.lib")
	//#pragma comment(lib, "opencv_calib3d310d.lib")
#else
	//#pragma comment(lib, "opencv_core310.lib")
	//#pragma comment(lib, "opencv_imgcodecs310.lib")
	//#pragma comment(lib, "opencv_features2d310.lib")
	//#pragma comment(lib, "opencv_videoio310.lib")
	//#pragma comment(lib, "opencv_highgui310.lib")
	//#pragma comment(lib, "opencv_imgproc310.lib")
	//#pragma comment(lib, "opencv_flann310.lib")
	//#pragma comment(lib, "opencv_xfeatures2d310.lib")
	//#pragma comment(lib, "opencv_calib3d310.lib")
#endif


using namespace graphic;
using namespace framework;
using namespace cv;


/**
* Helper function to find a cosine of angle between vectors
* from pt0->pt1 and pt0->pt2
*/
double angle(cv::Point pt1, const cv::Point &pt2, const cv::Point &pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1 * dy2) / sqrt((dx1*dx1 + dy1 * dy1)*(dx2*dx2 + dy2 * dy2) + 1e-10);
}

/**
* Helper function to display text in the center of a contour
*/
void setLabel(cv::Mat& im, const std::string &label, const std::vector<cv::Point>& contour, const cv::Scalar &color = cv::Scalar(0, 0, 0))
{
	int fontface = cv::FONT_HERSHEY_SIMPLEX;
	double scale = 0.4;
	int thickness = 1;
	int baseline = 0;

	cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
	cv::Rect r = cv::boundingRect(contour);

	cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
	cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255, 255, 255), CV_FILLED);
	cv::putText(im, label, pt, fontface, scale, color, thickness, 8);
}





cFilterView::cFilterView(const string &name)
	: framework::cDockWindow(name)
	//, m_thresholdMin(0)
	//, m_thresholdMax(50000)
{
}

cFilterView::~cFilterView()
{
}


bool cFilterView::Init(graphic::cRenderer &renderer)
{
	m_depthTexture.Create(renderer, g_baslerDepthWidth, g_baslerDepthHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

	m_srcImg = cv::Mat(480, 640, CV_32FC1);

	return true;
}


void cFilterView::OnRender(const float deltaSeconds)
{
	if (g_root.m_isUpdate)
	{
		ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_vertices[0]
			, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
			, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image(m_depthTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

	// HUD
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(std::min(m_rect.Width() - 15.f, 500.f), 250));
	if (ImGui::Begin("FilterView Info", &isOpen, ImVec2(std::min(m_rect.Width() - 15.f, 500.f), 250.f), windowAlpha, flags))
	{
		

		//bool isUpdate = false;
		//if (ImGui::DragInt("Threshold Min", &m_thresholdMin, 100, 0, USHORT_MAX))
		//	isUpdate = true;
		//if (ImGui::DragInt("Threshold Max", &m_thresholdMax, 100, 1, USHORT_MAX))
		//	isUpdate = true;

		//m_thresholdMin = min(m_thresholdMin, m_thresholdMax);
		//m_thresholdMax = max(m_thresholdMin, m_thresholdMax);

		//if (isUpdate)
		//{
		//	ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_vertices[0]
		//		, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
		//		, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
		//}

		ImGui::End();
	}
}


void cFilterView::ProcessDepth()
{
	ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_vertices[0]
		, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
		, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
}


void cFilterView::ProcessDepth(INT64 nTime
	, const Vector3 *pBuffer
	, int nWidth
	, int nHeight
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	if ((nWidth != m_depthTexture.Width()) || (nHeight != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), nWidth, nHeight, DXGI_FORMAT_R32G32B32_FLOAT);
	}

	const size_t sizeInBytes2 = m_srcImg.step[0] * m_srcImg.rows;
	float *dst = (float*)m_srcImg.data;
	for (int i = 0; i < nWidth*nHeight; ++i)
		*dst++ = pBuffer[i].y / 500.f;

	Mat grayscaleMat;
	m_srcImg.convertTo(grayscaleMat, CV_8UC1, 255.0f);
	m_srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);

	cv::threshold(grayscaleMat, m_binImg, 50, 255, cv::THRESH_BINARY);
	findContours(m_binImg, m_contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	int m_minArea = 100;
	double m_minCos = -0.4f;
	double m_maxCos = 0.4f;

	std::vector<cv::Point> approx;

	for (u_int i = 0; i < m_contours.size(); i++)
	{
		// Approximate contour with accuracy proportional
		// to the contour perimeter
		cv::approxPolyDP(cv::Mat(m_contours[i]), approx, cv::arcLength(cv::Mat(m_contours[i]), true)*0.02f, true);

		// Skip small or non-convex objects 
		if (std::fabs(cv::contourArea(m_contours[i])) < m_minArea || !cv::isContourConvex(approx))
			continue;

		if (approx.size() == 4)
		{
			// Number of vertices of polygonal curve
			const int vtc = approx.size();

			// Get the cosines of all corners
			std::vector<double> cos;
			for (int j = 2; j < vtc + 1; j++)
				cos.push_back(angle(approx[j%vtc], approx[j - 2], approx[j - 1]));

			// Sort ascending the cosine values
			std::sort(cos.begin(), cos.end());

			// Get the lowest and the highest cosine
			const double mincos = cos.front();
			const double maxcos = cos.back();

			// Use the degrees obtained above and the number of vertices
			// to determine the shape of the contour
			if ((vtc == 4) && (mincos >= m_minCos) && (maxcos <= m_maxCos))
			{
				vector<cv::Point> rectPoint(4);
				rectPoint[0] = approx[0];
				rectPoint[1] = approx[1];
				rectPoint[2] = approx[2];
				rectPoint[3] = approx[3];
				cRectContour rect;
				rect.Init(rectPoint);

				setLabel(m_dstImg, "BOX", m_contours[i], Scalar(1,1,1));
				rect.Draw(m_dstImg, Scalar(255, 255, 255), 1);

				//isDetect = true;
			}
		}
	}

	UpdateTexture();
}


// Mat -> DX11 Texture
void cFilterView::UpdateTexture()
{
	const int nWidth = g_baslerDepthWidth;
	const int nHeight = g_baslerDepthHeight;

	BYTE *src = (BYTE*)m_dstImg.data;
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_depthTexture.Lock(renderer, map))
	{
		for (int i = 0; i < nHeight; ++i)
		{
			for (int k = 0; k < nWidth; ++k)
			{
				const BYTE v1 = src[i * m_dstImg.step[0] + k];
				BYTE *p = dst + (i * map.RowPitch) + (k * sizeof(float) * 4);

				*(float*)p = v1 / 255.f;
				*(float*)(p + sizeof(float) * 1) = v1 / 255.f;
				*(float*)(p + sizeof(float) * 2) = v1 / 255.f;
				*(float*)(p + sizeof(float) * 3) = 1.f;
			}
		}

		m_depthTexture.Unlock(renderer);
	}
}
