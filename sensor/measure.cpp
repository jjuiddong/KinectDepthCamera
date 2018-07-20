
#include "stdafx.h"
#include "measure.h"
#include "../view/3dview.h"
#include <opencv2/imgproc/imgproc.hpp>


using namespace graphic;
using namespace framework;
using namespace cv;
using namespace common;


cMeasure::cMeasure()
	: m_distribCount(0)
	, m_areaCount(0)
	, m_areaFloorCnt(0)
	, m_measureId(0)
{
	m_projMap = cv::Mat((int)g_capture3DHeight, (int)g_capture3DWidth, CV_32FC1);

	ZeroMemory(m_volDistrib, sizeof(m_volDistrib));
	ZeroMemory(m_horzDistrib, sizeof(m_horzDistrib));
	ZeroMemory(m_vertDistrib, sizeof(m_vertDistrib));
	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));
}

cMeasure::~cMeasure()
{
	Clear();
}


void ThreadFilterAllFunc(cMeasure *fview, bool isCalcHorz)
{
	cv::Mat &srcImg = g_root.m_measure.m_projMap;

	vector<sContourInfo> contours; // ���� �νĵ� �ڽ� ����
	Mat mbinImg;
	Mat grayscaleMat;
	srcImg.convertTo(grayscaleMat, CV_16UC1, 500.0f);

	const Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	for (int i = 0; i < g_root.m_measure.m_areaFloorCnt; ++i)
	{
		sAreaFloor *areaFloor = g_root.m_measure.m_areaBuff[i];

		float threshold1 = areaFloor->startIdx * 0.1f - 1;
		//float threshold2 = areaFloor->endIdx * 0.11f;
		float threshold2 = areaFloor->endIdx * 0.105f + 1;

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

		for (int vtxCnt = 4; vtxCnt < 9; ++vtxCnt)
		{
			int loopCnt = 0;
			cv::Mat binImg = mbinImg.clone();
			while (loopCnt < 14)
			{
				if (isCalcHorz && (vtxCnt == 4))
				{
					int a = 0;
				}

				if (g_root.m_isSave2DMat)
				{
					// save mat file
					StrPath path;
					path.Format("filter/mat_calc%d_f%d_v%d_l%d.jpg", isCalcHorz, i, vtxCnt, loopCnt);
					cv::imwrite(path.c_str(), binImg);
				}

				vector<cContour> temp;
				if (fview->FindBox(binImg, vtxCnt, temp))
				{
					// ���� ũ��� �ǵ�����.
					cv::Mat img2 = binImg.clone();
					for (int k = 0; k < loopCnt; ++k)
						cv::erode(img2, img2, element);

					if (g_root.m_isSave2DMat)
					{
						// save mat file
						StrPath path;
						path.Format("filter/mat_after_calc%d_f%d_v%d_l%d.jpg", isCalcHorz, i, vtxCnt, loopCnt);
						cv::imwrite(path.c_str(), img2);
					}

					vector<cContour> out;
					if (fview->FindBox(img2, vtxCnt, out))
					{
						for (auto &contour : out)
						{
							// ���� contour�� �����Ѵٸ� ������.
							bool isSame = false;
							for (auto &c : contours)
							{
								if (c.contour == contour)
								{
									isSame = true;
									break; // ignore this contour
								}
							}
							if (isSame)
								continue;
							//

							sContourInfo info;
							info.level = vtxCnt;
							info.loop = loopCnt;
							info.lowerH = 0;
							//const float scaleH = 0.99f;
							//const float offsetY = (g_root.m_isPalete) ? -13.f : 3.5f;

							//info.upperH = (areaFloor->maxIdx * 0.1f) * scaleH + offsetY;
							//info.upperH = (areaFloor->maxIdx * 0.1f);
							info.upperH = areaFloor->avrHeight;
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
		{
			// ���� contour�� �����Ѵٸ� ������.
			bool isSame = false;
			for (auto &c : fview->m_contours)
			{
				if (c.contour == ct.contour)
				{
					isSame = true;
					break; // ignore this contour
				}
			}
			if (isSame)
				continue;
			//

			fview->m_contours.push_back(ct);
		}
	}
}


void ThreadFilterSubFunc(cMeasure *fview, sAreaFloor *areaFloor, bool isCalcHorz)
{
	cv::Mat &srcImg = g_root.m_measure.m_projMap;

	vector<sContourInfo> contours; // ���� �νĵ� �ڽ� ����
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
				// ���� ũ��� �ǵ�����.
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

						//const float scaleH = 0.99f;
						//const float offsetY = (g_root.m_isPalete) ? -13.f : 3.5f;

						//info.upperH = (areaFloor->maxIdx * 0.1f) * scaleH + offsetY;
						info.upperH = areaFloor->avrHeight;

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


bool cMeasure::MeasureVolume(
	const bool isForceMeasure // = false
)
{
	m_areaFloorCnt = 0;

	if (g_root.m_isAutoMeasure || isForceMeasure)
		CalcHeightDistribute();

	Measure2DImage();
	CalcBoxVolumeAverage();

	return true;
}


// ���� ������ ���ؼ�, �ڽ��� ���̺��� �����Ѵ�.
void cMeasure::CalcHeightDistribute()
{
	// ����Ʈ Ŭ���忡�� ���� ������ ����Ѵ�.
	// ���̺����� �̿��ؼ� �������� �޽��� �����Ѵ�.
	// ���� ���� ����Ʈ Ŭ���带 �����Ѵ�.
	// ���� ������ ī�޶� �����Ѵ�. (�Ǽ�)
	cSensor *sensor = NULL;
	for (auto s : g_root.m_baslerCam.m_sensors)
	{
		if (s->IsEnable() && s->m_isShow && s->m_buffer.m_isLoaded)
			sensor = s;
	}
	if (!sensor)
		return;

	graphic::cRenderer &renderer = g_root.m_3dView->GetRenderer();
	cSensorBuffer &sbuff = sensor->m_buffer;

	// Calculate Height Distribution
	{
		m_distribCount = 0;
		ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
		ZeroMemory(m_hAverage, sizeof(m_hAverage));

		Vector3 *src = &sbuff.m_vertices[0];
		const u_int vertexSize = sbuff.m_vertices.size();
		for (u_int i=0; i < vertexSize; ++i)
		{
			const Vector3 &vtx = *src++;
			if (vtx.IsEmpty())
				continue;
			if (abs(vtx.x) > 200.f)
				continue;
			if (abs(vtx.z) > 200.f)
				continue;

			const int h = (int)(vtx.y * 10.f);
			if ((h >= 0) && (h < ARRAYSIZE(m_hDistrib)))
			{
				m_distribCount++;
				m_hDistrib[h] += 1.f;
				m_hAverage[h] += vtx.y;
			}
		}
	}

	// Calculate Height Average
	for (int i = 0; i < ARRAYSIZE(m_hAverage); ++i)
		if (m_hDistrib[i] > 0.f)
			m_hAverage[i] /= (float)m_hDistrib[i];


	// height distribute pulse
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	{
		//const float LIMIT_AREA = 30.f;
		const float LIMIT_AREA = 50.f;
		const float minArea = 60.f;
		float limitLowArea = LIMIT_AREA;
		int state = 0; // 0: check, rising pulse, 1: check down pulse, 2: calc low height
		int startIdx = 0, endIdx = 0;
		int maxArea = 0;
		for (int i = 0; i < ARRAYSIZE(m_hDistrib); ++i)
		{
			const float a = m_hDistrib[i];

			switch (state)
			{
			case 0:
				if (a > minArea)
				{
					state = 1;
					startIdx = i;
					maxArea = i;
				}
				break;

			case 1:
				if (a > m_hDistrib[maxArea])
				{
					maxArea = i;
				}
				if (a < limitLowArea)
				{
					state = 2;
					endIdx = i;
				}
				break;

			case 2:
				state = 0;

				limitLowArea = std::max(LIMIT_AREA, m_hDistrib[maxArea] * 0.01f);
				// find first again
				for (int k = startIdx; k >= 0; --k)
				{
					if (m_hDistrib[k] < limitLowArea)
					{
						// ������ �����ؼ� ���� ��ȭ�� ���ų�, 
						// �ٽ� �ް��� ������ ������ ������, ���� �������� �Ѵ�.

						//float oldA = m_hDistrib[k];
						//for (int m = k-1; m >= 0; --m)
						//{
						//	float d = m_hDistrib[m] - oldA;
						//	if (d > 5) // ������ �ٽ� �����ϸ� ����.
						//	{
						//		startIdx = m;
						//		break;
						//	}
						//	oldA = m_hDistrib[m];
						//}
						//break;

						startIdx = k;
						break;
					}
				}

				for (int k = startIdx; k < endIdx; ++k)
					m_hDistrib2[k] = 1;
				m_hDistrib2[maxArea] = 2; // ���� ������ ū ���̿��� 2�� �����Ѵ�.

				limitLowArea = LIMIT_AREA; // recovery
				break;
			}
		}
	}

	// Generate Area Floor
	const cColor colors[] = { cColor::YELLOW, cColor::RED, cColor::GREEN, cColor::BLUE };

	u_int floor = 0;
	int state = 0; // 0: check rising pulse, 1: check down pulse, 2: collect area floor
	int startIdx = 0;
	int endIdx = 0;
	int maxAreaIdx = 0;
	for (int i = 0; i < ARRAYSIZE(m_hDistrib2); ++i)
	{
		switch (state)
		{
		case 0:
			// �޽� ��� üũ
			if (m_hDistrib2[i] > 0)
			{
				state = 1;
				startIdx = i;

				if (m_hDistrib2[i] > 1)
					maxAreaIdx = i;
			}
			break;

		case 1:
			// �޽� �ϰ� üũ
			if (m_hDistrib2[i] <= 0)
			{
				if ((maxAreaIdx == 0) || (startIdx < 100) || (i - startIdx > 400)) // ������ �ʹ�ũ�� ����
				{
					state = 0; // ���õǴ� �޽� (ù ��° �޽�)
				}
				else
				{
					state = 2; // �������� ����Ѵ�.
					endIdx = i;
				}
			}
			if (m_hDistrib2[i] > 1)
				maxAreaIdx = i; // ���� ������ ū ���� ����
		}

		if (state != 2)
			continue;
		state = 0;

		if (m_areaBuff.size() <= floor)
		{
			cVertexBuffer *vtxBuff = new cVertexBuffer();
			vtxBuff->Create(renderer, sbuff.m_width*sbuff.m_height, sizeof(sVertex), D3D11_USAGE_DYNAMIC);

			sAreaFloor *areaFloor = new sAreaFloor;
			areaFloor->vtxBuff = vtxBuff;
			m_areaBuff.push_back(areaFloor);
		}

		sAreaFloor *areaFloor = m_areaBuff[floor++];
		areaFloor->startIdx = startIdx;
		areaFloor->endIdx = endIdx;
		areaFloor->maxIdx = maxAreaIdx;
		areaFloor->avrHeight = m_hAverage[maxAreaIdx];
		areaFloor->areaCnt = 0;
		if ((floor - 1) < ARRAYSIZE(colors))
			areaFloor->color = colors[floor - 1].GetColor();
		else
			areaFloor->color = common::Vector4(1, 1, 1, 1);

		ZeroMemory(&areaFloor->areaGraph, sizeof(areaFloor->areaGraph));

		// Generate AreaFloor Vertex
		if (sVertex *dst = (sVertex*)areaFloor->vtxBuff->Lock(renderer))
		{
			Vector3 *src = &sbuff.m_vertices[0];
			const u_int vertexSize = sbuff.m_vertices.size();
			for (u_int i = 0; i < vertexSize; ++i)
			{
				const Vector3 &vtx = *src++;
				if (vtx.IsEmpty())
					continue;
				if (abs(vtx.x) > 200.f)
					continue;
				if (abs(vtx.z) > 200.f)
					continue;

				const int h = (int)(vtx.y * 10.f);
				if ((h >= 0) && (h < ARRAYSIZE(m_hDistrib)))
				{
					const bool ok = (startIdx <= h) && (endIdx > h);
					if (ok)
					{
						++areaFloor->areaCnt;
						dst->p = vtx + Vector3(0, 0.01f, 0);
						++dst;
					}
				}
			}

			areaFloor->vtxBuff->Unlock(renderer);
		}

		areaFloor->areaGraph.AddValue((float)areaFloor->areaCnt);
	}
	m_areaFloorCnt = floor;
}


// ���̸� ������ �ڽ��� �ܰ����� �ν��ϰ�, �ڽ��� ũ��(����x����)�� ���Ѵ�.
void cMeasure::Measure2DImage()
{
	m_contours.clear();
	m_removeContours.clear();

	cv::Mat &srcImg = g_root.m_measure.m_projMap;
	srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	std::thread th1 = std::thread(ThreadFilterAllFunc, this, true);
	std::thread th2 = std::thread(ThreadFilterAllFunc, this, false);
	th1.join();
	th2.join();

	// Render Candidate Box Rect
	if (g_root.m_isShowBoxCandidate)
	{
		for (u_int i = 0; i < m_contours.size(); ++i)
		{
			auto &info = m_contours[i];
			const Scalar color(255, 255, 255);
			info.contour.Draw(m_dstImg, color);
		}
	}

	m_beforeRemoveContours = m_contours;

	RemoveDuplicateContour(m_contours);

	// Display Detect Box (for debugging)
	if (!m_contours.empty() && !m_binImg.empty())
		cvtColor(m_binImg, m_binImg, cv::COLOR_GRAY2RGB);

	// Render Box Rect
	RenderContourRect(m_dstImg, m_contours);

	m_boxes.clear();
	for (auto &info : m_contours)
	{
		sBoxInfo box = CalcBoxInfo(info);
		m_boxes.push_back(box);
	}
}


// �νĵ� �ڽ��� m_dstImg�� �׸���.
void cMeasure::DrawContourRect()
{
	cv::Mat &srcImg = g_root.m_measure.m_projMap;
	srcImg.convertTo(m_dstImg, CV_8UC1, 255.0f);
	cvtColor(m_dstImg, m_dstImg, cv::COLOR_GRAY2RGB);

	if (g_root.m_isShowBeforeContours)
	{
		RenderContourRect(m_dstImg, m_beforeRemoveContours);
	}
	else
	{
		RenderContourRect(m_dstImg, m_contours);
		RenderContourRect(m_dstImg, m_removeContours, m_contours.size());
	}
}


void cMeasure::RenderContourRect(cv::Mat &dst, const vector<sContourInfo> &contours
	, const int offsetId //=0
)
{
	for (u_int i = 0; i < contours.size(); ++i)
	{
		auto &info = contours[i];
		if (!info.visible)
			continue;

		//const Scalar color(255, 255, 255);
		const Vector3 color2 = info.color.GetColor() * 255;

		if (g_root.m_isShowBox)
		{
			info.contour.Draw(dst, cv::Scalar(color2.x, color2.y, color2.z));

			char boxName[64];
			sprintf(boxName, "BOX%d", i + 1 + offsetId);
			setLabel(dst, boxName, info.contour.m_data, Scalar(1, 1, 1));
		}

		if (g_root.m_isShowBoxVertex)
			info.contour.DrawVertex(dst, 4, cv::Scalar(color2.x, color2.y, color2.z));
	}
}


// ���󿡼� �νĵ� �ȼ� ������ �ڽ� ���� ����� ����Ѵ�.
sBoxInfo cMeasure::CalcBoxInfo(const sContourInfo &info)
{
	//const float scale = 50.f / 73.2f;
	//const float scale = 50.f / 74.5f;
	//const float scale = 50.f / 72.3f;
	const float scale = 50.f / 74.3f;
	//const float scaleH = 0.99f;
	//const float offsetY = ((info.lowerH <= 0) && g_root.m_isPalete) ? -13.f : 3.5f;

	sBoxInfo box;
	if (info.contour.Size() == 4)
	{
		const Vector2 v1((float)info.contour[0].x, (float)info.contour[0].y);
		const Vector2 v2((float)info.contour[1].x, (float)info.contour[1].y);
		const Vector2 v3((float)info.contour[2].x, (float)info.contour[2].y);
		const Vector2 v4((float)info.contour[3].x, (float)info.contour[3].y);

		// maximum value
		const float l1 = std::max((v1 - v2).Length(), (v3 - v4).Length());
		const float l2 = std::max((v2 - v3).Length(), (v4 - v1).Length());
		box.volume.x = std::max(l1, l2); // ū ���� ����
										 //box.volume.y = (info.upperH - info.lowerH) * scaleH + offsetY;
		box.volume.y = (info.upperH - info.lowerH);
		box.volume.z = std::min(l1, l2); // ���� ���� ����

		// average value
		//box.volume.x = (((v1 - v2).Length()) + ((v3 - v4).Length())) * 0.5f;
		//box.volume.y = (info.upperH - info.lowerH) + offsetY;
		//box.volume.z = (((v2 - v3).Length()) + ((v4 - v1).Length())) * 0.5f;

		box.volume.x *= scale;
		box.volume.y *= 1.f;
		box.volume.z *= scale;

		// koreanair cargo code
		//if ((box.volume.y < 40.f) && (box.volume.y > 1.f)) // ���̰� ������ ������ Ŀ���� �����ش�.
		//	box.volume.y -= 1.f;

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

		// koreanair cargo code
		//if ((box.volume.y < 40.f) && (box.volume.y > 1.f)) // ���̰� ������ ������ Ŀ���� �����ش�.
		//	box.volume.y -= 1.f;

		box.minVolume = (float)info.contour.Area() * scale * scale * box.volume.y;
		box.maxVolume = box.minVolume;
		box.loopCnt = info.loop;
	}

	// 3���� �ڽ� ���� ����
	for (u_int i = 0; i < info.contour.Size(); ++i)
	{
		box.box3d[i] = Vector3((info.contour[i].x - 320) / 1.5f, info.upperH, (480 - info.contour[i].y - 240) / 1.5f);
		box.box3d[i + info.contour.Size()] = Vector3((info.contour[i].x - 320) / 1.5f, info.lowerH, (480 - info.contour[i].y - 240) / 1.5f);
	}

	box.color = info.color;
	box.pointCnt = info.contour.Size();
	return box;
}


// ���� �ð����� �ڽ�ũ�� ��հ��� ���Ѵ�.
// ������ ��ġ�� ����� ������, ���� ������ �����Ѵ�.
void cMeasure::CalcBoxVolumeAverage()
{
	const float max_center_gap = 10.f; // 10 cm
	const float max_vertex_gap = 10.f; // 10 cm

	cSensor *sensor1 = NULL;
	for (auto s : g_root.m_baslerCam.m_sensors)
		if (s->IsEnable() && s->m_isShow)
			sensor1 = s;
	if (!sensor1)
		return;

	// ������ ������Ʈ ���� �ʾҰų�, ������ ũ��, ������� �ʴ´�.
	// �ϳ��� �ٸ� ������ �ִٸ�, ����Ѵ�.
	// �ϳ��� ������ ū������ �ִٸ�, �����Ѵ�.
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
		//return; ���� ����Ÿ�� �˻��Ѵ�. ��Ʈ��ũ �����̶�����, ������ �� ���� ���� �ִ�.
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

	// Volume ���� ������Ʈ
	for (auto &contour : m_contours)
	{
		sBoxInfo box = CalcBoxInfo(contour);
		const float vw = box.minVolume / 6000.f;
		const int volIdx = (int)(vw * 100);
		
		if ((volIdx >= 0) && (volIdx < ARRAYSIZE(m_volDistrib)))
			++m_volDistrib[volIdx];
	}

	// ����,���� ���� ������Ʈ
	for (auto &contour : m_contours)
	{
		sBoxInfo box = CalcBoxInfo(contour);
		const float h = std::max(box.volume.x, box.volume.y);
		const float v = std::min(box.volume.x, box.volume.y);
		const int hIdx = (int)(h * 10.f);
		const int vIdx = (int)(v * 10.f);
		if ((hIdx >= 0) && (hIdx < ARRAYSIZE(m_horzDistrib)))
			++m_horzDistrib[hIdx];
		if ((vIdx >= 0) && (vIdx < ARRAYSIZE(m_vertDistrib)))
			++m_vertDistrib[vIdx];
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

			// ������ ������ �ٸ���, �˻� ����
			if (srcBox.contour.Size() != avrBox.box.contour.Size())
				continue;

			cv::Point center1 = avrBox.box.contour.Center();
			Vector3 c1((float)center1.x, avrBox.box.upperH, (float)center1.y);

			// �ڽ� ������ ������, ���� �ڽ��ΰ����� ���
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

			// BoxInfo ���� ������Ʈ
			sContourInfo tempInfo = srcBox;
			for (u_int k = 0; k < srcBox.contour.Size(); ++k)
				tempInfo.contour.m_data[k] = cv::Point((int)box.avrVertices[k].x, (int)box.avrVertices[k].y);
			box.result = CalcBoxInfo(tempInfo);
			m_avrContours.push_back(box);
			continue;
		}

		// box ������ �˻�
		// �� �ڽ��� �������� �����ϴ� avr box�� �������� ã�Ƽ�, ������ ��ġ�� ����Ѵ�.
		sAvrContour &avrBox = m_avrContours[boxIdx];
		++avrBox.count;

		for (u_int k = 0; k < srcBox.contour.Size(); ++k)
		{
			cv::Point vtx0 = srcBox.contour[k];
			Vector2 p0((float)vtx0.x, (float)vtx0.y);

			// ���� ����� ������ ã��
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

			// �����Ǵ� �������� ã������, ��ġ�� ����ؼ� �����Ѵ�. �����ս� ����
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
			//		continue; // ������ ���� �Ÿ��� �ָ�, ���� ������ �˻�

			//	// �����Ǵ� �������� ã������, ��ġ�� ����ؼ� �����Ѵ�. �����ս� ����
			//	avrBox.avrVertices[m].x = (float)CalcAverage(avrBox.count, avrBox.avrVertices[m].x, p0.x);
			//	avrBox.avrVertices[m].y = (float)CalcAverage(avrBox.count, avrBox.avrVertices[m].y, p0.y);

			//	avrBox.vertices3d[m] = Vector3((avrBox.avrVertices[m].x - 320) / 1.5f, avrBox.box.upperH, (480 - avrBox.avrVertices[m].y - 240) / 1.5f);
			//	avrBox.vertices3d[m + avrBox.box.contour.Size()] = Vector3((avrBox.avrVertices[m].x - 320) / 1.5f, avrBox.box.lowerH, (480 - avrBox.avrVertices[m].y - 240) / 1.5f);
			//}
		}

		// BoxInfo ���� ������Ʈ
		sContourInfo tempInfo = avrBox.box;
		for (u_int k = 0; k < avrBox.box.contour.Size(); ++k)
			tempInfo.contour.m_data[k] = cv::Point((int)avrBox.avrVertices[k].x, (int)avrBox.avrVertices[k].y);
		avrBox.result = CalcBoxInfo(tempInfo);
	}

	++m_calcAverageCount;
}


void cMeasure::ClearBoxVolumeAverage()
{
	ZeroMemory(m_volDistrib, sizeof(m_volDistrib));
	ZeroMemory(m_horzDistrib, sizeof(m_horzDistrib));
	ZeroMemory(m_vertDistrib, sizeof(m_vertDistrib));
	m_avrContours.clear();
	m_calcAverageCount = 0;
	g_root.m_dbClient.m_results.clear();
	m_results.clear();
}


// ������������ vtxCnt�� ���� ��, �ڽ��� �����Ѵ�.
bool cMeasure::FindBox(cv::Mat &img
	, const u_int vtxCnt
	, OUT vector<cContour> &out)
{
	vector<vector<cv::Point>> contours;
	findContours(img, contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	//Mat dbgDstImg;
	//cvtColor(img, dbgDstImg, cv::COLOR_GRAY2RGB);

	int m_minArea = 300;
	double m_minCos = (vtxCnt == 4) ? -0.4f : -0.3f; // ���� üũ
	double m_maxCos = (vtxCnt == 4) ? 0.4f : 0.3f;

	vector<cRectContour> rects;
	std::vector<cv::Point> approx;
	for (u_int i = 0; i < contours.size(); i++)
	{
		// Approximate contour with accuracy proportional
		// to the contour perimeter
		const double area = cv::contourArea(contours[i]);

		const double epsilontAlpha = (area < 3000.f) ? 0.03f : 0.015f;
		cv::approxPolyDP(cv::Mat(contours[i]), approx
			, cv::arcLength(cv::Mat(contours[i]), true) * epsilontAlpha, true);

		// Skip small or non-convex objects 
		const bool isConvex = cv::isContourConvex(approx);
		if (std::fabs(cv::contourArea(contours[i])) < m_minArea)
			continue;

		m_minCos = (area < 3000.f) ? -0.4f : -0.33f; // ���� üũ
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

			// for debugging
			//{
			//	cContour contour;
			//	contour.Init(approx);
			//	contour.Draw(dbgDstImg, cv::Scalar(0,0,255));
			//}

			// Use the degrees obtained above and the number of vertices
			// to determine the shape of the contour
			if (
				//(vtc == 4) && 
				(mincos >= m_minCos) && (maxcos <= m_maxCos))
			{
				if (vtc == 4)
				{
					RotatedRect rotateR = minAreaRect(contours[i]);
					cv::Point2f pts[4];
					rotateR.points(pts);

					vector<cv::Point> tmpContour(4);
					for (int k = 0; k < 4; ++k)
						tmpContour[k] = pts[k];

					cContour contour;
					contour.Init(tmpContour);
					contour.m_maxCos = 0;
					contour.m_minCos = 0;
					out.push_back(contour);
				}
				else
				{
					cContour contour;
					contour.Init(approx);
					contour.m_maxCos = maxcos;
					contour.m_minCos = mincos;
					out.push_back(contour);
				}
			}
		}
	}

	return !out.empty();
}


void cMeasure::RemoveDuplicateContour(vector<sContourInfo> &contours)
{
	// Box �ߺ��ν� ����
	if (contours.empty())
		return;

	for (auto &contour : contours)
	{
		contour.used = true;
		contour.visible = true;
	}

	for (u_int i = 0; i < contours.size() - 1; ++i)
	{
		sContourInfo &contour1 = contours[i];
		if (!contour1.used)
			continue;

		for (u_int k = i + 1; k < contours.size(); ++k)
		{
			if (!contour1.used)
				continue;

			sContourInfo &contour2 = contours[k];
			if (!contour2.used)
				continue;

			if (contour1.contour.IsContain(contour2.contour))
			{
				const int a1 = contour1.contour.Area();
				const int a2 = contour2.contour.Area();
				//const bool isDuplciate = abs(((float)a1 / (float)a2) - 1.f) < 0.1f; // ���� ���� ������
				const bool isDuplciate = abs(((float)a1 / (float)a2) - 1.f) < 0.01f; // ���� ���� ������
				contour2.duplicate = isDuplciate;

				contour1.area = a1;
				contour2.area = a2;

				// �ߺ� �νĵ� �ڽ��� ��, �� ���� ������ �ڽ��� �����Ѵ�.
				// ���̰� ���� ���ٸ�, �� ���� loop�� �߰ߵ� �ڽ��� �����, �������� �����Ѵ�.
				if (isDuplciate)
				{
					if (contour1.upperH == contour2.upperH)
					{
						// ���� �簢���� ����� ����� �����, �������� �����Ѵ�.
						//old: �� ���� loop�� �߰ߵ� �ڽ��� �����, �������� �����Ѵ�.
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
						// �� ���� ������ �ڽ��� �����Ѵ�.
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
				else
				{
					// ���̰� ����ϰ�, ���̰� �ٸ� �ڽ��� �������� ��, 
					// ���̰� ���� �ڽ��� �����Ѵ�.
					if (abs(contour1.upperH - contour2.upperH) < 5.f)
					{
						if (a1 > a2)
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
						// ���̰� �ٸ���, ������ ���� ���,
						// ���̰� ���� �ڽ��� �Ʒ��� �ִٸ�, �߸� �νĵ� ���̹Ƿ� �����Ѵ�.
						// ���̰� ���� �ڽ��� ���� �ִٸ�, ����� �νĵ� ���̹Ƿ� �������� �ʴ´�.
						if (a1 > a2)
						{
							if (contour1.upperH > contour2.upperH)
								contour2.used = false;
						}
						else
						{
							if (contour1.upperH < contour2.upperH)
								contour1.used = false;
						}
					}
				}
			}
		}
	}


	// �ڽ� ���� ����
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

				// �ΰ��� �ڽ��� ���Ʒ��� ������ ���� ��,
				// �ڽ��ٴ� ���̸� ����Ѵ�.
				// ������ ū �ڽ��� �Ʒ��� �ִ� ������ �����Ѵ�.
				if (a1 < a2)
				{
					// ������Ʈ�Ǵ� ���̰� �������� ���� ���� ������Ʈ
					if (contour1.lowerH < contour2.upperH)
						contour1.lowerH = contour2.upperH;
				}
				else
				{
					// ������Ʈ�Ǵ� ���̰� �������� ���� ���� ������Ʈ
					if (contour2.lowerH < contour1.upperH)
						contour2.lowerH = contour1.upperH;
				}

				// �ڽ� ���̰� �ʹ� ������ �����Ѵ�.
				if (abs(contour1.upperH - contour1.lowerH) < 5)
					contour1.used = false;
				if (abs(contour2.upperH - contour2.lowerH) < 5)
					contour2.used = false;
			}
		}
	}


	vector<sContourInfo> temp;
	for (auto &contour : contours)
	{
		if (contour.used)
		{
			temp.push_back(contour);
		}
		else
		{
			//contour.color = cColor::WHITE;
			m_removeContours.push_back(contour); // for debugging
		}
	}

	contours.clear();
	contours = temp;
}


void cMeasure::Clear()
{
	for (auto p : m_areaBuff)
	{
		delete p->vtxBuff;
		delete p;
	}
	m_areaBuff.clear();
}
