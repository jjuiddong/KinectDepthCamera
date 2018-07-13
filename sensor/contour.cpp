
#include "stdafx.h"
#include "contour.h"
#include "cvutil.h"
#include "clipper.hpp"

using namespace ClipperLib;
using namespace cv;


cContour::cContour()
	: m_minCos(0)
	, m_maxCos(0)

{
}

cContour::~cContour()
{
}


bool cContour::Init(const vector<cv::Point> &contour)
{
	m_data = contour;
	return true;
}


void cContour::Draw(cv::Mat &dst
	, const cv::Scalar &color //= cv::Scalar(0, 0, 0)
	, const int thickness //= 1
) const
{
	DrawLines(dst, m_data, color, thickness);
}


void cContour::DrawVertex(cv::Mat &dst
	, const int radius // = 5
	, const cv::Scalar &color //= cv::Scalar(0, 0, 0)
	, const int thickness //= 1
) const
{
	for (u_int i = 0; i < m_data.size(); ++i)
		circle(dst, m_data[i], radius, color, thickness);
}


bool cContour::IsContain(const cContour &contour) const
{
	Paths p1(1), p2(1), solution;

	for (auto &pos : m_data)
		p1[0] << IntPoint(pos.x, pos.y);

	for (auto &pos : contour.m_data)
		p2[0] << IntPoint(pos.x, pos.y);

	Clipper c;
	c.AddPaths(p1, ptSubject, true);
	c.AddPaths(p2, ptClip, true);
	c.Execute(ctIntersection, solution, pftNonZero, pftNonZero);
	
	return !solution.empty();
}


u_int cContour::Area() const
{
	return (u_int)abs(cv::contourArea(m_data));
}


cv::Point cContour::Center() const
{
	if (m_data.empty())
		return cv::Point(0, 0);

	Point center;
	for (auto &pt : m_data)
		center += pt;

	center = Point(center.x / m_data.size(), center.y / m_data.size());
	return center;
}


bool cContour::operator==(const cContour &rhs) const
{
	if (m_data.size() != rhs.m_data.size())
		return false;

	for (u_int i = 0; i < m_data.size(); ++i)
		if (m_data[i] != rhs.m_data[i])
			return false;

	return true;
}