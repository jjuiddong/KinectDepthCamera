//
// 2018-03-24, jjuiddong
// Filter View
//
#pragma once

#include "rectcontour.h"


class cFilterView : public framework::cDockWindow
{
public:
	cFilterView(const string &name);
	virtual ~cFilterView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();


protected:
	void ProcessDepth();
	void UpdateTexture();
	bool FindBox(cv::Mat &img, OUT vector<cRectContour> &out);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	vector<vector<cv::Point>> m_contours;
	graphic::cTexture m_depthTexture;

	// BoxVolume
	struct sRectInfo {
		int loop;
		float lowerH;
		float upperH;
		bool duplicate;
		cRectContour r;
	};
	vector<sRectInfo> m_rects;
	vector<sRectInfo> m_removeRects;
};
