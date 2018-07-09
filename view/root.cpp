
#include "stdafx.h"
#include "root.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip> 
#include "3dview.h"
#include "depthview.h"
#include "depthview2.h"
#include "infraredview.h"
#include "filterview.h"

using namespace common;

cRoot::cRoot()
	: m_distribCount(0)
	, m_areaCount(0)
	, m_areaFloorCnt(0)
	, m_input(eInputType::BASLER)
	, m_isAutoSaveCapture(false)
	, m_isTryConnectBasler(true)
	, m_isAutoMeasure(false)
	, m_isPalete(false)
	, m_isConnectKinect(false)
	, m_baslerCam(true)
	, m_isGrabLog(false)
	, m_isRangeCulling(false)
	, m_cullRangeMin(-200,-200,-200)
	, m_cullRangeMax(200, 200, 200)
	, m_measureId(0)
	, m_isCalcHorz(false)
	, m_plane(Vector3(0,1,0),0)
	, m_volumeCenter(0,0,0)
	, m_rangeMinMax(50, 50)
	, m_isContinuousCalibrationPlane(false)
	, m_planeStandardDeviation(0)

{
	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));
	m_projMap = cv::Mat((int)g_capture3DHeight, (int)g_capture3DWidth, CV_32FC1);

	// Camera Offset Setting, Korean Air Cargo
	//m_cameraOffset[0].pos = Vector3(-59.740f, 4.170f, -75.420f);
	//m_cameraOffset[0].rot.SetRotationY(-0.02f);
	//m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 78.090f);	 // 30cm
	//m_cameraOffset[2].pos = Vector3(0, 0, 0);

	//m_planeSub[0] = Plane(Vector3(0,1,0), 0);
	//m_planeSub[1] = Plane(Vector3(0.010194f, 0.999914f, 0.008305f), 0);
	//m_planeSub[2] = Plane(Vector3(0.01611f, 0.999722f, 0.014008f), 0);

	// Test Room Setting
	//m_planeSub[0] = Plane(Vector3(-0.006403f, 0.999976f, 0.002532f), -0.598581f);
	//m_planeSub[1] = Plane(Vector3(0, 1, 0), 0);
	//m_planeSub[2] = Plane(Vector3(0, 1, 0), 0);
}

cRoot::~cRoot()
{
	Clear();
}


bool cRoot::Create()
{
	if (m_config.Read("config_depthframe.txt"))
	{
		m_isConnectKinect = m_config.GetBool("kinect_connect", true);
		m_isTryConnectBasler = m_config.GetBool("basler_connect", true);
		m_inputFilePath = m_config.GetString("inputfilepath", "../Media/Depth");

		m_rangeCenter.x = m_config.GetFloat("calib-center-x");
		m_rangeCenter.y = m_config.GetFloat("calib-center-y");
		m_rangeCenter.z = m_config.GetFloat("calib-center-z");
		m_rangeMinMax.x = m_config.GetFloat("calib-minmax-x", 50);
		m_rangeMinMax.y = m_config.GetFloat("calib-minmax-y", 50);
	}
	else
	{
		m_inputFilePath = "../Media/Depth";
	}

	m_3dEyePos = common::Vector3(0.f, 380.f, -300.f);
	m_3dLookAt = common::Vector3(0, 0, 0);

	m_timer.Create();

	return true;
}


bool cRoot::InitSensor()
{
	if (m_isConnectKinect)
		m_kinect.Init();

	if (m_isTryConnectBasler)
	{
		m_baslerCam.Init();
	}
	else
	{
		// for BaslerCamSync::IsReadyCapture() return true
		m_baslerCam.m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
	}

	m_dbClient.Create();

	return m_kinect.IsConnect();// || m_baslerCam.IsConnect();
}


// 센서를 종료한다.
bool cRoot::DisconnectSensor()
{
	m_kinect.Clear();
	m_baslerCam.Clear();
	return true;
}


bool cRoot::KinectCapture()
{
	// nothing~ now
	return true;
}


// camIdx 카메라가 인식한 영상으로 볼륨 측정한다.
void cRoot::MeasureVolume(
	const bool isUpdateSensor //=false
)
{
	m_areaFloorCnt = 0;

	if (m_isAutoMeasure || isUpdateSensor)
	{
		graphic::cRenderer &renderer = g_root.m_3dView->GetRenderer();

		// 포인트 클라우드에서 높이 분포를 계산한다.
		// 높이분포를 이용해서 면적분포 메쉬를 생성한다.
		// 높이 별로 포인트 클라우드를 생성한다.
		cSensor *sensor = NULL;
		for (auto s : m_baslerCam.m_sensors)
		{
			if (s->IsEnable() && s->m_isShow)
				sensor = s;
		}
		if (sensor)
			sensor->m_buffer.MeasureVolume(renderer);
	}

	// Update FilterView, DepthView, DepthView2
	g_root.m_3dView->Capture3D();
	g_root.m_filterView->Process();
	g_root.m_infraredView->Process(2);
	g_root.m_filterView->CalcBoxVolumeAverage();
	//((cViewer*)g_application)->m_depthView->Process();
	//((cViewer*)g_application)->m_depthView2->Process();
}


void cRoot::Update(const float deltaSeconds)
{
}


// Load Global Ground Plane
// Calibration에 관련된 변수들을 파일에 읽어온다.
bool cRoot::LoadPlane()
{
	if (!m_config.Read("config_depthframe.txt"))
		return false;

	if (m_config.GetString("plane-x", "none") != "none")
	{
		m_plane.N.x = m_config.GetFloat("plane-x");
		m_plane.N.y = m_config.GetFloat("plane-y");
		m_plane.N.z = m_config.GetFloat("plane-z");
		m_plane.D = m_config.GetFloat("plane-d");
	}

	if (g_root.m_config.GetString("center-x", "none") != "none")
	{
		m_volumeCenter.x = m_config.GetFloat("center-x");
		m_volumeCenter.y = m_config.GetFloat("center-y");
		m_volumeCenter.z = m_config.GetFloat("center-z");
	}

	// Load PlaneSub
	for (int i = 0; i < 3; ++i)
	{
		StrId id;

		id.Format("camera-offset%d-x", i);
		m_cameraOffset[i].pos.x = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset%d-y", i);
		m_cameraOffset[i].pos.y = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset%d-z", i);
		m_cameraOffset[i].pos.z = m_config.GetFloat(id.c_str(), 0);
		id.Format("camera-offset-angle", i);
		m_cameraOffsetYAngle[i] = m_config.GetFloat(id.c_str(), 0);
		m_cameraOffset[i].rot.SetRotationY( m_cameraOffsetYAngle[i] );

		id.Format("planesub%d-x", i);
		m_planeSub[i].N.x = m_config.GetFloat(id.c_str(), 0);
		id.Format("planesub%d-y", i);
		m_planeSub[i].N.y = m_config.GetFloat(id.c_str(), 1);
		id.Format("planesub%d-z", i);
		m_planeSub[i].N.z = m_config.GetFloat(id.c_str(), 0);
		id.Format("planesub%d-d", i);
		m_planeSub[i].D = m_config.GetFloat(id.c_str(), 0);
	}

	for (u_int i = 0; i < m_baslerCam.m_sensors.size(); ++i)
	{
		auto &sensor = m_baslerCam.m_sensors[i];
		sensor->m_buffer.m_offset = m_cameraOffset[i];
		sensor->m_buffer.m_planeSub = m_planeSub[i];
	}

	return true;
}


// Calibration에 관련된 변수들을 파일에 저장한다.
bool cRoot::SavePlane()
{
	if (m_plane.N != Vector3(0,1,0))
	{
		m_config.SetValue("plane-x", m_plane.N.x);
		m_config.SetValue("plane-y", m_plane.N.y);
		m_config.SetValue("plane-z", m_plane.N.z);
		m_config.SetValue("plane-d", m_plane.D);
	}

	if (!m_volumeCenter.IsEmpty())
	{
		m_config.SetValue("center-x", m_volumeCenter.x);
		m_config.SetValue("center-y", m_volumeCenter.y);
		m_config.SetValue("center-z", m_volumeCenter.z);
	}

	// Save PlaneSub
	for (int i = 0; i < 3; ++i)
	{
		StrId id;

		if (!m_cameraOffset[i].pos.IsEmpty())
		{
			id.Format("camera-offset%d-x", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.x);
			id.Format("camera-offset%d-y", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.y);
			id.Format("camera-offset%d-z", i);
			m_config.SetValue(id.c_str(), m_cameraOffset[i].pos.z);
			id.Format("camera-offset-angle", i);
			m_config.SetValue(id.c_str(), m_cameraOffsetYAngle[i]);
		}

		if (!m_planeSub[i].N.IsEmpty())
		{
			id.Format("planesub%d-x", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.x);
			id.Format("planesub%d-y", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.y);
			id.Format("planesub%d-z", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].N.z);
			id.Format("planesub%d-d", i);
			m_config.SetValue(id.c_str(), m_planeSub[i].D);
		}
	}

	return m_config.Write("config_depthframe.txt");
}


void cRoot::GeneratePlane(common::Vector3 pos[3])
{
	Matrix44 tm;
	if (m_plane.N.IsEmpty())
	{
		Plane plane(pos[0], pos[1], pos[2]);
		m_plane = plane;
	}
	else
	{
		// old plane
		{
			Quaternion q;
			q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
			tm *= q.GetMatrix();
			Matrix44 T;
			T.SetPosition(Vector3(0, m_plane.D, 0));
			tm *= T;
		}

		tm.Inverse2();

		common::Vector3 p[3];
		p[0] = pos[0] * tm;
		p[1] = pos[1] * tm;
		p[2] = pos[2] * tm;
		Plane plane(p[0], p[1], p[2]);


		// new plane
		//{
		//	Quaternion q;
		//	q.SetRotationArc(plane.N, Vector3(0, 1, 0));
		//	tm *= q.GetMatrix();
		//	//Matrix44 T;
		//	//T.SetPosition(Vector3(0, plane.D, 0));
		//	//tm *= T;
		//}
		//Vector3 N = Vector3(0, 1, 0) * tm.Inverse();
		//Plane p(N.Normal(), 0);
		m_plane = plane;
	}
}


void cRoot::Clear()
{
	for (auto p : m_areaBuff)
	{
		delete p->vtxBuff;
		delete p;
	}
	m_areaBuff.clear();

	m_kinect.Clear();
	m_baslerCam.Clear();
	
	m_config.SetValue("kinect_connect", m_isConnectKinect);
	m_config.SetValue("basler_connect", m_isTryConnectBasler);
	m_config.SetValue("inputfilepath", m_inputFilePath.c_str());
	m_config.Write("config_depthframe.txt");
}
