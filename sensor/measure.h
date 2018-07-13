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
	int m_measureId; // ���� ��ư�� ������ ���� 1�� ����
	cv::Mat m_projMap; // change space, (orthogonal projection map)
	cv::Mat m_binImg;
	cv::Mat m_dstImg;

	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2000]; // 0 ~ 2000 ����, 1mm ����, (ex) m_hDistrib[100] = ���� 10cm ��ġ�� ����)
	float m_hDistrib2[2000]; // height distribution pulse
	float m_hAverage[2000]; // 1mm ���� ���������� ���� ��� 
	sGraph<2000> m_hDistribDifferential; // 2 differential
	vector<sBoxInfo> m_boxes; // ���� �νĵ� �ڽ� ����
	vector<sBoxInfo> m_boxesStored; // ������� ���� �ڽ� ����
	vector<sMeasureResult> m_results;
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;

	vector<sContourInfo> m_contours; // ���� �νĵ� �ڽ� ����
	vector<sContourInfo> m_removeRects;
	vector<sAvrContour> m_avrContours; // ������� ���� �ڽ� ����
	int m_calcAverageCount;

	float m_volDistrib[2000]; // scale:10, �Ҽ��� ù��° �ڸ����� ����ǥ��, 0.01 ~ 20 volume
	common::CriticalSection m_cs;
};
