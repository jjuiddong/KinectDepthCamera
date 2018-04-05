
#include "stdafx.h"
#include "rectcontour.h"
#include "cvutil.h"


using namespace common;
using namespace cv;



// hhttp://www.gisdeveloper.co.kr/15
// AP1 - AP2 �� ������ ����
// AP3 - AP4 �� ������ ����
// �� ������ ������ ã�� IP�� �����Ѵ�.
// ������ ���ٸ� false�� �����Ѵ�.
bool GetIntersectPoint(const Point2f& AP1, const Point2f& AP2,
	const Point2f& BP1, const Point2f& BP2, Point2f* IP)
{
	float t;
	float s;
	float under = (BP2.y - BP1.y)*(AP2.x - AP1.x) - (BP2.x - BP1.x)*(AP2.y - AP1.y);
	if (under == 0) return false;

	float _t = (BP2.x - BP1.x)*(AP1.y - BP1.y) - (BP2.y - BP1.y)*(AP1.x - BP1.x);
	float _s = (AP2.x - AP1.x)*(AP1.y - BP1.y) - (AP2.y - AP1.y)*(AP1.x - BP1.x);

	t = _t / under;
	s = _s / under;

	if (t<0.0 || t>1.0 || s<0.0 || s>1.0) return false;
	if (_t == 0 && _s == 0) return false;

	IP->x = AP1.x + t * (float)(AP2.x - AP1.x);
	IP->y = AP1.y + t * (float)(AP2.y - AP1.y);

	return true;
}


bool GetIntersectPoint(const cv::Point& AP1, const cv::Point& AP2,
	const cv::Point& BP1, const cv::Point& BP2, cv::Point2f* IP)
{
	return GetIntersectPoint(cv::Point2f((float)AP1.x, (float)AP1.y),
		cv::Point2f((float)AP2.x, (float)AP2.y),
		cv::Point2f((float)BP1.x, (float)BP1.y),
		cv::Point2f((float)BP2.x, (float)BP2.y),
		IP);
}


// contours �� ������� �����Ѵ�.
// 0 ---------- 1
// |				    |
// |					|
// |					|
// 3 ---------- 2
//
// chIndices �� NULL�� �ƴϸ�, src �ε����� �ٲ� ������ �����Ѵ�.
bool OrderedContours(const cv::Point src[4], OUT cv::Point dst[4], OUT int *chIndices=NULL)
{
	//--------------------------------------------------------------------
	// 4 point cross check
	int crossIndices[3][4] =
	{
		{ 0, 1, 2, 3 },
	{ 0, 2, 1, 3 },
	{ 0, 3, 1, 2 },
	};

	int crossIdx = -1;
	for (int i = 0; i < 3; ++i)
	{
		// line1 = p1-p2 
		// line2 = p3-p4
		const cv::Point p1 = src[crossIndices[i][0]];
		const cv::Point p2 = src[crossIndices[i][1]];
		const cv::Point p3 = src[crossIndices[i][2]];
		const cv::Point p4 = src[crossIndices[i][3]];

		cv::Point2f tmp;
		if (GetIntersectPoint(p1, p2, p3, p4, &tmp))
		{
			crossIdx = i;
			break;
		}
	}

	if (crossIdx < 0)
		return false;

	// p1 ------ p2
	// |              |
	// |              |
	// |              |
	// p4 ------ p3
	const cv::Point p1 = src[crossIndices[crossIdx][0]];
	const cv::Point p2 = src[crossIndices[crossIdx][2]];
	const cv::Point p3 = src[crossIndices[crossIdx][1]];
	const cv::Point p4 = src[crossIndices[crossIdx][3]];


	//--------------------------------------------------------------------
	// ���� ���
	// ���� ū �ڽ��� ã�´�.
	Point center = p1;
	center += p2;
	center += p3;
	center += p4;

	// �߽��� ���
	center.x /= 4;
	center.y /= 4;

	Vector3 v1 = Vector3((float)p1.x, (float)p1.y, 0) - Vector3((float)center.x, (float)center.y, 0);
	v1.Normalize();
	Vector3 v2 = Vector3((float)p2.x, (float)p2.y, 0) - Vector3((float)center.x, (float)center.y, 0);
	v2.Normalize();

	// 0 ---------- 1
	// |				    |
	// |					|
	// |					|
	// 3 ---------- 2
	const Vector3 crossV = v1.CrossProduct(v2);
	if (crossV.z > 0)
	{
		dst[0] = p1;
		dst[1] = p2;
		dst[2] = p3;
		dst[3] = p4;

		if (chIndices)
		{
			chIndices[0] = crossIndices[crossIdx][0];
			chIndices[1] = crossIndices[crossIdx][2];
			chIndices[2] = crossIndices[crossIdx][1];
			chIndices[3] = crossIndices[crossIdx][3];
		}
	}
	else
	{
		dst[0] = p2;
		dst[1] = p1;
		dst[2] = p4;
		dst[3] = p3;

		if (chIndices)
		{
			chIndices[0] = crossIndices[crossIdx][2];
			chIndices[1] = crossIndices[crossIdx][0];
			chIndices[2] = crossIndices[crossIdx][3];
			chIndices[3] = crossIndices[crossIdx][1];
		}
	}

	return true;
}


// contours �� ������� �����Ѵ�.
// 0 ---------- 1
// |				    |
// |					|
// |					|
// 2 ---------- 3
bool OrderedContours(const vector<cv::Point> &src, OUT vector<cv::Point> &dst)
{
	Point arr[4];
	Point out[4];

	arr[0] = src[0];
	arr[1] = src[1];
	arr[2] = src[2];
	arr[3] = src[3];

	const bool reval = OrderedContours(arr, out);
	if (reval)
	{
		dst[0] = out[0];
		dst[1] = out[1];
		dst[2] = out[2];
		dst[3] = out[3];
	}
	else
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
	}

	return reval;
}






cRectContour::cRectContour() :
	m_contours(4, cv::Point(0,0))
	, m_xIdx(0)
{
}

cRectContour::cRectContour(const vector<cv::Point> &contours) :
	m_contours(4)
	, m_xIdx(0)
{
	Init(contours);
}

cRectContour::cRectContour(const vector<cv::KeyPoint> &contours) :
	m_contours(4)
	, m_xIdx(0)
{
	Init(contours);
}


cRectContour::cRectContour(const cv::Point &center, const float size) :
	m_contours(4)
	, m_xIdx(0)
{
	Init(center, size);
}

cRectContour::cRectContour(const cv::Rect &rect) :
	m_contours(4)
	, m_xIdx(0)
{
	Init(rect);
}

cRectContour::~cRectContour()
{
}


// �ʱ�ȭ.
bool cRectContour::Init(const cv::Point contours[4])
{
	Point dst[4];
	//OrderedContours(contours, dst);
	m_contours[0] = dst[0];
	m_contours[1] = dst[1];
	m_contours[2] = dst[2];
	m_contours[3] = dst[3];
	//CalcWidthHeightVector();
	return true;
}

bool cRectContour::Init(const vector<cv::Point> &contours)
{
	OrderedContours(contours, m_contours);
	//CalcWidthHeightVector();
	return true;
}
bool cRectContour::Init(const vector<cv::Point2f> &contours)
{
	vector<cv::Point> ct(4);
	ct[0] = contours[0];
	ct[1] = contours[1];
	ct[2] = contours[2];
	ct[3] = contours[3];
	return Init(ct);
}


// �ʱ�ȭ.
bool cRectContour::Init(const vector<cv::KeyPoint> &keypoints)
{
	if (keypoints.size() < 4)
		return false;

	vector<Point> keys(4);
	for (u_int i = 0; i < keypoints.size(); ++i)
		keys[i] = Point(keypoints[i].pt);

	//OrderedContours(keys, m_contours);
	//CalcWidthHeightVector();

	return true;
}


// ���� center�� �߽����� size��ŭ Ű���� �簢���� �����.
//
// 0 -------- 1
// |          |
// |    +     |
// |          |
// 3 -------- 2
//
bool cRectContour::Init(const cv::Point &center, const float size)
{
	m_contours[0] = center + Point(-(int)(size / 2.f), -(int)(size / 2.f));
	m_contours[1] = center + Point((int)(size / 2.f), -(int)(size / 2.f));
	m_contours[2] = center + Point((int)(size / 2.f), (int)(size / 2.f));
	m_contours[3] = center + Point(-(int)(size / 2.f), (int)(size / 2.f));
	//CalcWidthHeightVector();
	return true;
}


bool cRectContour::Init(const cv::Rect &rect)
{
	vector<cv::Point> ct(4);
	ct[0] = rect.tl();
	ct[1] = Point(rect.x + rect.width, rect.y);
	ct[2] = Point(rect.x + rect.width, rect.y+rect.height);
	ct[3] = Point(rect.x, rect.y+rect.height);
	return Init(ct);
}


void cRectContour::Normalize()
{
	vector<Point> keys(4);
	for (u_int i = 0; i < m_contours.size(); ++i)
		keys[i] = m_contours[i];
	//OrderedContours(keys, m_contours);
	//CalcWidthHeightVector();
}


// �ڽ� ���.
void cRectContour::Draw(cv::Mat &dst, const cv::Scalar &color, const int thickness) const
// color = cv::Scalar(0, 0, 0), thickness = 1
{
	DrawLines(dst, m_contours, color, thickness);
}


// �簢���� ������ �����Ѵ�.
Point cRectContour::Center() const
{
	Point center;
	for (auto &pt : m_contours)
		center += pt;
	
	center = Point(center.x / m_contours.size(), center.y / m_contours.size());
	return center;
}


// �簢���� ������ �߽����� �������Ѵ�.
void cRectContour::ScaleCenter(const float scale)
{
	const Point center = Center();

	for (u_int i=0; i < m_contours.size(); ++i)
	{
		m_contours[i] = center + ((m_contours[i] - center) * scale);
	}
}


// ���� ���� ���� �����ϸ� �Ѵ�.
// 0 -------- 1
// |          |
// |    +     |
// |          |
// 3 -------- 2
void cRectContour::Scale(const float vscale, const float hscale)
{

	// ���� �����ϸ�
	vector<cv::Point> tmp1(4);
	tmp1[0] = m_contours[1] + (m_contours[0] - m_contours[1]) * hscale;
	tmp1[1] = m_contours[0] + (m_contours[1] - m_contours[0]) * hscale;
	tmp1[2] = m_contours[3] + (m_contours[2] - m_contours[3]) * hscale;
	tmp1[3] = m_contours[2] + (m_contours[3] - m_contours[2]) * hscale;

	// ���� �����ϸ�
	vector<cv::Point> tmp2(4);
	tmp2[0] = m_contours[3] + (m_contours[0] - m_contours[3]) * vscale;
	tmp2[3] = m_contours[0] + (m_contours[3] - m_contours[0]) * vscale;
	tmp2[1] = m_contours[2] + (m_contours[1] - m_contours[2]) * vscale;
	tmp2[2] = m_contours[1] + (m_contours[2] - m_contours[1]) * vscale;

	m_contours[0] = (tmp1[0] + tmp2[0]) * 0.5f;
	m_contours[1] = (tmp1[1] + tmp2[1]) * 0.5f;
	m_contours[2] = (tmp1[2] + tmp2[2]) * 0.5f;
	m_contours[3] = (tmp1[3] + tmp2[3]) * 0.5f;
}


bool cRectContour::IsContain(const cRectContour &rect)
{
	const Point center1 = Center();
	const int w1 = Width();
	const int h1 = Height();
	const Point center2 = rect.Center();
	const int w2 = rect.Width();
	const int h2 = rect.Height();

	const Point c(abs(center1.x - center2.x), abs(center1.y - center2.y));
	if ((c.x < (w1 / 2 + w2 / 2)) && (c.y < (h1 / 2 + h2 / 2)))
		return true;

	return false;
}


// index ��° �����͸� �����Ѵ�.
Point cRectContour::At(const int index) const
{
	return m_contours[index];
}


bool cRectContour::IsEmpty()
{
	if ((m_contours[0] == cv::Point(0, 0))
		&& (m_contours[1] == cv::Point(0, 0))
		&& (m_contours[2] == cv::Point(0, 0))
		&& (m_contours[3] == cv::Point(0, 0)))
		return true;

	return false;
}


// AABB 
int cRectContour::Width() const
{
	//const int idx1 = m_xIdx;
	//const int idx2 = m_xIdx + 1;
	//const int idx3 = m_xIdx + 2;
	//const int idx4 = (m_xIdx + 3) % 4;

	//return (int)abs(((m_contours[idx2].x - m_contours[idx1].x) + 
	//	(m_contours[idx3].x - m_contours[idx4].x)) * 0.5f);

	int minX = INT_MAX;
	int maxX = 0;
	for (u_int i = 0; i < m_contours.size(); ++i)
	{
		if (m_contours[i].x < minX)
			minX = m_contours[i].x;
		if (m_contours[i].x > maxX)
			maxX = m_contours[i].x;
	}

	return abs(maxX - minX);
}


// AABB
int cRectContour::Height() const
{
	//const int idx1 = m_xIdx + 1;
	//const int idx2 = m_xIdx + 2;
	//const int idx3 = (m_xIdx + 3) % 4;
	//const int idx4 = m_xIdx;

	//return (int)abs(((m_contours[idx2].y - m_contours[idx1].y) + 
	//	(m_contours[idx3].y - m_contours[idx4].y)) * 0.5f);

	int minY = INT_MAX;
	int maxY = 0;
	for (u_int i = 0; i < m_contours.size(); ++i)
	{
		if (m_contours[i].y < minY)
			minY = m_contours[i].y;
		if (m_contours[i].x > maxY)
			maxY = m_contours[i].y;
	}

	return abs(maxY - minY);
}


cRectContour& cRectContour::operator= (const cRectContour &rhs)
{
	if (this != &rhs)
	{
		m_contours = rhs.m_contours;
		m_xIdx = rhs.m_xIdx;
	}

	return *this;
}


// Normalize�� ���¿��� �� �Լ��� ȣ���ؾ� �Ѵ�.
void cRectContour::CalcWidthHeightVector()
{
	Vector3 p1((float)m_contours[0].x, (float)m_contours[0].y, 0);
	Vector3 p2((float)m_contours[1].x, (float)m_contours[1].y, 0);
	Vector3 v = p2 - p1;
	v.Normalize();
	Vector3 xAxis(1, 0, 0);
	if (abs(v.DotProduct(xAxis)) > 0.8f)
	{
		m_xIdx = 0;
	}
	else
	{
		m_xIdx = 1;
	}
}
