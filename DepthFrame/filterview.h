//
// 2018-03-24, jjuiddong
// Depth View
//
#pragma once


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


public:
	cv::Mat m_srcImg;
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	cv::Mat m_edges;
	vector<vector<cv::Point> > m_contours;

	graphic::cTexture m_depthTexture;
	//int m_thresholdMin;
	//int m_thresholdMax;
};
