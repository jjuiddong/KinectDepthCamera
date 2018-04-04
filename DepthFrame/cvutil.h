//
// 2018-04-04, jjuiddong
// opencv utility
//
#pragma once

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


double angle(cv::Point pt1, const cv::Point &pt2, const cv::Point &pt0);

void setLabel(cv::Mat& im, const std::string &label
	, const std::vector<cv::Point>& contour
	, const cv::Scalar &color = cv::Scalar(0, 0, 0));

void setLabel(cv::Mat& im, const std::string &label
	, const cv::Point& pos, const cv::Scalar &color = cv::Scalar(0, 0, 0));

