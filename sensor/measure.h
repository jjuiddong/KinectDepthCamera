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
	enum MEASURE_TYPE {
		OBJECT		// Detect Object Shape, and then Measure Volume
		, INTEGRAL  // No Detect Object, Integral total 2D Project HeightMap
		, BOTH		// Object,Integral both measure
	};


	cMeasure();
	virtual ~cMeasure();

	bool MeasureVolume(const MEASURE_TYPE type, const bool isForceMeasure = false);
	void CalcBoxVolumeAverage();
	void ClearBoxVolumeAverage();
	bool FindBox(cv::Mat &img, const u_int vtxCnt, OUT vector<cContour> &out);
	void DrawContourRect();
	void Clear();


protected:
	void CalcHeightDistribute();
	void Measure2DImage();
	void MeasureIntegral();
	void RemoveDuplicateContour(vector<sContourInfo> &contours);
	sBoxInfo CalcBoxInfo(const sContourInfo &info);
	void RenderContourRect(cv::Mat &dst, const vector<sContourInfo> &contours
		, const int offsetId=0);
	cv::Rect FindBiggestBlob(cv::Mat &src);


public:
	int m_measureId; // 측정 버튼을 누를때 마다 1씩 증가
	cv::Mat m_projMap; // change space, (orthogonal projection map)
	cv::Mat m_binImg;
	cv::Mat m_dstImg;

	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2200]; // 0 ~ 2200 분포, 1mm 단위, (ex) m_hDistrib[100] = 높이 10cm 위치의 분포)
	float m_hDistrib2[2200]; // height distribution pulse
	float m_hAverage[2200]; // 1mm 단위 분포에서의 높이 평균 
	sGraph<2200> m_hDistribDifferential; // 2 differential
	int m_offsetDistrib; // for -200 ~ 2000 (calibrationview calc height distribution)

	// OBJECT MEASURE
	vector<sBoxInfo> m_boxes; // 현재 인식된 박스 정보 (update Measure2DImage())
	vector<sBoxInfo> m_boxesStored; // 평균으로 계산된 박스 정보, 외부에서 세팅 (cInputView)
	vector<sMeasureResult> m_results; // cInputView 에서 세팅
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;
	
	// INTEGRAL MEASURE
	cv::Rect m_projImageRoi;
	float m_integralVW;

	// Box Raw Data
	vector<sContourInfo> m_contours; // 현재 인식된 박스 정보
	vector<sContourInfo> m_removeContours; // 중복 제거된 박스 정보
	vector<sContourInfo> m_beforeRemoveContours; // 중복 제거되기전 박스 정보
	vector<sAvrContour> m_avrContours; // 평균으로 계산된 박스 정보
	int m_calcAverageCount;

	float m_volDistrib[2000]; // scale:10, 소수점 첫번째 자리까지 분포표시, 0.01 ~ 20 volume
	float m_horzDistrib[2000]; // 가로 분포, 1mm ~ 2000mm
	float m_vertDistrib[2000]; // 세로 분포, 1mm ~ 2000mm
	common::CriticalSection m_cs;
};
