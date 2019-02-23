//
// 2018-03-13, jjuiddong
// Global class
//
#pragma once

#include "../sensor/measure.h"


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

class c3DView;
class cColorView;
class cDepthView;
class cDepthView2;
class cInfraredView;
class cAnalysisView;
class cInputView;
class cAnimationView;
class cFilterView;
class cLogView;
class cResultView;
class cBoxView;
class cCameraView;
class cCalibrationView;

class cRoot
{
public:
	cRoot();
	virtual ~cRoot();

	bool Create();
	bool InitSensor();
	bool DisconnectSensor();
	void Update(const float deltaSeconds);
	bool KinectCapture();
	void MeasureVolume(const bool isForceMeasure = false);
	bool LoadPlane();
	bool SavePlane();
	void GeneratePlane(common::Vector3 pos[3]);
	cSensor* GetFirstVisibleSensor();
	void Clear();


public:
	struct eInputType { enum Enum { FILE, KINECT, BASLER }; };

	eInputType::Enum m_input;
	common::Vector3 m_3dEyePos;
	common::Vector3 m_3dLookAt;
	common::cTimer m_timer;
	HWND m_hwnd;
	
	// Kinect
	bool m_isConnectKinect;
	//cKinect m_kinect;

	// Basler
	bool m_isTryConnectBasler;
	cBaslerCameraSync m_baslerCam;

	// Database
	cDBClient m_dbClient;

	// Measure
	cMeasure m_measure;
	cMeasure::MEASURE_TYPE m_measureType;

	// Option
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;
	bool m_isPalete;
	bool m_isGrabLog;
	bool m_isGrabErrLog;
	bool m_isCalcHorz;
	bool m_isSave2DMat;

	// Ground Calibration
	int m_masterSensor; // Master Sensor Index for Animation, Volume Measure Calculation
	common::StrPath m_configFileName;
	common::Plane m_groundPlane;
	common::Plane m_planeSub[ MAX_CAMERA];
	common::Vector3 m_volumeCenter;
	common::Transform m_cameraOffset[ MAX_CAMERA];
	float m_cameraOffsetYAngle[ MAX_CAMERA];
	int m_devChannel[MAX_CAMERA]; // DeviceChannel
	common::sRectf m_cullRect[MAX_CAMERA];
	common::sRectf m_extraCullRect[MAX_CAMERA];

	cCalibration m_calib;
	common::Vector3 m_regionCenter;
	common::Vector2 m_regionSize;
	double m_planeStandardDeviation;
	bool m_isContinuousCalibrationPlane; // calibration 된 정보를 평균화해서 출력한다.
	common::Vector3 m_hdistribCenter; // CalibrationView height distribution region center
	common::Vector2 m_hdistribSize; // CalibrationView height distribution region size

	graphic::cShader11 m_tessPos; // ProjectionMap Tessellation Shader
	graphic::cVertexBuffer m_projVtxBuff; // ProjectionMap VertexBuffer
	common::Vector3 m_projRoi[4];

	// Config
	common::StrPath m_inputFilePath;
	bool m_isRangeCulling;
	bool m_isShowBoxVertex; // filterview, show box vertex, default=true
	bool m_isShowBox; // filterview, show box vertex, default=true
	bool m_isShowBoxCandidate; // filterview, show candidate box vertex, default=true
	bool m_isShowBeforeContours; // filterview, show before remove duplicate contours array
	common::Vector3 m_cullRangeMin;
	common::Vector3 m_cullRangeMax;
	common::cConfig m_config;

	// *.PCD Write Thread
	common::cThread m_pcdWriteThread; // not used

	// View
	c3DView *m_3dView;
	cColorView *m_colorView;
	cDepthView *m_depthView;
	cDepthView2 *m_depthView2;
	cInfraredView *m_infraredView;
	cAnalysisView *m_analysisView;
	cInputView *m_inputView;
	cAnimationView *m_aniView;
	cFilterView *m_filterView;
	cLogView *m_logView;
	cResultView *m_resultView;
	cBoxView *m_boxView;
	cCameraView *m_camView;
	cCalibrationView *m_calibView;
};


// 재귀 평균
static double CalcAverage(const int k, const double Avr, const double Xk)
{
	const double alpha = (double)(k - 1.f) / (double)k;
	return alpha * Avr + (1.f - alpha) * Xk;
}
