//
// 2018-03-13, jjuiddong
// Global class
//
#pragma once


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
	void MeasureVolume(const bool isUpdateSensor=false);
	bool LoadPlane();
	bool SavePlane();
	void GeneratePlane(common::Vector3 pos[3]);
	void Clear();


public:
	struct eInputType { enum Enum { FILE, KINECT, BASLER }; };

	eInputType::Enum m_input;
	common::Vector3 m_3dEyePos;
	common::Vector3 m_3dLookAt;
	int m_measureId; // 측정 버튼을 누를때 마다 1씩 증가

	cv::Mat m_projMap; // change space, (orthogonal projection map)
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
		//common::Vector3 box3d[8*2];
		common::Vector3 box3d[13 * 2];
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

	// Database
	cDBClient m_dbClient;

	// Option
	bool m_isAutoSaveCapture;
	bool m_isAutoMeasure;
	bool m_isPalete;
	bool m_isGrabLog;
	bool m_isCalcHorz;

	// Ground Calibration
	common::Plane m_plane;
	common::Plane m_planeSub[3];
	common::Vector3 m_volumeCenter;
	common::Transform m_cameraOffset[3];
	float m_cameraOffsetYAngle[3];

	cCalibration m_calib;
	common::Vector3 m_rangeCenter;
	common::Vector2 m_rangeMinMax;
	double m_planeStandardDeviation;
	bool m_isContinuousCalibrationPlane; // calibration 된 정보를 평균화해서 출력한다.

	// Config
	common::StrPath m_inputFilePath;
	bool m_isRangeCulling;
	common::Vector3 m_cullRangeMin;
	common::Vector3 m_cullRangeMax;
	common::cConfig m_config;

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
