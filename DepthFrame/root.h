//
// 2018-03-13, jjuiddong
// Global class
//
#pragma once

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

static const float g_capture3DWidth = 640.f * 1.f;
static const float g_capture3DHeight = 480.f * 1.f;


class cRoot
{
public:
	cRoot();
	virtual ~cRoot();

	bool Create();
	void Update(graphic::cRenderer &renderer, const float deltaSeconds);
	bool BaslerCapture();
	bool KinectCapture();
	void MeasureVolume(const bool isUpdateSensor=false);
	void Clear();


protected:
	bool KinectSetup();
	int BaslerCameraSetup();
	void setupCamera();
	void processData(const GrabResult& grabResult);
	void UpdateDepthImage(graphic::cRenderer &renderer);


public:
	struct eInputType { enum Enum { FILE, KINECT, BASLER }; };

	eInputType::Enum m_input;

	cSensorBuffer m_sensorBuff;

	// Update Every Time
	INT64 m_nTime;
	USHORT m_nDepthMinReliableDistance;
	USHORT m_nDepthMaxDistance;
	int m_depthThresholdMin;
	int m_depthThresholdMax;
	int m_distribCount;
	int m_areaCount;
	int m_heightErr[2]; // Upper, Lower
	float m_hDistrib[2000]; // 0 ~ 2000 분포, 0.1cm 단위
	float m_hDistrib2[2000]; // height distribution pulse
	sGraph<2000> m_hDistribDifferential; // 2 differential

	struct sAreaFloor
	{
		int startIdx;
		int endIdx;
		int maxIdx; // 가장 많이 분포한 높이 인덱스
		int areaCnt;
		sGraph<100> areaGraph;
		graphic::cVertexBuffer *vtxBuff;
	};
	vector<sAreaFloor*> m_areaBuff;
	int m_areaFloorCnt;

	struct sBoxInfo {
		common::Vector3 pos;
		common::Vector3 volume;
	};
	vector<sBoxInfo> m_boxes;
	vector<sBoxInfo> m_boxesStored;

	// Kinect
	bool m_isConnectKinect;
	bool m_kinectSetupSuccess;
	IKinectSensor *m_pKinectSensor;
	IDepthFrameReader *m_pDepthFrameReader;
	IColorFrameReader *m_pColorFrameReader;
	IInfraredFrameReader *m_pInfraredFrameReader;

	// Basler
	bool m_isConnectBasler;
	CToFCamera *m_Camera;
	bool m_baslerSetupSuccess;

	// Option
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;
	bool m_isPalete;

	// Box3D
	common::Vector3 m_box3DPos[8];

	// Config
	common::StrPath m_inputFilePath;
	common::cConfig m_config;
};
