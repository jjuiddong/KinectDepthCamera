//
// 2018-03-13, jjuiddong
// Global class
//
#pragma once

#include "basler.h"
#include "baslersync.h"
#include "kinectv2.h"


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

static const float g_capture3DWidth = 640.f * 1.f;
static const float g_capture3DHeight = 480.f * 1.f;


class cRoot
{
public:
	cRoot();
	virtual ~cRoot();

	bool Create();
	bool InitSensor();
	void Update(graphic::cRenderer &renderer, const float deltaSeconds);
	bool BaslerCapture();
	bool KinectCapture();
	void MeasureVolume(const size_t camIdx=0, const bool isUpdateSensor=false);
	void Clear();


public:
	struct eInputType { enum Enum { FILE, KINECT, BASLER }; };

	eInputType::Enum m_input;
	common::Vector3 m_3dEyePos;
	common::Vector3 m_3dLookAt;
	cSensorBuffer m_sensorBuff[2];

	// Update Every Time
	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2000]; // 0 ~ 2000 분포, 0.1cm 단위
	float m_hDistrib2[2000]; // height distribution pulse
	sGraph<2000> m_hDistribDifferential; // 2 differential

	struct sAreaFloor
	{
		int startIdx;
		int endIdx;
		int maxIdx; // 가장 많이 분포한 높이 인덱스
		int areaCnt;
		graphic::cColor color;
		sGraph<100> areaGraph;
		graphic::cVertexBuffer *vtxBuff;
	};
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;

	struct sBoxInfo {
		float minVolume;
		float maxVolume;
		common::Vector3 pos;
		common::Vector3 volume;
		common::Vector3 box3d[8*2];
		u_int pointCnt;
		graphic::cColor color;
	};
	vector<sBoxInfo> m_boxes;
	vector<sBoxInfo> m_boxesStored;

	// Kinect
	bool m_isConnectKinect;
	cKinect m_kinect;

	// Basler
	bool m_isTryConnectBasler;
	cBaslerCameraSync m_balserCam;
	//cBaslerCamera m_balserCam;

	// Option
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;
	bool m_isPalete;

	// Config
	common::StrPath m_inputFilePath;
	common::cConfig m_config;
};
