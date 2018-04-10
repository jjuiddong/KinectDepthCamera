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
	void CalcBoxVolumeAverage();
	void ClearBoxVolumeAverage();


public:
	// BoxVolume
	struct sContourInfo {
		bool used; // use internal
		int level;
		int loop;
		float lowerH;
		float upperH;
		bool duplicate;
		graphic::cColor color;
		cContour contour;
	};


protected:
	void ProcessDepth(const size_t camIdx=0);
	void UpdateTexture();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void RemoveDuplicateContour(vector<sContourInfo> &contours);
	cRoot::sBoxInfo CalcBoxInfo(const sContourInfo &info);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	graphic::cTexture m_depthTexture;
	vector<sContourInfo> m_contours;
	vector<sContourInfo> m_removeRects;

	// Box Volume Average
	struct sAvrContour {
		bool check;
		int count;
		sContourInfo box;
		cRoot::sBoxInfo result;
		common::Vector2 avrVertices[8]; // 평균 버텍스 위치, 최대 8개 꼭지점
		common::Vector3 vertices3d[8*2]; // 꼭지점 평균 버텍스 3D 위치
	};
	vector<sAvrContour> m_avrContours;
	int m_calcAverageCount;
};
