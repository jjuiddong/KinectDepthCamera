
#include "stdafx.h"
#include "kinectv2.h"


cKinect::cKinect()
	: m_pKinectSensor(NULL)
	, m_pDepthFrameReader(NULL)
	, m_pColorFrameReader(NULL)
	, m_pInfraredFrameReader(NULL)
	, m_isSetupSuccess(false)
{
}

cKinect::~cKinect()
{
	Clear();
}


bool cKinect::Init()
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


void cKinect::Capture(graphic::cRenderer &renderer)
{
	if (!m_pDepthFrameReader)
		return;

	IDepthFrame* pDepthFrame = NULL;
	HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
	if (FAILED(hr))
		return;

	IFrameDescription* pFrameDescription = NULL;
	int nWidth = 0;
	int nHeight = 0;
	UINT nBufferSize = g_kinectDepthHeight * g_kinectDepthWidth;
	UINT16 *pBuffer = NULL;
	INT64 nTime;
	USHORT nDepthMinReliableDistance;
	USHORT nDepthMaxDistance = USHRT_MAX;

	hr = pDepthFrame->get_RelativeTime(&nTime);
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

	hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
	if (FAILED(hr))
		goto error;

	// In order to see the full range of depth (including the less reliable far field depth)
	// we are setting nDepthMaxDistance to the extreme potential depth threshold

	// Note:  If you wish to filter by reliable depth distance, uncomment the following line.
	hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);

	hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	if (FAILED(hr))
		goto error;

	//m_sensorBuff.ReadKinectSensor(renderer, nTime, pBuffer, nDepthMinReliableDistance, nDepthMaxDistance);

error:
	SAFE_RELEASE(pFrameDescription);
	SAFE_RELEASE(pDepthFrame);
}


void cKinect::Clear()
{
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
