
#include "stdafx.h"
#include "contour.h"
#include "cvutil.h"
#include "clipper.hpp"

using namespace ClipperLib;
using namespace cv;


cContour::cContour()
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


bool cContour::IsContain(const cContour &contour)
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


u_int cContour::Area()
{
	return (u_int)cv::contourArea(m_data);
}
