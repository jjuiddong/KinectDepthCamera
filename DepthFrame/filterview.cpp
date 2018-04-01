
#include "stdafx.h"
#include "filterview.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
//#include "rectcontour.h"


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
	#pragma comment(lib, "opencv_core340.lib")
	//#pragma comment(lib, "opencv_imgcodecs310.lib")
	//#pragma comment(lib, "opencv_features2d310.lib")
	//#pragma comment(lib, "opencv_videoio310.lib")
	//#pragma comment(lib, "opencv_highgui310.lib")
	#pragma comment(lib, "opencv_imgproc340.lib")
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

void setLabel(cv::Mat& im, const std::string &label, const cv::Point& pos, const cv::Scalar &color = cv::Scalar(0, 0, 0))
{
	int fontface = cv::FONT_HERSHEY_SIMPLEX;
	double scale = 0.4;
	int thickness = 1;
	int baseline = 0;

	//cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
	//cv::Rect r = cv::boundingRect(contour);

	//cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
	//cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255, 255, 255), CV_FILLED);
	cv::putText(im, label, pos, fontface, scale, color, thickness, 8);
}




cFilterView::cFilterView(const string &name)
	: framework::cDockWindow(name)
{
}

cFilterView::~cFilterView()
{
}


bool cFilterView::Init(graphic::cRenderer &renderer)
{
	m_depthTexture.Create(renderer, g_baslerDepthWidth, g_baslerDepthHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

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
	ImGui::SetNextWindowSize(ImVec2(std::min(m_rect.Width() - 15.f, 500.f), m_rect.Height()));
	if (ImGui::Begin("FilterView Info", &isOpen, ImVec2(std::min(m_rect.Width() - 15.f, 500.f), m_rect.Height()), windowAlpha, flags))
	{
		ImGui::Spacing();
		ImGui::Separator();
		for (u_int i = 0; i < g_root.m_boxes.size(); ++i)
		{
			auto &box = g_root.m_boxes[i];
			ImGui::Text("Box%d", i + 1);
			ImGui::Text("\t X = %f", box.volume.x);
			ImGui::Text("\t Y = %f", box.volume.z);
			ImGui::Text("\t H = %f", box.volume.y);
			ImGui::Spacing();
			ImGui::Separator();
		}

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

	m_rects.clear();
	m_removeRects.clear();
	cv::Mat &srcImg = g_root.m_sensorBuff.m_srcImg;

	Mat grayscaleMat;
	srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);
	srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	for (int k = 0; k < g_root.m_areaFloorCnt; ++k)
	{
		cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[k];

		float threshold = areaFloor->startIdx * 0.1f;

		cv::threshold(grayscaleMat, m_binImg, threshold, 255, cv::THRESH_BINARY);
		m_binImg.convertTo(m_binImg, CV_8UC1);

		int loopCnt = 0;
		cv::erode(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);
		cv::erode(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);

		while (loopCnt < 8)
		{
			vector<cRectContour> out;
			if (FindBox(m_binImg, out))
			{
				for (auto &r : out)
				{
					sRectInfo info;
					info.loop = loopCnt;
					info.lowerH = 0;
					info.upperH = areaFloor->maxIdx * 0.1f;
					info.r = r;
					m_rects.push_back(info);
				}
				break;
			}

			cv::dilate(m_binImg, m_binImg, element);
			++loopCnt;
		}
	}

	// Box 중복인식 제거
	if (!m_rects.empty())
	{
		set<int> rmIndices;
		for (u_int i = 0; i < m_rects.size()-1; ++i)
		{
			for (u_int k = i+1; k < m_rects.size(); ++k)
			{
				if (m_rects[i].r.IsContain(m_rects[k].r))
				{
					const int a1 = m_rects[i].r.Width() * m_rects[i].r.Height();
					const int a2 = m_rects[k].r.Width() * m_rects[k].r.Height();
					const bool isDuplciate = abs(((float)a1 / (float)a2) - 1.f) < 0.1f; // 거의 같은 사이즈
					m_rects[k].duplicate = isDuplciate;

					// 중복 인식된 박스일 때, 더 낮은 높이의 박스를 제거한다.
					if (isDuplciate)
					{
						//if (a1 > a2)
						if (m_rects[i].upperH > m_rects[k].upperH)
						{
							rmIndices.insert(k);
						}
						else
						{
							rmIndices.insert(i);
						}
					}
					else
					{
						// 두개의 박스가 위아래로 겹쳐져 있을 때,
						// 박스바닥 높이를 계산한다.
						if (m_rects[i].upperH > m_rects[k].upperH)
						{
							m_rects[i].lowerH = m_rects[k].upperH;
						}
						else
						{
							m_rects[k].lowerH = m_rects[i].upperH;
						}
					}

				}
			}
		}

		// 높은 인덱스부터 제거한다.
		for (auto it = rmIndices.rbegin(); it != rmIndices.rend(); ++it)
		{
			if (!m_rects[*it].duplicate) // 비슷한 크기의 박스는 표시하지 않는다. 중복 인식된 박스
				m_removeRects.push_back(m_rects[*it]);

			common::popvector(m_rects, *it);
		}
	}

	// display remove rect
	for (auto &info : m_removeRects)
	{
		cRectContour &rect = info.r;
		const Scalar color(255, 0, 0);
		rect.Draw(m_dstImg, color, 1);
	}

	// Display Detect Box
	g_root.m_boxes.clear();
	char boxName[64];
	for (u_int i=0; i < m_rects.size(); ++i)
	{
		auto &info = m_rects[i];
		cRectContour &rect = info.r;
		const Scalar color(255, 255, 255);
		
		sprintf(boxName, "BOX%d", i + 1);
		setLabel(m_dstImg, boxName, rect.m_contours, Scalar(1, 1, 1));
		setLabel(m_dstImg, " 1", rect.At(0), color);
		setLabel(m_dstImg, " 2", rect.At(1), color);
		setLabel(m_dstImg, " 3", rect.At(2), color);
		setLabel(m_dstImg, " 4", rect.At(3), color);

		cv::circle(m_dstImg, rect.At(0), 5, color);
		cv::circle(m_dstImg, rect.At(1), 5, color);
		cv::circle(m_dstImg, rect.At(2), 5, color);
		cv::circle(m_dstImg, rect.At(3), 5, color);
		
		rect.Draw(m_dstImg, color, 1);

		for (int i = 0; i < 4; ++i)
		{
			g_root.m_box3DPos[i] = g_root.m_sensorBuff.m_vertices[rect.At(i).y * 640 + rect.At(i).x];
		}

		const Vector2 v1((float)rect.At(0).x, (float)rect.At(0).y);
		const Vector2 v2((float)rect.At(1).x, (float)rect.At(1).y);
		const Vector2 v3((float)rect.At(2).x, (float)rect.At(2).y);
		const Vector2 v4((float)rect.At(3).x, (float)rect.At(3).y);
		
		//const float scale = 50.f / 110.f;
		const float scale = 50.f / 73.2f;
		const float offsetY = ((info.lowerH <= 0) && g_root.m_isPalete)? -13.f : 2.5f;
		
		// maximum value
		cRoot::sBoxInfo box;
		const float l1 = std::max((v1 - v2).Length(), (v3 - v4).Length());
		const float l2 = std::max((v2 - v3).Length(), (v4 - v1).Length());
		box.volume.x = std::max(l1, l2);
		box.volume.y = (info.upperH - info.lowerH) + offsetY;
		box.volume.z = std::min(l1, l2);

		//box.volume.x = (((v1 - v2).Length()) + ((v3 - v4).Length())) * 0.5f;
		//box.volume.y = (info.upperH - info.lowerH) + offsetY;
		//box.volume.z = (((v2 - v3).Length()) + ((v4 - v1).Length())) * 0.5f;

		box.volume.x *= scale;
		box.volume.y *= 1.f;
		box.volume.z *= scale;

		box.volume.x -= (float)info.loop*1.5f;
		box.volume.z -= (float)info.loop*1.5f;

		g_root.m_boxes.push_back(box);
	}

	UpdateTexture();
}


bool cFilterView::FindBox(cv::Mat &img, OUT vector<cRectContour> &out)
{
	findContours(img, m_contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	int m_minArea = 200;
	double m_minCos = -0.4f;
	double m_maxCos = 0.4f;

	vector<cRectContour> rects;
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
				out.push_back(rect);

				//setLabel(m_dstImg, "BOX", m_contours[i], Scalar(1, 1, 1));
				//setLabel(m_dstImg, " 1", approx[0], Scalar(255, 255, 255));
				//setLabel(m_dstImg, " 2", approx[1], Scalar(255, 255, 255));
				//setLabel(m_dstImg, " 3", approx[2], Scalar(255, 255, 255));
				//setLabel(m_dstImg, " 4", approx[3], Scalar(255, 255, 255));

				//cv::circle(m_dstImg, approx[0], 5, Scalar(255, 255, 255));
				//cv::circle(m_dstImg, approx[1], 5, Scalar(255, 255, 255));
				//cv::circle(m_dstImg, approx[2], 5, Scalar(255, 255, 255));
				//cv::circle(m_dstImg, approx[3], 5, Scalar(255, 255, 255));

				//rect.Draw(m_dstImg, Scalar(255, 255, 255), 1);

				AddLog("Detect Box");
				AddLog(common::format("pos1 = %d, %d", approx[0].x, approx[0].y));
				AddLog(common::format("pos2 = %d, %d", approx[1].x, approx[1].y));
				AddLog(common::format("pos3 = %d, %d", approx[2].x, approx[2].y));
				AddLog(common::format("pos4 = %d, %d", approx[3].x, approx[3].y));

				const Vector2 v1((float)approx[0].x, (float)approx[0].y);
				const Vector2 v2((float)approx[1].x, (float)approx[1].y);
				const Vector2 v3((float)approx[2].x, (float)approx[2].y);
				const Vector2 v4((float)approx[3].x, (float)approx[3].y);
				AddLog(common::format("pos1-2 len = %f", (v1 - v2).Length()));
				AddLog(common::format("pos2-3 len = %f", (v2 - v3).Length()));
				AddLog(common::format("pos3-4 len = %f", (v3 - v4).Length()));
				AddLog(common::format("pos4-1 len = %f", (v4 - v1).Length()));
			}
		}
	}

	return !out.empty();
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
				const BYTE v1 = src[i * m_dstImg.step[0] + k * 3];
				const BYTE v2 = src[i * m_dstImg.step[0] + k * 3 + 1];
				const BYTE v3 = src[i * m_dstImg.step[0] + k * 3 + 2];
				BYTE *p = dst + (i * map.RowPitch) + (k * sizeof(float) * 4);

				*(float*)p = v1 / 255.f;
				*(float*)(p + sizeof(float) * 1) = v2 / 255.f;
				*(float*)(p + sizeof(float) * 2) = v3 / 255.f;
				*(float*)(p + sizeof(float) * 3) = 1.f;
			}
		}

		m_depthTexture.Unlock(renderer);
	}
}
