
#include "stdafx.h"
#include "root.h"


cRoot::cRoot()
	: m_pKinectSensor(NULL)
	, m_pDepthFrameReader(NULL)
	, m_pColorFrameReader(NULL)
	, m_pInfraredFrameReader(NULL)
	, m_isUpdate(false)
	//, m_pDepthBuff(NULL)
	, m_distribCount(0)
	, m_areaCount(0)
	//, m_areaMin(INT_MAX)
	//, m_areaMax(0)
	, m_areaFloorCnt(0)
	, m_input(eInputType::FILE)
{
	//m_pDepthBuff = new USHORT[g_kinectDepthWidth * g_kinectDepthHeight];

	//m_depthThresholdMin = 0;
	//m_depthThresholdMax = USHORT_MAX;
	m_depthThresholdMin = 440;
	m_depthThresholdMax = 945;
	m_depthDensity = 1.5f;
	m_heightErr[0] = 8;
	m_heightErr[1] = 3;

	ZeroMemory(m_hDistrib, sizeof(m_hDistrib));
	ZeroMemory(&m_hDistribDifferential, sizeof(m_hDistribDifferential));	
	//ZeroMemory(&m_areaGraph, sizeof(m_areaGraph));
}

cRoot::~cRoot()
{
	Clear();
}


bool cRoot::Create()
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
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


void cRoot::Update(graphic::cRenderer &renderer, const float deltaSeconds)
{
	UpdateDepthImage(renderer);
}


void cRoot::UpdateDepthImage(graphic::cRenderer &renderer)
{
	if (!m_isUpdate)
		return;
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
	//hr = pDepthFrame->CopyFrameDataToArray(nBufferSize, m_pDepthBuff);
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

	//SAFE_DELETEA(m_pDepthBuff);

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
}
