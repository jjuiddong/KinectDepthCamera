
#include "stdafx.h"
#include "filterview.h"
#include "cvutil.h"

using namespace graphic;
using namespace framework;
using namespace cv;
using namespace common;


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


void cFilterView::Process()
{
	ProcessDepth();
}


void ThreadFilterAllFunc(cFilterView *fview, bool isCalcHorz)
{
	cv::Mat &srcImg = g_root.m_projMap;

	vector<sContourInfo> contours; // 현재 인식된 박스 정보
	Mat mbinImg;
	Mat grayscaleMat;
	srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);

	const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	{
		cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[i];

		float threshold1 = areaFloor->startIdx * 0.1f;
		//float threshold2 = areaFloor->endIdx * 0.11f;
		float threshold2 = areaFloor->endIdx * 0.105f;

		//if (g_root.m_isCalcHorz)
		if (isCalcHorz)
		{
			cv::threshold(grayscaleMat, mbinImg, threshold1, 255, cv::THRESH_BINARY);
		}
		else
		{
			Mat bin1;
			cv::threshold(grayscaleMat, bin1, threshold2, 255, cv::THRESH_BINARY_INV);
			cv::threshold(grayscaleMat, mbinImg, threshold1, 255, cv::THRESH_BINARY);
			mbinImg &= bin1;
		}

		mbinImg.convertTo(mbinImg, CV_8UC1);

		cv::erode(mbinImg, mbinImg, element);
		cv::dilate(mbinImg, mbinImg, element);
		cv::erode(mbinImg, mbinImg, element);
		cv::dilate(mbinImg, mbinImg, element);
		cv::dilate(mbinImg, mbinImg, element);

		bool isFindBox = false;
		for (int vtxCnt = 4; !isFindBox && (vtxCnt < 9); ++vtxCnt)
		{
			int loopCnt = 0;
			cv::Mat binImg = mbinImg.clone();
			while (!isFindBox && (loopCnt < 8))
			{
				vector<cContour> temp;
				if (fview->FindBox(binImg, vtxCnt, temp))
				{
					// 원래 크기로 되돌린다.
					cv::Mat img2 = binImg.clone();
					for (int k = 0; k < loopCnt; ++k)
						cv::erode(img2, img2, element);

					vector<cContour> out;
					if (fview->FindBox(img2, vtxCnt, out))
					{
						for (auto &contour : out)
						{
							sContourInfo info;
							info.level = vtxCnt;
							info.loop = loopCnt;
							info.lowerH = 0;
							const float scaleH = 0.99f;
							const float offsetY = (g_root.m_isPalete) ? -13.f : 3.5f;

							info.upperH = (areaFloor->maxIdx * 0.1f) * scaleH + offsetY;
							info.contour = contour;
							info.color = areaFloor->color;
							contours.push_back(info);
						}
					}
				}

				cv::dilate(binImg, binImg, element);
				++loopCnt;
			}
		}
	}


	{
		AutoCSLock cs(fview->m_cs);
		for (auto &ct : contours)
			fview->m_contours.push_back(ct);
	}
}



void ThreadFilterSubFunc(cFilterView *fview, cRoot::sAreaFloor *areaFloor, bool isCalcHorz)
{
	cv::Mat &srcImg = g_root.m_projMap;

	vector<sContourInfo> contours; // 현재 인식된 박스 정보
	Mat mbinImg;
	Mat grayscaleMat;
	srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);

	const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));

	float threshold1 = areaFloor->startIdx * 0.1f;
	float threshold2 = areaFloor->endIdx * 0.11f;

	//if (g_root.m_isCalcHorz)
	if (isCalcHorz)
	{
		cv::threshold(grayscaleMat, mbinImg, threshold1, 255, cv::THRESH_BINARY);
	}
	else
	{
		Mat bin1;
		cv::threshold(grayscaleMat, bin1, threshold2, 255, cv::THRESH_BINARY_INV);
		cv::threshold(grayscaleMat, mbinImg, threshold1, 255, cv::THRESH_BINARY);
		mbinImg &= bin1;
	}

	mbinImg.convertTo(mbinImg, CV_8UC1);

	cv::erode(mbinImg, mbinImg, element);
	cv::dilate(mbinImg, mbinImg, element);
	cv::erode(mbinImg, mbinImg, element);
	cv::dilate(mbinImg, mbinImg, element);
	cv::dilate(mbinImg, mbinImg, element);

	bool isFindBox = false;
	for (int vtxCnt = 4; !isFindBox && (vtxCnt < 9); ++vtxCnt)
	{
		int loopCnt = 0;
		cv::Mat binImg = mbinImg.clone();
		while (!isFindBox && (loopCnt < 8))
		{
			vector<cContour> temp;
			if (fview->FindBox(binImg, vtxCnt, temp))
			{
				// 원래 크기로 되돌린다.
				cv::Mat img2 = binImg.clone();
				for (int k = 0; k < loopCnt; ++k)
					cv::erode(img2, img2, element);

				vector<cContour> out;
				if (fview->FindBox(img2, vtxCnt, out))
				{
					for (auto &contour : out)
					{
						sContourInfo info;
						info.level = vtxCnt;
						info.loop = loopCnt;
						info.lowerH = 0;

						const float scaleH = 0.99f;
						const float offsetY = (g_root.m_isPalete) ? -13.f : 3.5f;

						info.upperH = (areaFloor->maxIdx * 0.1f) * scaleH + offsetY;


						info.contour = contour;
						info.color = areaFloor->color;
						contours.push_back(info);
					}
				}
			}

			cv::dilate(binImg, binImg, element);
			++loopCnt;
		}
	}


	{
		AutoCSLock cs(fview->m_cs);
		for (auto &ct : contours)
			fview->m_contours.push_back(ct);
	}
}



// 영상인식해서 박스의 모양을 인식한다.
void cFilterView::ProcessDepth()
{
	m_contours.clear();
	m_removeRects.clear();
	cv::Mat &srcImg = g_root.m_projMap;

	if ((srcImg.cols != m_depthTexture.Width()) || (srcImg.rows != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), srcImg.cols, srcImg.rows, DXGI_FORMAT_R32G32B32_FLOAT);
	}

	srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	std::thread th1 = std::thread(ThreadFilterAllFunc, this, true);
	std::thread th2 = std::thread(ThreadFilterAllFunc, this, false);
	th1.join();
	th2.join();

	//vector<std::thread*> threads;

	//for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	//{
	//	cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[i];
	//	std::thread *pth1 = new std::thread(ThreadFilterSubFunc, this, areaFloor, true);
	//	std::thread *pth2 = new std::thread(ThreadFilterSubFunc, this, areaFloor, false);

	//	threads.push_back(pth1);
	//	threads.push_back(pth2);
	//}

	//for (auto &th : threads)
	//	th->join();

	//for (auto &th : threads)
	//	delete th;
	//threads.clear();


	//Mat grayscaleMat;
	//srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);
	//srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	//cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	//const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	//for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	//{
	//	cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[i];

	//	float threshold1 = areaFloor->startIdx * 0.1f;
	//	float threshold2 = areaFloor->endIdx * 0.11f;

	//	if (g_root.m_isCalcHorz)
	//	{
	//		cv::threshold(grayscaleMat, m_binImg, threshold1, 255, cv::THRESH_BINARY);
	//	}
	//	else
	//	{
	//		Mat bin1;
	//		cv::threshold(grayscaleMat, bin1, threshold2, 255, cv::THRESH_BINARY_INV);
	//		cv::threshold(grayscaleMat, m_binImg, threshold1, 255, cv::THRESH_BINARY);
	//		m_binImg &= bin1;
	//	}

	//	m_binImg.convertTo(m_binImg, CV_8UC1);

	//	cv::erode(m_binImg, m_binImg, element);
	//	cv::dilate(m_binImg, m_binImg, element);
	//	cv::erode(m_binImg, m_binImg, element);
	//	cv::dilate(m_binImg, m_binImg, element);
	//	cv::dilate(m_binImg, m_binImg, element);

	//	bool isFindBox = false;
	//	for (int vtxCnt = 4; !isFindBox && (vtxCnt < 9); ++vtxCnt)
	//	{
	//		int loopCnt = 0;
	//		cv::Mat binImg = m_binImg.clone();
	//		while (!isFindBox && (loopCnt < 8))
	//		{
	//			vector<cContour> temp;
	//			if (FindBox(binImg, vtxCnt, temp))
	//			{
	//				// 원래 크기로 되돌린다.
	//				cv::Mat img2 = binImg.clone();
	//				for (int k = 0; k < loopCnt; ++k)
	//					cv::erode(img2, img2, element);

	//				vector<cContour> out;
	//				if (FindBox(img2, vtxCnt, out))
	//				{
	//					for (auto &contour : out)
	//					{
	//						sContourInfo info;
	//						info.level = vtxCnt;
	//						info.loop = loopCnt;
	//						info.lowerH = 0;
	//						info.upperH = areaFloor->maxIdx * 0.1f;
	//						info.contour = contour;
	//						info.color = areaFloor->color;
	//						m_contours.push_back(info);
	//					}
	//				}
	//			}

	//			cv::dilate(binImg, binImg, element);
	//			++loopCnt;
	//		}
	//	}
	//}


	RemoveDuplicateContour(m_contours);

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
	
		cRoot::sBoxInfo box = CalcBoxInfo(info);
		g_root.m_boxes.push_back(box);
	}

	UpdateTexture();
}


// 영상에서 인식된 픽셀 정보로 박스 실제 사이즈를 계산한다.
cRoot::sBoxInfo cFilterView::CalcBoxInfo(const sContourInfo &info)
{
	//const float scale = 50.f / 73.2f;
	//const float scale = 50.f / 74.5f;
	const float scale = 50.f / 72.3f;
	//const float scaleH = 0.99f;
	//const float offsetY = ((info.lowerH <= 0) && g_root.m_isPalete) ? -13.f : 3.5f;

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
		box.volume.x = std::max(l1, l2); // 큰 값이 가로
		//box.volume.y = (info.upperH - info.lowerH) * scaleH + offsetY;
		box.volume.y = (info.upperH - info.lowerH);
		box.volume.z = std::min(l1, l2); // 작은 값이 세로

		// average value
		//box.volume.x = (((v1 - v2).Length()) + ((v3 - v4).Length())) * 0.5f;
		//box.volume.y = (info.upperH - info.lowerH) + offsetY;
		//box.volume.z = (((v2 - v3).Length()) + ((v4 - v1).Length())) * 0.5f;

		box.volume.x *= scale;
		box.volume.y *= 1.f;
		box.volume.z *= scale;

		if ((box.volume.y < 40.f) && (box.volume.y > 1.f)) // 높이가 낮으면 오차가 커져서 없애준다.
			box.volume.y -= 1.f;

		//box.volume.x -= (float)info.loop*1.4f;
		//box.volume.z -= (float)info.loop*1.4f;

		box.minVolume = box.volume.x * box.volume.y * box.volume.z;
		box.maxVolume = box.minVolume;
		box.loopCnt = info.loop;
	}
	else
	{
		box.volume.x *= 0;
		//box.volume.y = (info.upperH - info.lowerH) * scaleH + offsetY;
		box.volume.y = (info.upperH - info.lowerH);
		box.volume.z *= 0;

		if ((box.volume.y < 40.f) && (box.volume.y > 1.f)) // 높이가 낮으면 오차가 커져서 없애준다.
			box.volume.y -= 1.f;

		box.minVolume = (float)info.contour.Area() * scale * scale * box.volume.y;
		box.maxVolume = box.minVolume;
		box.loopCnt = info.loop;
	}

	// 3차원 박스 정보 설정
	for (u_int i = 0; i < info.contour.Size(); ++i)
	{
		box.box3d[i] = Vector3((info.contour[i].x - 320) / 1.5f, info.upperH, (480 - info.contour[i].y - 240) / 1.5f);
		box.box3d[i + info.contour.Size()] = Vector3((info.contour[i].x - 320) / 1.5f, info.lowerH, (480 - info.contour[i].y - 240) / 1.5f);
	}

	box.color = info.color;
	box.pointCnt = info.contour.Size();
	return box;
}


// 일정 시간동안 박스크기 평균값을 구한다.
void cFilterView::CalcBoxVolumeAverage()
{
	const float max_center_gap = 10.f; // 10 cm
	const float max_vertex_gap = 10.f; // 10 cm

	cSensor *sensor1 = NULL;
	for (auto s : g_root.m_baslerCam.m_sensors)
		if (s->IsEnable() && s->m_isShow)
			sensor1 = s;
	if (!sensor1)
		return;

	// 정보가 업데이트 되지 않았거나, 오차가 크면, 계산하지 않는다.
	// 하나라도 다른 정보가 있다면, 계산한다.
	// 하나라도 오차가 큰정보가 있다면, 종료한다.
	for (auto s : g_root.m_baslerCam.m_sensors)
		if (s->IsEnable() && s->m_isShow)
			if (s->m_buffer.m_diffAvrs.GetCurValue() > 0.3f)
				return;
	
	bool isCalc = false;
	for (auto s : g_root.m_baslerCam.m_sensors)
		if (s->IsEnable() && s->m_isShow)
			isCalc |= (s->m_buffer.m_diffAvrs.GetCurValue() != 0);

	if (!isCalc)
	{
		//dbg::Logp("Error CalcBoxVolumeAverage(), Same Data \n");
		//return; 같은 데이타라도 검사한다. 네트워크 딜레이때문에, 정보를 못 받을 수도 있다.
	}

	//if (g_root.m_sensorBuff[0].m_diffAvrs.GetCurValue() == 0)
	//	return;
	//if (g_root.m_sensorBuff[0].m_diffAvrs.GetCurValue() > 0.3f)
	//	return;
	if (m_contours.empty())
	{
		static int cnt = 0;
		//dbg::Logp("Error CalcBoxVolumeAverage(), No Detect Box %d\n", cnt++);
		return;
	}

	for (auto &info : m_avrContours)
		info.check = false;

	for (u_int i = 0; i < m_contours.size(); ++i)
	{
		sContourInfo &srcBox = m_contours[i];
		cv::Point center0 = srcBox.contour.Center();
		Vector3 c0((float)center0.x, srcBox.upperH, (float)center0.y);

		int boxIdx = -1;
		for (u_int k = 0; k < m_avrContours.size(); ++k)
		{
			sAvrContour &avrBox = m_avrContours[k];
			if (avrBox.check)
				continue;

			// 꼭지점 갯수가 다르면, 검색 실패
			if (srcBox.contour.Size() != avrBox.box.contour.Size())
				continue;

			cv::Point center1 = avrBox.box.contour.Center();
			Vector3 c1((float)center1.x, avrBox.box.upperH, (float)center1.y);

			// 박스 중점이 가까우면, 같은 박스인것으로 취급
			const float len = (c0 - c1).Length();
			if (len > max_center_gap)
				continue;

			avrBox.check = true;
			boxIdx = (int)k;
			break;
		}

		if (boxIdx < 0) // Not Found
		{
			sAvrContour box;
			box.check = true;
			box.count = 1;
			box.box = srcBox;
			for (u_int k = 0; k < srcBox.contour.Size(); ++k)
			{
				box.avrVertices[k] = Vector2((float)srcBox.contour[k].x, (float)srcBox.contour[k].y);
				
				box.vertices3d[k] = Vector3((box.avrVertices[k].x - 320) / 1.5f, box.box.upperH, (480 - box.avrVertices[k].y - 240) / 1.5f);
				box.vertices3d[k + box.box.contour.Size()] = Vector3((box.avrVertices[k].x - 320) / 1.5f, box.box.lowerH, (480 - box.avrVertices[k].y - 240) / 1.5f);
			}

			// BoxInfo 정보 업데이트
			sContourInfo tempInfo = srcBox;
			for (u_int k = 0; k < srcBox.contour.Size(); ++k)
				tempInfo.contour.m_data[k] = cv::Point((int)box.avrVertices[k].x, (int)box.avrVertices[k].y);
			box.result = CalcBoxInfo(tempInfo);
			m_avrContours.push_back(box);
			continue;
		}

		// box 꼭지점 검색
		// 각 박스의 꼭지점에 대응하는 avr box의 꼭지점을 찾아서, 꼭지점 위치를 평균한다.
		sAvrContour &avrBox = m_avrContours[boxIdx];
		++avrBox.count;

		for (u_int k = 0; k < srcBox.contour.Size(); ++k)
		{
			cv::Point vtx0 = srcBox.contour[k];
			Vector2 p0((float)vtx0.x, (float)vtx0.y);

			// 가장 가까운 꼭지점 찾기
			float minLen = FLT_MAX;
			int vtxIdx = -1;
			for (u_int m = 0; m < avrBox.box.contour.Size(); ++m)
			{
				cv::Point vtx1 = avrBox.box.contour[m];
				Vector2 p1((float)vtx1.x, (float)vtx1.y);
				const float len = (p1 - p0).Length();
				if (minLen > len)
				{
					minLen = len;
					vtxIdx = m;
				}
			}

			if (vtxIdx < 0)
				continue;
			if (minLen > max_vertex_gap)
				continue;

			// 대응되는 꼭지점을 찾았으면, 위치를 평균해서 저장한다. 재귀평균식 적용
			avrBox.avrVertices[vtxIdx].x = (float)CalcAverage(avrBox.count, avrBox.avrVertices[vtxIdx].x, p0.x);
			avrBox.avrVertices[vtxIdx].y = (float)CalcAverage(avrBox.count, avrBox.avrVertices[vtxIdx].y, p0.y);

			avrBox.vertices3d[vtxIdx] = Vector3((avrBox.avrVertices[vtxIdx].x - 320) / 1.5f
				, avrBox.box.upperH, (480 - avrBox.avrVertices[vtxIdx].y - 240) / 1.5f);
			avrBox.vertices3d[vtxIdx + avrBox.box.contour.Size()] = Vector3((avrBox.avrVertices[vtxIdx].x - 320) / 1.5f
				, avrBox.box.lowerH, (480 - avrBox.avrVertices[vtxIdx].y - 240) / 1.5f);

			//for (u_int m = 0; m < avrBox.box.contour.Size(); ++m)
			//{
			//	cv::Point vtx1 = avrBox.box.contour[m];
			//	Vector2 p1((float)vtx1.x, (float)vtx1.y);

			//	const float len = (p1 - p0).Length();
			//	if (len > max_vertex_gap)
			//		continue; // 꼭지점 사이 거리가 멀면, 다음 꼭지점 검색

			//	// 대응되는 꼭지점을 찾았으면, 위치를 평균해서 저장한다. 재귀평균식 적용
			//	avrBox.avrVertices[m].x = (float)CalcAverage(avrBox.count, avrBox.avrVertices[m].x, p0.x);
			//	avrBox.avrVertices[m].y = (float)CalcAverage(avrBox.count, avrBox.avrVertices[m].y, p0.y);

			//	avrBox.vertices3d[m] = Vector3((avrBox.avrVertices[m].x - 320) / 1.5f, avrBox.box.upperH, (480 - avrBox.avrVertices[m].y - 240) / 1.5f);
			//	avrBox.vertices3d[m + avrBox.box.contour.Size()] = Vector3((avrBox.avrVertices[m].x - 320) / 1.5f, avrBox.box.lowerH, (480 - avrBox.avrVertices[m].y - 240) / 1.5f);
			//}
		}

		// BoxInfo 정보 업데이트
		sContourInfo tempInfo = avrBox.box;
		for (u_int k = 0; k < avrBox.box.contour.Size(); ++k)
			tempInfo.contour.m_data[k] = cv::Point((int)avrBox.avrVertices[k].x, (int)avrBox.avrVertices[k].y);
		avrBox.result = CalcBoxInfo(tempInfo);
	}

	++m_calcAverageCount;
}


void cFilterView::ClearBoxVolumeAverage()
{
	m_avrContours.clear();
	m_calcAverageCount = 0;
}


// 꼭지점갯수가 vtxCnt와 같을 때, 박스를 검출한다.
bool cFilterView::FindBox(cv::Mat &img
	, const u_int vtxCnt
	, OUT vector<cContour> &out)
{
	vector<vector<cv::Point>> contours;
	findContours(img, contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	int m_minArea = 300;
	//double m_minCos = -0.4f; // 직각 체크
	//double m_maxCos = 0.4f;
	double m_minCos = (vtxCnt == 4)? -0.4f : -0.3f; // 직각 체크
	double m_maxCos = (vtxCnt == 4)? 0.4f : 0.3f;

	vector<cRectContour> rects;
	std::vector<cv::Point> approx;
	for (u_int i = 0; i < contours.size(); i++)
	{
		// Approximate contour with accuracy proportional
		// to the contour perimeter
		//double ll = cv::arcLength(cv::Mat(contours[i]), true);
		const double area = cv::contourArea(contours[i]);

		//const double epsilontAlpha = (vtxCnt == 4) ? 0.03f : 0.01f;
		const double epsilontAlpha = (area < 3000.f) ? 0.03f : 0.015f;
		cv::approxPolyDP(cv::Mat(contours[i]), approx
			, cv::arcLength(cv::Mat(contours[i]), true) * epsilontAlpha, true);
		//, cv::arcLength(cv::Mat(contours[i]), true) * 0.013f, true);
		//, cv::arcLength(cv::Mat(contours[i]), true) * 0.03f , true);

		// Skip small or non-convex objects 
		const bool isConvex = cv::isContourConvex(approx);
		//if (std::fabs(cv::contourArea(contours[i])) < m_minArea || !isConvex)
		if (std::fabs(cv::contourArea(contours[i])) < m_minArea)
			continue;

		//m_minCos = (area < 3000.f) ? -0.4f : -0.23f; // 직각 체크
		//m_maxCos = (area < 3000.f) ? 0.4f : 0.23f;
		m_minCos = (area < 3000.f) ? -0.4f : -0.33f; // 직각 체크
		m_maxCos = (area < 3000.f) ? 0.4f : 0.33f;

		if ((approx.size() <= vtxCnt) && (approx.size() >= 4))
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
				contour.m_maxCos = maxcos;
				contour.m_minCos = mincos;
				out.push_back(contour);
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
						// 가장 사각형과 비슷한 모양을 남기고, 나머지를 제거한다.
						//old: 더 적은 loop로 발견된 박스를 남기고, 나머지를 제거한다.
						const double cos1 = std::max(abs(contour1.contour.m_minCos), abs(contour1.contour.m_maxCos));
						const double cos2 = std::max(abs(contour2.contour.m_minCos), abs(contour2.contour.m_maxCos));

						if ( //(contour1.level < contour2.level)
							//|| ((contour1.level == contour2.level) && (contour1.loop < contour2.loop)))
							cos1 < cos2)
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


	// 박스 높이 조정
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
