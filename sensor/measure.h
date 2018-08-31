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
	int m_measureId; // ���� ��ư�� ������ ���� 1�� ����
	cv::Mat m_projMap; // change space, (orthogonal projection map)
	cv::Mat m_binImg;
	cv::Mat m_dstImg;

	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2200]; // 0 ~ 2200 ����, 1mm ����, (ex) m_hDistrib[100] = ���� 10cm ��ġ�� ����)
	float m_hDistrib2[2200]; // height distribution pulse
	float m_hAverage[2200]; // 1mm ���� ���������� ���� ��� 
	sGraph<2200> m_hDistribDifferential; // 2 differential
	int m_offsetDistrib; // for -200 ~ 2000 (calibrationview calc height distribution)

	// OBJECT MEASURE
	vector<sBoxInfo> m_boxes; // ���� �νĵ� �ڽ� ���� (update Measure2DImage())
	vector<sBoxInfo> m_boxesStored; // ������� ���� �ڽ� ����, �ܺο��� ���� (cInputView)
	vector<sMeasureResult> m_results; // cInputView ���� ����
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;
	
	// INTEGRAL MEASURE
	cv::Rect m_projImageRoi;
	float m_integralVW;

	// Box Raw Data
	vector<sContourInfo> m_contours; // ���� �νĵ� �ڽ� ����
	vector<sContourInfo> m_removeContours; // �ߺ� ���ŵ� �ڽ� ����
	vector<sContourInfo> m_beforeRemoveContours; // �ߺ� ���ŵǱ��� �ڽ� ����
	vector<sAvrContour> m_avrContours; // ������� ���� �ڽ� ����
	int m_calcAverageCount;

	float m_volDistrib[2000]; // scale:10, �Ҽ��� ù��° �ڸ����� ����ǥ��, 0.01 ~ 20 volume
	float m_horzDistrib[2000]; // ���� ����, 1mm ~ 2000mm
	float m_vertDistrib[2000]; // ���� ����, 1mm ~ 2000mm
	common::CriticalSection m_cs;
};
