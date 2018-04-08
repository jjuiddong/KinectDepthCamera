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
	void Process(const size_t camIdx = 0);


protected:
	// BoxVolume
	struct sContourInfo {
		bool used; // use internal
		int level;
		int loop;
		float lowerH;
		float upperH;
		bool duplicate;
		graphic::cColor color;
		cRectContour r;
		cContour contour;
	};

	void ProcessDepth(const size_t camIdx=0);
	void UpdateTexture();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void RemoveDuplicateContour(vector<sContourInfo> &contours);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	graphic::cTexture m_depthTexture;
	vector<sContourInfo> m_contours;
	vector<sContourInfo> m_removeRects;
};
