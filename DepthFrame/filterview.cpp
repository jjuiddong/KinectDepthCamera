
#include "stdafx.h"
#include "filterview.h"
#include "cvutil.h"

using namespace graphic;
using namespace framework;
using namespace cv;


cFilterView::cFilterView(const string &name)
	: framework::cDockWindow(name)
{
}

cFilterView::~cFilterView()
{
}


bool cFilterView::Init(graphic::cRenderer &renderer)
{
	m_depthTexture.Create(renderer, (int)g_capture3DWidth, (int)g_capture3DHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

	return true;
}


void cFilterView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image(m_depthTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

	// HUD
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(std::min(m_rect.Width() - 15.f, 800.f), m_rect.Height()));
	if (ImGui::Begin("FilterView Info", &isOpen, ImVec2(std::min(m_rect.Width() - 15.f, 800.f), m_rect.Height()), windowAlpha, flags))
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
			ImGui::Text("\t V/W = %f", box.minVolume / 6000.f);
			ImGui::Spacing();
			ImGui::Separator();
		}

		ImGui::End();
	}
}


void cFilterView::Process(
	const size_t camIdx //=0
)
{
	ProcessDepth(camIdx);
}


void cFilterView::ProcessDepth(const size_t camIdx //=0
)
{
	m_contours.clear();
	m_removeRects.clear();
	cv::Mat &srcImg = g_root.m_sensorBuff[camIdx].m_srcImg;

	if ((srcImg.cols != m_depthTexture.Width()) || (srcImg.rows != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), srcImg.cols, srcImg.rows, DXGI_FORMAT_R32G32B32_FLOAT);
	}

	Mat grayscaleMat;
	srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);
	srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	{
		cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[i];

		float threshold = areaFloor->startIdx * 0.1f;

		cv::threshold(grayscaleMat, m_binImg, threshold, 255, cv::THRESH_BINARY);
		m_binImg.convertTo(m_binImg, CV_8UC1);

		cv::erode(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);
		cv::erode(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);
		cv::dilate(m_binImg, m_binImg, element);

		bool isFindBox = false;
		for (int vtxCnt = 4; !isFindBox && (vtxCnt < 9); ++vtxCnt)
		{
			int loopCnt = 0;
			cv::Mat binImg = m_binImg.clone();
			while (!isFindBox && (loopCnt < 8))
			{
				vector<cContour> temp;
				if (FindBox(binImg, vtxCnt, temp))
				{
					// 원래 크기로 되돌린다.
					cv::Mat img2 = binImg.clone();
					for (int k = 0; k < loopCnt; ++k)
						cv::erode(img2, img2, element);

					vector<cContour> out;
					if (FindBox(img2, vtxCnt, out))
					{
						for (auto &contour : out)
						{
							//isFindBox = true;

							sContourInfo info;
							info.level = vtxCnt;
							info.loop = loopCnt;
							info.lowerH = 0;
							info.upperH = areaFloor->maxIdx * 0.1f;
							info.contour = contour;
							info.color = areaFloor->color;
							m_contours.push_back(info);
						}
					}
					//break;
				}

				cv::dilate(binImg, binImg, element);
				++loopCnt;
			}
		}
	}

	RemoveDuplicateContour(m_contours);

	// display remove rect
	for (auto &info : m_removeRects)
	{
		cRectContour &rect = info.r;
		const Scalar color(255, 0, 0);
		rect.Draw(m_dstImg, color, 1);
	}

	// Display Detect Box (for debugging)
	if (!m_contours.empty() && !m_binImg.empty())
		cvtColor(m_binImg, m_binImg, cv::COLOR_GRAY2RGB);

	g_root.m_boxes.clear();
	for (u_int i=0; i < m_contours.size(); ++i)
	{
		auto &info = m_contours[i];
		const Scalar color(255, 255, 255);
		const Vector3 color2 = info.color.GetColor() * 255;
		info.contour.Draw(m_dstImg, cv::Scalar(color2.x, color2.y, color2.z));
		
		char boxName[64];
		sprintf(boxName, "BOX%d", i + 1);
		setLabel(m_dstImg, boxName, info.contour.m_data, Scalar(1, 1, 1));
		//setLabel(m_dstImg, " 1", rect.At(0), color);
		//setLabel(m_dstImg, " 2", rect.At(1), color);
		//setLabel(m_dstImg, " 3", rect.At(2), color);
		//setLabel(m_dstImg, " 4", rect.At(3), color);

		//cv::circle(m_dstImg, rect.At(0), 5, color);
		//cv::circle(m_dstImg, rect.At(1), 5, color);
		//cv::circle(m_dstImg, rect.At(2), 5, color);
		//cv::circle(m_dstImg, rect.At(3), 5, color);
		
		//rect.Draw(m_dstImg, color, 1);
		//rect.Draw(m_binImg, Scalar(255,0,0), 1); // for debugging

		//const float scale = 50.f / 110.f;
		const float scale = 50.f / 73.2f;
		const float offsetY = ((info.lowerH <= 0) && g_root.m_isPalete) ? -13.f : 2.5f;

		cRoot::sBoxInfo box;

		if (info.contour.Size() == 4)
		{
			const Vector2 v1((float)info.contour[0].x, (float)info.contour[0].y);
			const Vector2 v2((float)info.contour[1].x, (float)info.contour[1].y);
			const Vector2 v3((float)info.contour[2].x, (float)info.contour[2].y);
			const Vector2 v4((float)info.contour[3].x, (float)info.contour[3].y);
		
			// maximum value
			const float l1 = std::max((v1 - v2).Length(), (v3 - v4).Length());
			const float l2 = std::max((v2 - v3).Length(), (v4 - v1).Length());
			box.volume.x = std::max(l1, l2);
			box.volume.y = (info.upperH - info.lowerH) + offsetY;
			box.volume.z = std::min(l1, l2);

			// average value
			//box.volume.x = (((v1 - v2).Length()) + ((v3 - v4).Length())) * 0.5f;
			//box.volume.y = (info.upperH - info.lowerH) + offsetY;
			//box.volume.z = (((v2 - v3).Length()) + ((v4 - v1).Length())) * 0.5f;

			box.volume.x *= scale;
			box.volume.y *= 1.f;
			box.volume.z *= scale;

			//box.volume.x -= (float)info.loop*1.4f;
			//box.volume.z -= (float)info.loop*1.4f;

			box.minVolume = box.volume.x * box.volume.y * box.volume.z;
			box.maxVolume = box.minVolume;
			box.loopCnt = info.loop;
		}
		else
		{
			box.volume.x *= 0;
			box.volume.y = (info.upperH - info.lowerH) + offsetY;
			box.volume.z *= 0;

			box.minVolume = (float)info.contour.Area() * scale * scale * box.volume.y;
			box.maxVolume = box.minVolume;
			box.loopCnt = info.loop;
		}

		for (u_int i = 0; i < info.contour.Size(); ++i)
		{
			box.box3d[i] = Vector3((info.contour[i].x - 320) / 1.5f, info.upperH, (480 - info.contour[i].y - 240) / 1.5f);
			box.box3d[i + info.contour.Size()] = Vector3((info.contour[i].x - 320) / 1.5f, info.lowerH, (480 - info.contour[i].y - 240) / 1.5f);
		}

		box.color = info.color;
		box.pointCnt = info.contour.Size();
		g_root.m_boxes.push_back(box);
	}

	UpdateTexture();
}


// 꼭지점갯수가 vtxCnt와 같을 때, 박스를 검출한다.
bool cFilterView::FindBox(cv::Mat &img
	, const u_int vtxCnt
	, OUT vector<cContour> &out)
{
	vector<vector<cv::Point>> contours;
	findContours(img, contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	int m_minArea = 300;
	double m_minCos = -0.4f; // 직각 체크
	double m_maxCos = 0.4f;

	vector<cRectContour> rects;
	std::vector<cv::Point> approx;
	for (u_int i = 0; i < contours.size(); i++)
	{
		// Approximate contour with accuracy proportional
		// to the contour perimeter
		cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true)*0.02f, true);

		// Skip small or non-convex objects 
		const bool isConvex = cv::isContourConvex(approx);
		//if (std::fabs(cv::contourArea(contours[i])) < m_minArea || !isConvex)
		if (std::fabs(cv::contourArea(contours[i])) < m_minArea)
			continue;

		//if (approx.size() == 4)
		if (approx.size() <= vtxCnt)
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
			if (
				//(vtc == 4) && 
				(mincos >= m_minCos) && (maxcos <= m_maxCos))
			{
				cContour contour;
				contour.Init(approx);
				out.push_back(contour);

				//contour.Draw(m_dstImg, Scalar(0, 255, 0), 1);
				//AddLog("Detect Box");
				//AddLog(common::format("pos1 = %d, %d", approx[0].x, approx[0].y));
				//AddLog(common::format("pos2 = %d, %d", approx[1].x, approx[1].y));
				//AddLog(common::format("pos3 = %d, %d", approx[2].x, approx[2].y));
				//AddLog(common::format("pos4 = %d, %d", approx[3].x, approx[3].y));

				//const Vector2 v1((float)approx[0].x, (float)approx[0].y);
				//const Vector2 v2((float)approx[1].x, (float)approx[1].y);
				//const Vector2 v3((float)approx[2].x, (float)approx[2].y);
				//const Vector2 v4((float)approx[3].x, (float)approx[3].y);
				//AddLog(common::format("pos1-2 len = %f", (v1 - v2).Length()));
				//AddLog(common::format("pos2-3 len = %f", (v2 - v3).Length()));
				//AddLog(common::format("pos3-4 len = %f", (v3 - v4).Length()));
				//AddLog(common::format("pos4-1 len = %f", (v4 - v1).Length()));
			}
		}
	}

	return !out.empty();
}


void cFilterView::RemoveDuplicateContour(vector<sContourInfo> &contours)
{
	// Box 중복인식 제거
	if (contours.empty())
		return;

	for (auto &contour : contours)
		contour.used = true;

	for (u_int i = 0; i < contours.size() - 1; ++i)
	{
		sContourInfo &contour1 = contours[i];
		if (!contour1.used)
			continue;

		for (u_int k = i + 1; k < contours.size(); ++k)
		{
			sContourInfo &contour2 = contours[k];
			if (!contour2.used)
				continue;

			if (contour1.contour.IsContain(contour2.contour))
			{
				const int a1 = contour1.contour.Area();
				const int a2 = contour2.contour.Area();
				const bool isDuplciate = abs(((float)a1 / (float)a2) - 1.f) < 0.1f; // 거의 같은 사이즈
				contour2.duplicate = isDuplciate;

				// 중복 인식된 박스일 때, 더 낮은 높이의 박스를 제거한다.
				// 높이가 거의 같다면, 더 적은 loop로 발견된 박스를 남기고, 나머지를 제거한다.
				if (isDuplciate)
				{
					if (contour1.upperH == contour2.upperH)
					{
						// 더 적은 loop로 발견된 박스를 남기고, 나머지를 제거한다.
						if ( (contour1.level < contour2.level)
							|| ((contour1.level == contour2.level) && (contour1.loop < contour2.loop)))
						{
							contour2.used = false;
						}
						else
						{
							contour1.used = false;
						}
					}
					else
					{
						// 더 낮은 높이의 박스를 제거한다.
						if (contour1.upperH > contour2.upperH)
						{
							contour2.used = false;
						}
						else
						{
							contour1.used = false;
						}
					}
				}
			}
		}
	}


	for (u_int i = 0; i < contours.size() - 1; ++i)
	{
		sContourInfo &contour1 = contours[i];
		if (!contour1.used)
			continue;

		for (u_int k = i + 1; k < contours.size(); ++k)
		{
			sContourInfo &contour2 = contours[k];
			if (!contour2.used)
				continue;

			if (contour1.contour.IsContain(contour2.contour))
			{
				const int a1 = contour1.contour.Area();
				const int a2 = contour2.contour.Area();

				// 두개의 박스가 위아래로 겹쳐져 있을 때,
				// 박스바닥 높이를 계산한다.
				// 면적이 큰 박스가 아래에 있는 것으로 가정한다.
				if (a1 < a2)
				{
					contour1.lowerH = contour2.upperH;
				}
				else
				{
					contour2.lowerH = contour1.upperH;
				}

				// 박스 높이가 너무 작으면 제거한다.
				if (abs(contour1.upperH - contour1.lowerH) < 5)
					contour1.used = false;
				if (abs(contour2.upperH - contour2.lowerH) < 5)
					contour2.used = false;
			}
		}
	}


	vector<sContourInfo> temp;
	for (auto &contour : contours)
		if (contour.used)
			temp.push_back(contour);

	contours.clear();
	contours = temp;
}


// Mat -> DX11 Texture
void cFilterView::UpdateTexture()
{
	const int nWidth = m_depthTexture.Width();
	const int nHeight = m_depthTexture.Height();

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
