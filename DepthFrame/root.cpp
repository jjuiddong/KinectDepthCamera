
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
	, m_input(eInputType::FILE)
	, m_isAutoSaveCapture(false)
	, m_isTryConnectBasler(true)
	, m_isAutoMeasure(false)
	, m_isPalete(false)
	, m_isConnectKinect(false)
	, m_baslerCameraIdx(0)
	, m_balserCam(true)
{
	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));

	m_cameraOffset2.pos = Vector3(-57.9f, 2.8f, -71.3f);
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

	return true;
}


bool cRoot::InitSensor()
{
	if (m_isConnectKinect)
		m_kinect.Init();
	if (m_isTryConnectBasler)
		m_balserCam.Init();
	return m_kinect.IsConnect() || m_balserCam.IsConnect();
}


bool cRoot::BaslerCapture()
{
	m_balserCam.Capture();
	return true;
}


bool cRoot::KinectCapture()
{
	// nothing~ now
	return true;
}


void cRoot::MeasureVolume(
	const size_t camIdx  //=0
	, const bool isUpdateSensor //=false
)
{
	m_areaFloorCnt = 0;

	if (m_isAutoMeasure || isUpdateSensor)
	{
		graphic::cRenderer &renderer = ((cViewer*)g_application)->m_3dView->GetRenderer();
		m_sensorBuff[camIdx].MeasureVolume(renderer);
	}

	// Update FilterView, DepthView, DepthView2
	((cViewer*)g_application)->m_3dView->Capture3D(camIdx);
	((cViewer*)g_application)->m_filterView->Process(camIdx);
	((cViewer*)g_application)->m_infraredView->Process(camIdx);

	//((cViewer*)g_application)->m_depthView->Process();
	//((cViewer*)g_application)->m_depthView2->Process();
}


void cRoot::Update(graphic::cRenderer &renderer, const float deltaSeconds)
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
	m_balserCam.Clear();
	
	m_config.SetValue("kinect_connect", m_isConnectKinect);
	m_config.SetValue("basler_connect", m_isTryConnectBasler);
	m_config.SetValue("inputfilepath", m_inputFilePath.c_str());
	m_config.Write("config_depthframe.txt");
}
