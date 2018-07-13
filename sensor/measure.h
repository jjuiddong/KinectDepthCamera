//
// 2018-07-11, jjuiddong
// measure volume
//
#pragma once

#include "volume.h"


// cMeasure class
class cMeasure
{
public:
	cMeasure();
	virtual ~cMeasure();

	bool MeasureVolume();
	void CalcBoxVolumeAverage();
	void ClearBoxVolumeAverage();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void Clear();


protected:
	void CalcHeightDistribute();
	void Measure2DImage();
	void RemoveDuplicateContour(vector<sContourInfo> &contours);
	sBoxInfo CalcBoxInfo(const sContourInfo &info);
	void RenderContourRect(cv::Mat &dst, const vector<sContourInfo> &contours);


public:
	int m_measureId; // 측정 버튼을 누를때 마다 1씩 증가
	cv::Mat m_projMap; // change space, (orthogonal projection map)
	cv::Mat m_binImg;
	cv::Mat m_dstImg;

	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2000]; // 0 ~ 2000 분포, 1mm 단위, (ex) m_hDistrib[100] = 높이 10cm 위치의 분포)
	float m_hDistrib2[2000]; // height distribution pulse
	float m_hAverage[2000]; // 1mm 단위 분포에서의 높이 평균 
	sGraph<2000> m_hDistribDifferential; // 2 differential
	vector<sBoxInfo> m_boxes; // 현재 인식된 박스 정보
	vector<sBoxInfo> m_boxesStored; // 평균으로 계산된 박스 정보
	vector<sMeasureResult> m_results;
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;

	vector<sContourInfo> m_contours; // 현재 인식된 박스 정보
	vector<sContourInfo> m_removeRects;
	vector<sAvrContour> m_avrContours; // 평균으로 계산된 박스 정보
	int m_calcAverageCount;

	float m_volDistrib[2000]; // scale:10, 소수점 첫번째 자리까지 분포표시, 0.01 ~ 20 volume
	common::CriticalSection m_cs;
};
