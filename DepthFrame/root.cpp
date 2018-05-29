
#include "stdafx.h"
#include "root.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip> 
#include "3dview.h"
#include "depthframe.h"
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
{
	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));
	m_projMap = cv::Mat((int)g_capture3DHeight, (int)g_capture3DWidth, CV_32FC1);

	//m_cameraOffset[0].pos = Vector3(-57.9f, 2.8f, -71.3f);
	//m_cameraOffset[0].pos = Vector3(-61.11f, 4.17f, -72.11f);
	m_cameraOffset[0].pos = Vector3(-59.740f, 4.170f, -75.420f);
	m_cameraOffset[0].rot.SetRotationY(-0.02f);

	//m_cameraOffset[1].pos = Vector3(56.56f, -4.2f, 77.38f);
	//m_cameraOffset[1].pos = Vector3(56.56f, -2.2f, 77.38f);
	//m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 76.460f);	 // 0m
	//m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 75.970f); // 1m

	//m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 78.090f);	 // 30cm
	m_cameraOffset[1].pos = Vector3(55.030f, -2.200f, 78.090f);	 // 30cm

	m_cameraOffset[2].pos = Vector3(0, 0, 0);

	m_planeSub[0] = Plane(Vector3(0,1,0), 0);
	m_planeSub[1] = Plane(Vector3(0.010194f, 0.999914f, 0.008305f), 0);
	m_planeSub[2] = Plane(Vector3(0.01611f, 0.999722f, 0.014008f), 0);
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
		graphic::cRenderer &renderer = ((cViewer*)g_application)->m_3dView->GetRenderer();

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
	((cViewer*)g_application)->m_3dView->Capture3D();
	((cViewer*)g_application)->m_filterView->Process();
	((cViewer*)g_application)->m_infraredView->Process(2);
	((cViewer*)g_application)->m_filterView->CalcBoxVolumeAverage();
	//((cViewer*)g_application)->m_depthView->Process();
	//((cViewer*)g_application)->m_depthView2->Process();
}


void cRoot::Update(const float deltaSeconds)
{
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
