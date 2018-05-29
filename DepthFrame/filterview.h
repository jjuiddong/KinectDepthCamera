//
// 2018-03-24, jjuiddong
// Filter View
//
#pragma once


class cFilterView : public framework::cDockWindow
{
public:
	cFilterView(const string &name);
	virtual ~cFilterView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();
	void CalcBoxVolumeAverage();
	void ClearBoxVolumeAverage();


public:
	void ProcessDepth();
	void UpdateTexture();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void RemoveDuplicateContour(vector<sContourInfo> &contours);
	cRoot::sBoxInfo CalcBoxInfo(const sContourInfo &info);


public:
	cv::Mat m_binImg;
	cv::Mat m_dstImg;
	graphic::cTexture m_depthTexture;
	vector<sContourInfo> m_contours; // 현재 인식된 박스 정보
	vector<sContourInfo> m_removeRects;

	// Box Volume Average
	struct sAvrContour {
		bool check;
		int count;
		sContourInfo box;
		cRoot::sBoxInfo result;
		common::Vector2 avrVertices[13]; // 평균 버텍스 위치, 최대 8개 꼭지점
		common::Vector3 vertices3d[13*2]; // 꼭지점 평균 버텍스 3D 위치
	};
	vector<sAvrContour> m_avrContours; // 평균으로 계산된 박스 정보
	int m_calcAverageCount;
	common::CriticalSection m_cs;
};
