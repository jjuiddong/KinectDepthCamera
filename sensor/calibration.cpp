
#include "stdafx.h"
#include "calibration.h"

using namespace common;
using namespace graphic;


cCalibration::cCalibration()
{
	srand(timeGetTime());
}

cCalibration::~cCalibration()
{
	Clear();
}


// 바닥 평면 컬리브레이션
// 영역이 정해진 후, 
//        top
//  * ----- * ----- * 
//  |   0   |   1   | right
//  |       |       |
//  * ----- * ----- *
//  |   3   |   2   |
//  |       |       |
//  * ----- * ----- *
//        bottom
//
//   /\ Z axis
//    |
//    |
//    |
//   -------------> X Axis
//
// 4등분 한 후, 랜덤으로 설정된 3 영역에서, 랜덤포인트를 찍는다.
// 3포인트로 평면을 생성하고, 그 평면에서 영역의 높이 편차를 구한 후
// 가장 작게되는 편차가 될 때까지 반복한다.
//
cCalibration::sResult cCalibration::CalibrationBasePlane(const common::Vector3 &center0
	, const common::Vector2 minMax, const cSensor *sensor)
{
	// 위치값을 기본 좌표계로 복원한다.
	const Vector3 center = center0;
	const sRectf range = sRectf(center.x - minMax.x, center.z + minMax.y,
		center.x + minMax.x, center.z - minMax.y);

	//float offset = 5.f; // 5cm
	float offset = 2.f; // 2cm
	const float LIMIT_Y = 5.f;
	sRectf rects[4] = {
		sRectf(center.x - minMax.x, center.z + minMax.y
		, center.x, center.z)
		, sRectf(center.x, center.z + minMax.y
			, center.x + minMax.x, center.z)
		, sRectf(center.x, center.z
			, center.x + minMax.x, center.z - minMax.y)
		, sRectf(center.x - minMax.x, center.z
			, center.x, center.z - minMax.y)
	};

	// 오프셋 적용
	for (int i = 0; i < 4; ++i)
	{
		rects[i].left += offset;
		rects[i].top -= offset;
		rects[i].right -= offset;
		rects[i].bottom += offset;
	}

	vector<Vector3> pts[4];
	for (int i = 0; i < 4; ++i)
		pts[i].reserve(1024 * 5);

	// 4분할 영역에 속하는 포인트 클라우드를 분리한다.
	for (auto &vtx : sensor->m_buffer.m_vertices)
	{
		if (vtx.IsEmpty())
			continue;

		for (int i = 0; i < 4; ++i)
		{
			if ((rects[i].left < vtx.x)
				&& (rects[i].right > vtx.x)
				&& (rects[i].top > vtx.z)
				&& (rects[i].bottom < vtx.z))
			{
				if (abs(center.y - vtx.y) < LIMIT_Y)
					pts[i].push_back(vtx);
				break;
			}
		}
	}

	const double curSD = CalcHeightStandardDeviation(sensor->m_buffer.m_vertices, center, range, Matrix44::Identity);
	double minSD = FLT_MAX;
	Plane minPlane;
	int cnt = 0;
	while (cnt++ < MAX_GROUNDPLANE_CALIBRATION)
	{
		// 4분할된 영역을 랜덤하게 3 영역을 선택한다.
		int rectIdAr[] = { 0,1,2,3 };
		int ids[3];
		for (int i = 0; i < 3; ++i)
		{
			const int k = rand() % (4 - i);
			ids[i] = rectIdAr[k];
			rectIdAr[k] = rectIdAr[3 - i];
		}

		// 선택한 영역에서 랜덤하게 포인트를 선택한다.
		const Vector3 p1 = pts[ids[0]][common::randint(0, pts[ids[0]].size() - 1)];
		const Vector3 p2 = pts[ids[1]][common::randint(0, pts[ids[1]].size() - 1)];
		const Vector3 p3 = pts[ids[2]][common::randint(0, pts[ids[2]].size() - 1)];

		// 선택한 포인트로 평면을 만든다.
		Plane plane(p1, p2, p3);
		if (plane.N.y < 0)
			plane = Plane(p1, p3, p2);

		Matrix44 tm;
		if (!plane.N.IsEmpty())
		{
			Quaternion q;
			q.SetRotationArc(plane.N, Vector3(0, 1, 0));
			tm *= q.GetMatrix();
		}

		// 편차를 구하고, 가장 낮은 편차가 될 때까지 반복한다.
		const double sd = CalcHeightStandardDeviation(sensor->m_buffer.m_vertices, center, range, tm);
		if (minSD > sd)
		{
			minSD = sd;
			minPlane = plane;
		}
	}

	sResult result;
	result.curSD = curSD;
	result.minSD = minSD;
	result.plane = minPlane;

	m_result = result;
	m_planes.push_back(minPlane);

	// 평균값 계산
	Vector3 n;
	float d = 0;
	for (auto &plane : m_planes)
	{
		n += plane.N;
		d += plane.D;
	}
	n.Normalize();
	d /= (float)m_planes.size();
	m_avrPlane = Plane(n, d);

	return result;
}


// script format
//
// range info
// center x y z
// minmax x z
// filename1
// filename2
// filename3
// .,..
//
// range info
// center x y z
// minmax x z
// filename1
// filename2
// filename3
// .,..
bool cCalibration::CalibrationBasePlane(const char *scriptFileName, OUT sResult &out)
{
	using namespace std;
	ifstream ifs(scriptFileName);
	if (!ifs.is_open())
		return false;

	vector<sRange> ranges;

	int state = 0;
	Str256 line;
	char temp[64];
	sRange *range = NULL;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		if (line.empty())
			continue;

		switch (state)
		{
		case 0:
			if (line == "range info")
			{
				state = 1;
				ranges.push_back({});
				range = &ranges.back();
			}
			break;

		case 1: // parse...sensor index
		{
			if (!range)
				break;

			stringstream ss(line.c_str());
			ss >> temp >> range->sensorId;
			state = 2;
		}
		break;

		case 2: // parse...center x y z
		{
			if (!range)
				break;

			stringstream ss(line.c_str());
			ss >> temp >> range->center.x >> range->center.y >> range->center.z;
			state = 3;
		}
		break;

		case 3: // parse...minmax x z
		{			
			if (!range)
				break;

			stringstream ss(line.c_str());
			ss >> temp >> range->minMax.x >> range->minMax.y;
			state = 4;
		}
		break;

		case 4: // parse...filename
		{
			if (!range)
				break;

			if (line == "range info")
			{
				state = 1;
				ranges.push_back({});
				range = &ranges.back();
			}
			else
			{
				range->files.push_back(line.c_str());
			}
		}
		break;
		}
	}

	return CalibrationBasePlane(ranges, out);
}


// 여려개의 영역을 이용해 컬리브레이션 한다.
// ranges 는 최소 3개 이상이어야 한다.
bool cCalibration::CalibrationBasePlane(const vector<sRange> &ranges, OUT sResult &out)
{
	if (ranges.size() < 3)
		return false;

	vector<vector<Vector3>> *range_files = new vector<vector<Vector3>>[ranges.size()];

	// 각 영역의 Vertex 정보를 읽는다.
	// 지정된 영역만 저장하며, 이후에 이 정보만 계산한다.
	const float offset = 5.f;
	for (u_int i = 0; i < ranges.size(); ++i)
	{
		const sRange &range = ranges[i];

		if (g_root.m_baslerCam.m_sensors.size() <= (u_int)range.sensorId)
		{
			delete[] range_files;
			return false; // error occur
		}

		cSensor *sensor = g_root.m_baslerCam.m_sensors[range.sensorId];

		// plane calculation
		Transform tfm;
		tfm.scale = Vector3(1, 1, 1)*0.1f;
		Matrix44 tm = tfm.GetMatrix();
		if (!g_root.m_groundPlane.N.IsEmpty())
		{
			Quaternion q;
			q.SetRotationArc(g_root.m_groundPlane.N, Vector3(0, 1, 0));
			tm *= q.GetMatrix();

			Vector3 center = g_root.m_volumeCenter * q.GetMatrix();
			center.y = 0;

			Matrix44 T;
			T.SetPosition(Vector3(-center.x, g_root.m_groundPlane.D, -center.z));

			tm *= T;
		}

		tm *= sensor->m_buffer.m_offset.GetMatrix();

		const sRectf range_rect(range.center.x - range.minMax.x, range.center.z + range.minMax.y
			, range.center.x + range.minMax.x, range.center.z - range.minMax.y);

		for (auto &filename : range.files)
		{
			cDatReader reader;
			if (reader.Read(filename.c_str()))
			{
				range_files[i].push_back({});
				range_files[i].back().reserve(1024 * 5);

				for (auto &vtx : reader.m_vertices)
				{
					if (isnan(vtx.x))
						continue;

					const Vector3 pos = vtx * tm;

					if ((range_rect.left < pos.x)
						&& (range_rect.right > pos.x)
						&& (range_rect.top > pos.z)
						&& (range_rect.bottom < pos.z))
					{
						if (abs(range.center.y - pos.y) < offset)
							range_files[i].back().push_back(pos);
					}
				}
			}
			else
			{
				dbg::Logp("Error CalibrationBasePlane() Read file Error %s\n", filename.c_str());
			}
		}
	}

	int cnt = 0;
	double curSD = 0;
	double minSD = FLT_MAX;
	Plane minPlane;
	bool checkSD = true; // 현재 편차를 구하기 위한 플래그
	int *rectIdAr = new int[ranges.size()];

	// 파일이 없는 영역이 있으면, 함수를 종료한다.
	for (u_int i = 0; i < ranges.size(); ++i)
		for (auto &files : range_files[i])
			if (files.empty())
				goto error;
	
	while (cnt++ < MAX_GROUNDPLANE_CALIBRATION)
	{
		// 각 영역을 랜덤하게 3 영역을 선택한다.
		for (u_int i = 0; i < ranges.size(); ++i)
			rectIdAr[i] = i;

		int ids[3];
		for (int i = 0; i < 3; ++i)
		{
			const int k = rand() % (ranges.size() - i);
			ids[i] = rectIdAr[k];
			rectIdAr[k] = rectIdAr[ranges.size() - 1 - i];
		}

		// 각영역의 파일을 랜덤하게 선택한다.
		const int fileIdx1 = common::randint(0, range_files[ids[0]].size() - 1);
		const int fileIdx2 = common::randint(0, range_files[ids[1]].size() - 1);
		const int fileIdx3 = common::randint(0, range_files[ids[2]].size() - 1);

		const vector<Vector3> &vertices1 = range_files[ids[0]][fileIdx1];
		const vector<Vector3> &vertices2 = range_files[ids[1]][fileIdx2];
		const vector<Vector3> &vertices3 = range_files[ids[2]][fileIdx3];

		// 선택한 영역에서 랜덤하게 포인트를 선택한다.
		const Vector3 p1 = vertices1[common::randint(0, vertices1.size() - 1)];
		const Vector3 p2 = vertices2[common::randint(0, vertices2.size() - 1)];
		const Vector3 p3 = vertices3[common::randint(0, vertices3.size() - 1)];

		// 선택한 포인트로 평면을 만든다.
		Plane plane(p1, p2, p3);
		if (plane.N.y < 0)
			plane = Plane(p1, p3, p2);

		Matrix44 newTm;
		if (!plane.N.IsEmpty())
		{
			Quaternion q;
			q.SetRotationArc(plane.N, Vector3(0, 1, 0));
			newTm *= q.GetMatrix();
		}

		// 편차를 구하고, 가장 낮은 편차가 될 때까지 반복한다.
		double avrSD = 0;
		const vector<Vector3> *vtxAr[3] = { &vertices1, &vertices2, &vertices3 };
		for (int i = 0; i < 3; ++i)
		{
			const double sd = CalcHeightStandardDeviation(*vtxAr[i], newTm);
			
			//const sRectf range_rect(ranges[ids[i]].center.x - ranges[ids[i]].minMax.x
			//	, ranges[ids[i]].center.z + ranges[ids[i]].minMax.y
			//	, ranges[ids[i]].center.x + ranges[ids[i]].minMax.x
			//	, ranges[ids[i]].center.z - ranges[ids[i]].minMax.y);
			//const double sd = CalcHeightStandardDeviation(*vtxAr[i], ranges[ids[i]].center, range_rect, newTm);
			avrSD += sd;
		}
		avrSD /= 3.f;

		if (minSD > avrSD)
		{
			minSD = avrSD;
			minPlane = plane;
		}

		if (checkSD)
		{
			checkSD = false;
			const double sd1 = CalcHeightStandardDeviation(*vtxAr[0], Matrix44::Identity);
			const double sd2 = CalcHeightStandardDeviation(*vtxAr[1], Matrix44::Identity);
			const double sd3 = CalcHeightStandardDeviation(*vtxAr[2], Matrix44::Identity);
			curSD = (sd1 + sd2 + sd3) / 3.f;
		}
	}

	out.curSD = curSD;
	out.minSD = minSD;
	out.plane = minPlane;

	delete[] range_files;
	delete[] rectIdAr;
	return true;

error:
	delete[] range_files;
	delete[] rectIdAr;
	return false;
}


// rect영역의 높이 표준편차를 구한다.
// rect : x-z plane rect
//		  left-right, x axis
//		  top-bottom, z axis
// center : 기준 높이 y
// tm : transform matrix
//
double cCalibration::CalcHeightStandardDeviation(const vector<Vector3> &vertices
	, const Vector3 &center, const sRectf &rect, const Matrix44 &tm)
{
	// 높이 평균 구하기
	int k = 0;
	double avr = 0;
	for (auto &vtx : vertices)
	{
		if (vtx.IsEmpty())
			continue;

		if ((rect.left < vtx.x)
			&& (rect.right > vtx.x)
			&& (rect.top > vtx.z)
			&& (rect.bottom < vtx.z))
		{
			if (abs(vtx.y - center.y) > 10) // 10 cm, too much over
				continue;

			const Vector3 p = vtx * tm;
			avr = CalcAverage(++k, avr, p.y);
		}
	}

	// 표준 편차 구하기
	k = 0;
	double sd = 0;
	for (auto &vtx : vertices)
	{
		if (vtx.IsEmpty())
			continue;

		if ((rect.left < vtx.x)
			&& (rect.right > vtx.x)
			&& (rect.top > vtx.z)
			&& (rect.bottom < vtx.z))
		{
			if (abs(vtx.y - center.y) > 10) // 10 cm, too much over
				continue;

			const Vector3 p = vtx * tm;
			sd = CalcAverage(++k, sd, (p.y - avr) * (p.y - avr));
		}
	}
	sd = sqrt(sd);

	return sd;
}


// vertices 높이 평균, 편차를 구한다. 
// 편차를 리턴한다.
double cCalibration::CalcHeightStandardDeviation(const vector<Vector3> &vertices, const common::Matrix44 &tm)
{
	// 높이 평균 구하기
	int k = 0;
	double avr = 0;
	for (auto &vtx : vertices)
	{
		const Vector3 p = vtx * tm;
		avr = CalcAverage(++k, avr, p.y);
	}

	// 표준 편차 구하기
	k = 0;
	double sd = 0;
	for (auto &vtx : vertices)
	{
		const Vector3 p = vtx * tm;
		sd = CalcAverage(++k, sd, (p.y - avr) * (p.y - avr));
	}
	sd = sqrt(sd);

	return sd;
}


void cCalibration::Clear()
{
	m_planes.clear();
}
