
#include "stdafx.h"
#include "root.h"
#include <ConsumerImplHelper/ToFCamera.h>
#include <iostream>
#include <iomanip> 
#include "3dview.h"
#include "depthframe.h"
#include "depthview.h"
#include "depthview2.h"


/* Allocator class used by the CToFCamera class for allocating memory buffers
used for grabbing. This custom allocator allocates buffers on the C++ heap.
If the application doesn't provide an allocator, a default one is used
that allocates memory on the heap. */
class CustomAllocator : public BufferAllocator
{
public:
	virtual void* AllocateBuffer(size_t size_by) { return new char[size_by]; }
	virtual void FreeBuffer(void* pBuffer) { delete[] static_cast<char*>(pBuffer); }
	virtual void Delete() { delete this; }
};



cRoot::cRoot()
	: m_pKinectSensor(NULL)
	, m_pDepthFrameReader(NULL)
	, m_pColorFrameReader(NULL)
	, m_pInfraredFrameReader(NULL)
	, m_isUpdate(false)
	, m_distribCount(0)
	, m_areaCount(0)
	, m_areaFloorCnt(0)
	, m_input(eInputType::FILE)
	, m_Camera(NULL)
	, m_baslerSetupSuccess(false)
	, m_isAutoSaveCapture(false)
	, m_isConnectBasler(true)
	, m_isAutoMeasure(false)
	, m_isPalete(false)
	, m_isConnectKinect(false)
{
	//m_depthThresholdMin = 0;
	//m_depthThresholdMax = USHORT_MAX;
	m_depthThresholdMin = 440;
	m_depthThresholdMax = 945;
	m_depthDensity = 1.5f;
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

	if (m_isConnectKinect)
	{
		KinectSetup();
	}

	if (m_isConnectBasler)
	{
		try
		{
			CToFCamera::InitProducer();
			m_Camera = new CToFCamera();
			if (EXIT_SUCCESS == BaslerCameraSetup())
			{
				m_baslerSetupSuccess = true;
			}
		}
		catch (GenICam::GenericException& e)
		{
			//cerr << "Exception occurred: " << endl << e.GetDescription() << endl;
			//exitCode = EXIT_FAILURE;
			return false;
		}
	}

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


int cRoot::BaslerCameraSetup()
{
	const size_t nBuffers = 3;  // Number of buffers to be used for grabbing.
	const size_t nImagesToGrab = 10; // Number of images to grab.
	size_t nImagesGrabbed = 0;

	try
	{
		//
		// Open and parameterize the camera.
		//

		setupCamera();

		//
		// Grab and process images
		//

		// Let the camera class use our allocator. 
		// When the application doesn't provide an allocator, a default one that allocates memory buffers
		// on the heap will be used automatically.
		m_Camera->SetBufferAllocator(new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

		// Allocate the memory buffers and prepare image acquisition.
		m_Camera->PrepareAcquisition(nBuffers);

		// Enqueue all buffers to be filled with image data.
		for (size_t i = 0; i < nBuffers; ++i)
		{
			m_Camera->QueueBuffer(i);
		}

		// Start the acquisition engine.
		m_Camera->StartAcquisition();

		// Now, the acquisition can be started on the camera.
		m_Camera->IssueAcquisitionStartCommand(); // The camera continuously sends data now.

	}
	catch (const GenICam::GenericException& e)
	{
		//cerr << "Exception occurred: " << e.GetDescription() << endl;
		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		if (m_Camera->IsOpen() && !m_Camera->IsConnected())
		{
			//cerr << "Camera has been removed." << endl;
		}
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void cRoot::setupCamera()
{
	m_Camera->OpenFirstCamera();
	//cout << "Connected to camera " << m_Camera->GetCameraInfo().strDisplayName << endl;

	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_Camera->GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_Camera->GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_Camera->GetParameter("PixelFormat");

	// Enable range data
	ptrComponentSelector->FromString("Range");
	ptrComponentEnable->SetValue(true);
	// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
	// Note: To change the format of an image component, the Component Selector must first be set to the component
	// you want to configure (see above).
	// To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
	ptrPixelFormat->FromString("Coord3D_ABC32f");

	ptrComponentSelector->FromString("Intensity");
	ptrComponentEnable->SetValue(true);

	ptrComponentSelector->FromString("Confidence");
	ptrComponentEnable->SetValue(true);

}


bool cRoot::BaslerCapture()
{
	RETV(!m_baslerSetupSuccess, false);

	try
	{
		GrabResult grabResult;
		// Wait up to 1000 ms for the next grabbed buffer available in the 
		// acquisition engine's output queue.
		m_Camera->GetGrabResult(grabResult, 100);

		// Check whether a buffer has been grabbed successfully.
		if (grabResult.status == GrabResult::Timeout)
		{
			//cerr << "Timeout occurred." << endl;
			// The timeout might be caused by a removal of the camera. Check if the camera
			// is still connected.
			if (!m_Camera->IsConnected())
			{
				//cerr << "Camera has been removed." << endl;
			}
			//break; // exit loop
			return false;
		}
		if (grabResult.status != GrabResult::Ok)
		{
			//cerr << "Failed to grab image." << endl;
			//break; // exit loop
			return false;
		}

		//nImagesGrabbed++;
		// We can process the buffer now. The buffer will not be overwritten with new data until
		// it is explicitly placed in the acquisition engine's input queue again.
		processData(grabResult);

		// We finished processing the data, put the buffer back into the acquisition 
		// engine's input queue to be filled with new image data.
		m_Camera->QueueBuffer(grabResult.hBuffer);
	}
	catch (const GenICam::GenericException& e)
	{
		//cerr << "Exception occurred: " << e.GetDescription() << endl;
		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		if (m_Camera->IsOpen() && !m_Camera->IsConnected())
		{
			//cerr << "Camera has been removed." << endl;
		}
		return false;
	}

	return true;
}


void cRoot::processData(const GrabResult& grabResult)
{
	BufferParts parts;
	m_Camera->GetBufferParts(grabResult, parts);

	// Retrieve the values for the center pixel
	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
	uint16_t *pIntensity = (uint16_t*)parts[1].pData;
	uint16_t *pConfidence = (uint16_t*)parts[2].pData;
	const size_t nPixel = parts[0].width * parts[0].height;

	cDatReader reader;
	reader.m_vertices.resize(640 * 480);
	reader.m_intensity.resize(640 * 480);
	reader.m_confidence.resize(640 * 480);

	memcpy(&reader.m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);
	memcpy(&reader.m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
	memcpy(&reader.m_confidence[0], pConfidence, sizeof(unsigned short) * 640 * 480);

	m_sensorBuff.ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer(), reader);
	
	// save *.pcd file
	if (m_isAutoSaveCapture)
	{
		using namespace std;
		string fileName = string("../media/depth/") + common::GetCurrentDateTime() + ".pcd";
		ofstream o(fileName, ios::binary);
		if (o)
		{
			o.write((char*)p3DCoordinate, sizeof(float)*nPixel * 3);
			o.write((char*)pIntensity, sizeof(uint16_t)*nPixel);
			o.write((char*)pConfidence, sizeof(uint16_t)*nPixel);
		}
	}
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


	if (m_baslerSetupSuccess && m_Camera)
	{
		// Stop the camera
		m_Camera->IssueAcquisitionStopCommand();
		// Stop the acquisition engine and release memory buffers and other resources used for grabbing.
		m_Camera->FinishAcquisition();
		// Close the connection to the camera
		m_Camera->Close();
	}

	if (m_Camera)
	{
		SAFE_DELETE(m_Camera);

		if (CToFCamera::IsProducerInitialized())
			CToFCamera::TerminateProducer();  // Won't throw any exceptions
	}

	m_config.SetValue("kinect_connect", m_isConnectKinect);
	m_config.SetValue("basler_connect", m_isConnectBasler);
	m_config.SetValue("inputfilepath", m_inputFilePath.c_str());
	m_config.Write("config_depthframe.txt");
}
