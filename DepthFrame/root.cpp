
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



cRoot::cRoot()
	: m_pKinectSensor(NULL)
	, m_pDepthFrameReader(NULL)
	, m_pColorFrameReader(NULL)
	, m_pInfraredFrameReader(NULL)
	, m_distribCount(0)
	, m_areaCount(0)
	, m_areaFloorCnt(0)
	, m_input(eInputType::FILE)
	, m_baslerSetupSuccess(false)
	, m_isAutoSaveCapture(false)
	, m_isConnectBasler(true)
	, m_isAutoMeasure(false)
	, m_isPalete(false)
	, m_isConnectKinect(false)
	, m_kinectSetupSuccess(false)
{
	//m_depthThresholdMin = 0;
	//m_depthThresholdMax = USHORT_MAX;
	m_depthThresholdMin = 440;
	m_depthThresholdMax = 945;
	m_heightErr[0] = 80;
	m_heightErr[1] = 30;

	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistrib2, sizeof(m_hDistrib2));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));
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
		m_isConnectBasler = m_config.GetBool("basler_connect", true);
		m_inputFilePath = m_config.GetString("inputfilepath", "../Media/Depth");
	}
	else
	{
		m_inputFilePath = "../Media/Depth";
	}

	if (m_isConnectKinect)
	{
		m_kinectSetupSuccess = KinectSetup();
	}

	if (m_isConnectBasler)
	{
		m_baslerSetupSuccess = m_balserCam.Init();
	}

	m_3dEyePos = common::Vector3(0.f, 380.f, -300.f);
	m_3dLookAt = common::Vector3(0, 0, 0);

	return true;
}


bool cRoot::KinectSetup()
{
	HRESULT hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return false;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get the depth reader
		IDepthFrameSource* pDepthFrameSource = NULL;
		IColorFrameSource * pColorFrameSource = NULL;
		IInfraredFrameSource * pInfraredFrameSource = NULL;

		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pInfraredFrameSource->OpenReader(&m_pInfraredFrameReader);
		}

		SAFE_RELEASE(pDepthFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		MessageBox(NULL, L"No ready Kinect found!", L"Error", MB_OK);
		return false;
	}

	return true;
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
	const bool isUpdateSensor //=false
)
{
	m_areaFloorCnt = 0;

	if (m_isAutoMeasure || isUpdateSensor)
	{
		graphic::cRenderer &renderer = ((cViewer*)g_application)->m_3dView->GetRenderer();
		m_sensorBuff.MeasureVolume(renderer);
	}

	// Update FilterView, DepthView, DepthView2
	((cViewer*)g_application)->m_3dView->Capture3D();
	((cViewer*)g_application)->m_filterView->Process();
	((cViewer*)g_application)->m_infraredView->Process();
	//((cViewer*)g_application)->m_depthView->Process();
	//((cViewer*)g_application)->m_depthView2->Process();
}


void cRoot::Update(graphic::cRenderer &renderer, const float deltaSeconds)
{
	UpdateDepthImage(renderer);
}


void cRoot::UpdateDepthImage(graphic::cRenderer &renderer)
{
	if (!m_pDepthFrameReader)
		return;

	IDepthFrame* pDepthFrame = NULL;
	HRESULT hr = g_root.m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
	if (FAILED(hr))
		return;

	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	UINT nBufferSize = g_kinectDepthHeight * g_kinectDepthWidth;
	UINT16 *pBuffer = NULL;

	hr = pDepthFrame->get_RelativeTime(&m_nTime);
	if (FAILED(hr))
		goto error;

	hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Width(&nWidth);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Height(&nHeight);
	if (FAILED(hr))
		goto error;

	hr = pDepthFrame->get_DepthMinReliableDistance(&m_nDepthMinReliableDistance);
	if (FAILED(hr))
		goto error;

	// In order to see the full range of depth (including the less reliable far field depth)
	// we are setting nDepthMaxDistance to the extreme potential depth threshold
	m_nDepthMaxDistance = USHRT_MAX;

	// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
	hr = pDepthFrame->get_DepthMaxReliableDistance(&m_nDepthMaxDistance);

	hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	if (FAILED(hr))
		goto error;

	if (USHORT_MAX == m_depthThresholdMax)
		m_depthThresholdMax = m_nDepthMaxDistance / 4;

	m_sensorBuff.ReadKinectSensor(renderer, m_nTime, pBuffer, m_nDepthMinReliableDistance, m_nDepthMaxDistance);

error:
	SAFE_RELEASE(pFrameDescription);
	SAFE_RELEASE(pDepthFrame);
}


void cRoot::Clear()
{
	for (auto p : m_areaBuff)
	{
		delete p->vtxBuff;
		delete p;
	}
	m_areaBuff.clear();

	// done with depth frame reader
	SAFE_RELEASE(m_pDepthFrameReader);
	SAFE_RELEASE(m_pColorFrameReader);
	SAFE_RELEASE(m_pInfraredFrameReader);
	
	// close the Kinect Sensor
	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}

	SAFE_RELEASE(m_pKinectSensor);

	m_balserCam.Clear();
	
	m_config.SetValue("kinect_connect", m_isConnectKinect);
	m_config.SetValue("basler_connect", m_isConnectBasler);
	m_config.SetValue("inputfilepath", m_inputFilePath.c_str());
	m_config.Write("config_depthframe.txt");
}
