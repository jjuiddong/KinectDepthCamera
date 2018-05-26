//
// 2018-03-13, jjuiddong
// Global class
//
#pragma once

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
	void Update(const float deltaSeconds);
	bool KinectCapture();
	void MeasureVolume(const bool isUpdateSensor=false);
	void Clear();


public:
	struct eInputType { enum Enum { FILE, KINECT, BASLER }; };

	eInputType::Enum m_input;
	common::Vector3 m_3dEyePos;
	common::Vector3 m_3dLookAt;

	cv::Mat m_projMap; // change space, (orthogonal projection map)
	common::Transform m_cameraOffset[3]; // camera1 offset
	common::Plane m_planeSub[3];

	common::cTimer m_timer;
	
	// Update Every Time
	int m_distribCount;
	int m_areaCount;
	float m_hDistrib[2000]; // 0 ~ 2000 분포, 0.1cm 단위, m_hDistrib[100] = 높이 10cm 위치의 분포
	float m_hDistrib2[2000]; // height distribution pulse
	sGraph<2000> m_hDistribDifferential; // 2 differential

	struct sAreaFloor
	{
		int startIdx; // start height
		int endIdx;  // end height
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
		int loopCnt;
		graphic::cColor color;
	};
	vector<sBoxInfo> m_boxes; // 현재 인식된 박스 정보
	vector<sBoxInfo> m_boxesStored; // 평균으로 계산된 박스 정보

	// Kinect
	bool m_isConnectKinect;
	cKinect m_kinect;

	// Basler
	bool m_isTryConnectBasler;
	cBaslerCameraSync m_baslerCam;

	// Option
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;
	bool m_isPalete;
	bool m_isGrabLog;

	// Config
	common::StrPath m_inputFilePath;
	bool m_isRangeCulling;
	common::Vector3 m_cullRangeMin;
	common::Vector3 m_cullRangeMax;
	common::cConfig m_config;
};


// 재귀 평균
static double CalcAverage(const int k, const double Avr, const double Xk)
{
	const double alpha = (double)(k - 1.f) / (double)k;
	return alpha * Avr + (1.f - alpha) * Xk;
}
