//
// 2018-03-13, jjuiddong
//
#pragma once

#include "graphbuff.h"
#include "sensorbuffer.h"
#include "datreader.h"
#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;

// Kinect V2
static const int g_kinectDepthWidth = 512;
static const int g_kinectDepthHeight = 424;
static const int g_kinectColorWidth = 1920;
static const int g_kinectColorHeight = 1080;
static const int g_kinectInfraredWidth = 512;
static const int g_kinectInfraredHeight = 424;

static const int g_baslerDepthWidth = 640;
static const int g_baslerDepthHeight = 480;
static const int g_baslerColorWidth = 640;
static const int g_baslerColorHeight = 480;


class cRoot
{
public:
	cRoot();
	virtual ~cRoot();

	bool Create();
	void Update(graphic::cRenderer &renderer, const float deltaSeconds);
	bool BaslerCapture();
	void Clear();


protected:
	int BaslerCameraSetup();
	void setupCamera();
	void processData(const GrabResult& grabResult);
	void UpdateDepthImage(graphic::cRenderer &renderer);


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
	//

	cSensorBuffer m_sensorBuff;

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
	float m_hDistrib[2000]; // 0 ~ 2000 분포, 0.1cm 단위
	sGraph<2000> m_hDistribDifferential; // 2 differential

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

	// Basler
	bool m_baslerSetupSuccess;
	CToFCamera *m_Camera;
	bool m_isConnectBasler;
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;

	// Config
	common::cConfig m_config;
};
