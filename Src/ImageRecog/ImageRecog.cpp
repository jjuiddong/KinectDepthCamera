// ImageRecog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>


#ifdef _DEBUG
	#pragma comment(lib, "opencv_core340d.lib")
	#pragma comment(lib, "opencv_imgcodecs340d.lib")
	//#pragma comment(lib, "opencv_features2d310d.lib")
	//#pragma comment(lib, "opencv_videoio310d.lib")
	//#pragma comment(lib, "opencv_highgui310d.lib")
	#pragma comment(lib, "opencv_imgproc340d.lib")
	//#pragma comment(lib, "opencv_flann310d.lib")
	//#pragma comment(lib, "opencv_xfeatures2d310d.lib")
	//#pragma comment(lib, "opencv_calib3d310d.lib")
#else
	#pragma comment(lib, "opencv_core340.lib")
	#pragma comment(lib, "opencv_imgcodecs340.lib")
	//#pragma comment(lib, "opencv_features2d310.lib")
	//#pragma comment(lib, "opencv_videoio310.lib")
	//#pragma comment(lib, "opencv_highgui310.lib")
	#pragma comment(lib, "opencv_imgproc340.lib")
	//#pragma comment(lib, "opencv_flann310.lib")
	//#pragma comment(lib, "opencv_xfeatures2d310.lib")
	//#pragma comment(lib, "opencv_calib3d310.lib")
#endif

using namespace cv;
using std::vector;
typedef unsigned int u_int;

double angle(cv::Point pt1, const cv::Point &pt2, const cv::Point &pt0);
void DrawLines(Mat &dst, const vector<cv::Point> &lines, const cv::Scalar &color, const int thickness,
	const bool isLoop);


int main()
{
	const int vtxCnt = 8;

	Mat img;
	img = imread("depthframe_mat_1_0_8_6.jpg");
	Mat dbgDstImg = img;
	cvtColor(img, img, cv::COLOR_RGB2GRAY);
	threshold(img, img, 200, 255, cv::THRESH_BINARY);

	vector<vector<cv::Point>> contours;
	findContours(img, contours, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	int m_minArea = 300;
	double m_minCos = (vtxCnt == 4) ? -0.4f : -0.3f; // 직각 체크
	double m_maxCos = (vtxCnt == 4) ? 0.4f : 0.3f;

	//vector<cRectContour> rects;
	std::vector<cv::Point> approx;
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		// Approximate contour with accuracy proportional
		// to the contour perimeter
		const double area = cv::contourArea(contours[i]);
		const double arcLen = cv::arcLength(cv::Mat(contours[i]), true);
		const double epsilontAlpha = (area < 3000.f) ? 0.03f : 0.015f;
		const double epsilon = arcLen * epsilontAlpha;
		cv::approxPolyDP(cv::Mat(contours[i]), approx, epsilon, true);

		// Skip small or non-convex objects 
		const bool isConvex = cv::isContourConvex(approx);
		if (std::fabs(cv::contourArea(contours[i])) < m_minArea)
			continue;

		DrawLines(dbgDstImg, contours[i], Scalar(0, 255, 255), 1, true);

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

			DrawLines(dbgDstImg, approx, Scalar(0, 0, 255), 1, true);

			// Use the degrees obtained above and the number of vertices
			// to determine the shape of the contour
			if (
				//(vtc == 4) && 
				(mincos >= m_minCos) && (maxcos <= m_maxCos))
			{
				//cContour contour;
				//contour.Init(approx);
				//contour.m_maxCos = maxcos;
				//contour.m_minCos = mincos;
				//out.push_back(contour);
			}
		}
	}

    return 0;
}



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


void DrawLines(Mat &dst, const vector<cv::Point> &lines, const cv::Scalar &color, const int thickness,
	const bool isLoop)
{
	if (lines.size() < 2)
		return;

	for (u_int i = 0; i < lines.size() - 1; ++i)
		line(dst, lines[i], lines[i + 1], color, thickness);

	if (isLoop)
		line(dst, lines[lines.size() - 1], lines[0], color, thickness);
}
