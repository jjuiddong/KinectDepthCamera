
#include "stdafx.h"
#include "cvutil.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


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
void setLabel(cv::Mat& im, const std::string &label, const std::vector<cv::Point>& contour
	, const cv::Scalar &color //= cv::Scalar(0, 0, 0)
)
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


void setLabel(cv::Mat& im, const std::string &label, const cv::Point& pos
	, const cv::Scalar &color //= cv::Scalar(0, 0, 0)
)
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

