//
// 2018-03-24, jjuiddong
// Depth View
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
	void ProcessDepth();


protected:
	void ProcessDepth(INT64 nTime
		, const common::Vector3 *pBuffer
		, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);
	void UpdateTexture();
	bool FindBox(cv::Mat &img, OUT vector<cRectContour> &out);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	cv::Mat m_edges;
	vector<vector<cv::Point> > m_contours;
	graphic::cTexture m_depthTexture;

	// BoxVolume
	struct sRectInfo {
		int loop;
		float h;
		bool duplicate;
		cRectContour r;
	};
	vector<sRectInfo> m_rects;
	vector<sRectInfo> m_removeRects;

	struct sBoxInfo {
		common::Vector3 pos;
		common::Vector3 volume;
	};
	vector<sBoxInfo> m_boxes;
};
