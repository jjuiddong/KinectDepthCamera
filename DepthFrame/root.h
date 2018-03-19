//
// 2018-03-13, jjuiddong
//
#pragma once

#include "graphbuff.h"


class cRoot
{
public:
	cRoot();
	virtual ~cRoot();

	bool Create();
	void Update(const float deltaSeconds);
	void Clear();


protected:
	void UpdateDepthImage();


public:
	struct eInputType {
		enum Enum {
			FILE, KINECT, BASLER
		};
	};

	eInputType::Enum m_input;
	bool m_isUpdate;

	// Kinect
	IKinectSensor *m_pKinectSensor;
	IDepthFrameReader *m_pDepthFrameReader;
	IColorFrameReader *m_pColorFrameReader;
	IInfraredFrameReader *m_pInfraredFrameReader;
	USHORT *m_pDepthBuff;
	//

	// Update Every Time
	INT64 m_nTime;
	USHORT m_nDepthMinReliableDistance;
	USHORT m_nDepthMaxDistance;
	int m_depthThresholdMin;
	int m_depthThresholdMax;
	float m_depthDensity;
	int m_distribCount;
	int m_areaCount;
	int m_heightErr[2]; // Upper, Lower
	float m_hDistrib[150]; // 0 ~ 10 분포, 0.1 단위
	int m_areaMin;
	int m_areaMax;
	sGraph<150> m_hDistribDifferential; // 2 differential
	sGraph<100> m_areaGraph;

	struct sAreaFloor
	{
		int areaCnt;
		int areaMin;
		int areaMax;
		sGraph<100> areaGraph;
		graphic::cVertexBuffer *vtxBuff;
	};
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;
};


// Kinect V2
static const int cDepthWidth = 512;
static const int cDepthHeight = 424;
static const int cColorWidth = 1920;
static const int cColorHeight = 1080;
static const int cInfraredWidth = 512;
static const int cInfraredHeight = 424;
