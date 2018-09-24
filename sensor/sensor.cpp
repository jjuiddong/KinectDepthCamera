
#include "stdafx.h"
#include "sensor.h"

using namespace graphic;
using namespace GenApi;



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


cSensor::cSensor()
	: m_camera(NULL)
	, m_isEnable(false)
	, m_isMaster(false)
	, m_isShow(true)
	, m_id(0)
	, m_writeTime(0)
	//, m_outlierTolerance(6000)
	, m_outlierTolerance(OUTLIER_TOLERANCE)
	//, m_confidenceThreshold(560)
	, m_confidenceThreshold(CONFIDENCE_THRESHOLD)
	, m_totalGrabCount(0)
	, m_grabSeconds(0)
	, m_grabFPS(0)
	, m_grabTime(0)
{
}

cSensor::~cSensor()
{
	Clear();
}


bool cSensor::InitCamera(const int id, const CameraInfo &cinfo)
{
	Clear();

	common::dbg::Logp("Configuring Camera %d : %s\n", id, cinfo.strDisplayName.c_str());

	m_id = id;
	m_isEnable = false;
	m_info = cinfo;
	m_camera = new CToFCamera();

	// Open camera with camera info.
	m_camera->Open(cinfo);

	//GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
	//int devChannel[4] = {1, 2, 3};
	//ptrDeviceChannel->SetValue(devChannel[id]);
	
	GenApi::CIntegerPtr ptrOutlierTolerance = m_camera->GetParameter("OutlierTolerance");
	ptrOutlierTolerance->SetValue(m_outlierTolerance);

	if ((cinfo.strDisplayName == "Camera1") || (cinfo.strDisplayName == "Camera2"))
	{
		//m_confidenceThreshold = 1200;
	}

	if ((cinfo.strDisplayName == "Camera1"))
	{
		GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
		ptrDeviceChannel->SetValue(0);
	}

	if ((cinfo.strDisplayName == "Camera2"))
	{
		GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
		ptrDeviceChannel->SetValue(1);
	}
	
	if ((cinfo.strDisplayName == "Camera3"))
	{
		GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
		ptrDeviceChannel->SetValue(3);
	}

	if ((cinfo.strDisplayName == "Camera4"))
	{
		GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
		ptrDeviceChannel->SetValue(1);
	}

	if ((cinfo.strDisplayName == "Camera5"))
	{
		GenApi::CIntegerPtr ptrDeviceChannel = m_camera->GetParameter("DeviceChannel");
		ptrDeviceChannel->SetValue(3);
	}

	GenApi::CIntegerPtr ptrConfidenceThreshold = m_camera->GetParameter("ConfidenceThreshold");
	ptrConfidenceThreshold->SetValue(m_confidenceThreshold);

	GenApi::CIntegerPtr ptrWidth = m_camera->GetParameter("Width");
	ptrWidth->SetValue(640);
	GenApi::CIntegerPtr ptrHeight = m_camera->GetParameter("Height");
	ptrHeight->SetValue(480);
	//GenApi::CFloatPtr ptrFrameRate = m_camera->GetParameter("AcquisitionFrameRate");
	//ptrFrameRate->SetValue(2.f);

	//GenApi::CIntegerPtr ptrInterPacketDelay = m_camera->GetParameter("GevSCPD");
	//ptrInterPacketDelay->SetValue(454352); // 500ms

	//GenApi::CIntegerPtr ptrFrameTransmissionDelay = m_camera->GetParameter("GevSCFTD");
	//ptrFrameTransmissionDelay->SetValue(m_id*8); //ns

	GenApi::CIntegerPtr ptrMaxDepth = m_camera->GetParameter("DepthMax");
	ptrMaxDepth->SetValue(6000);

	m_oldOutlierTolerance = m_outlierTolerance;
	m_oldConfidenceThreshold = m_confidenceThreshold;

	//
	// Configure camera for synchronous free run.
	// Do not yet configure trigger delays.
	//

	// Enable IEEE1588.
	CBooleanPtr ptrIEEE1588Enable = m_camera->GetParameter("GevIEEE1588");
	ptrIEEE1588Enable->SetValue(true);

	// Enable trigger.
	GenApi::CEnumerationPtr ptrTriggerMode = m_camera->GetParameter("TriggerMode");

	// Set trigger mode to "on".
	ptrTriggerMode->FromString("On");

	// Configure the sync timer as trigger source.
	GenApi::CEnumerationPtr ptrTriggerSource = m_camera->GetParameter("TriggerSource");
	ptrTriggerSource->FromString("SyncTimer");

	// Enable 3D (point cloud) data, intensity data, and confidence data 
	GenApi::CEnumerationPtr ptrComponentSelector = m_camera->GetParameter("ComponentSelector");
	GenApi::CBooleanPtr ptrComponentEnable = m_camera->GetParameter("ComponentEnable");
	GenApi::CEnumerationPtr ptrPixelFormat = m_camera->GetParameter("PixelFormat");

	// Enable range data
	ptrComponentSelector->FromString("Range");
	ptrComponentEnable->SetValue(true);
	// Range information can be sent either as a 16-bit grey value image or as 3D coordinates (point cloud). For this sample, we want to acquire 3D coordinates.
	// Note: To change the format of an image component, the Component Selector must first be set to the component
	// you want to configure (see above).
	// To use 16-bit integer depth information, choose "Mono16" instead of "Coord3D_ABC32f".
	ptrPixelFormat->FromString("Coord3D_ABC32f");

	ptrComponentSelector->FromString("Intensity");
	ptrComponentEnable->SetValue(false);

	ptrComponentSelector->FromString("Confidence");
	ptrComponentEnable->SetValue(false);

	m_isEnable = true; // success
	return true;
}


// Check & Update Parameters
void cSensor::CheckAndUpdateParameters()
{
	if (m_oldOutlierTolerance != m_outlierTolerance)
	{
		try {
			GenApi::CIntegerPtr ptrVal = m_camera->GetParameter("OutlierTolerance");
			ptrVal->SetValue(m_outlierTolerance);
		}
		catch (...)
		{
			// nothing~
			common::dbg::Logp("Error update camera configuration\n");
		}

		m_oldOutlierTolerance = m_outlierTolerance;
	}

	if (m_oldConfidenceThreshold != m_confidenceThreshold)
	{
		try {
			GenApi::CIntegerPtr ptrVal = m_camera->GetParameter("ConfidenceThreshold");
			ptrVal->SetValue(m_confidenceThreshold);
		}
		catch (...)
		{
			// nothing~
			common::dbg::Logp("Error update camera configuration\n");
		}

		m_oldConfidenceThreshold = m_confidenceThreshold;
	}
}


// return type
// 1 : success
// 2 : not ready
// 3 : time out
// 4 : grabbed fail
int cSensor::Grab()
{
	RETV(!m_camera, 2);
	RETV(!m_isEnable, 2);
	RETV(!m_isShow, 2);

	int retVal = 1;

	try
	{
		const double curT = g_root.m_timer.GetMilliSeconds();

		CheckAndUpdateParameters();

		GrabResult grabResult;
		m_camera->GetGrabResult(grabResult, 100);

		if (grabResult.status == GrabResult::Timeout)
		{
			if (g_root.m_isGrabLog)
				common::dbg::ErrLogp("Err Timeout occurred. camIdx = %d\n", m_id);
			retVal = 3;
		}
		else if (grabResult.status != GrabResult::Ok)
		{
			if (g_root.m_isGrabLog)
				common::dbg::ErrLog("Err Got a buffer, but it hasn't been successfully grabbed. camIdx = %d\n", m_id);
			retVal = 4;
		}
		else if (grabResult.status == GrabResult::Ok)
		{
			BufferParts parts;
			m_camera->GetBufferParts(grabResult, parts);

			// Retrieve the values for the center pixel
			{
				common::AutoCSLock cs(m_cs);

				CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
				const size_t nPixel = parts[0].width * parts[0].height;
				memcpy(&m_tempBuffer.m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);

				if (parts.size() >= 2)
				{
					uint16_t *pIntensity = (uint16_t*)parts[1].pData;
					memcpy(&m_tempBuffer.m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
				}

				m_tempBuffer.m_time = curT; // Update Time

				m_totalGrabCount++;
				m_grabSeconds++;
			}
		}

		if (grabResult.status == GrabResult::Ok)
		{
			m_camera->QueueBuffer(grabResult.hBuffer);
		}

		if (curT - m_grabTime > 1000.f)
		{
			m_grabFPS = (float)m_grabSeconds;
			m_grabSeconds = 0;
			m_grabTime = curT;
		}
	}
	catch (...)
	{
		common::dbg::Logp("Error Grab \n");
		return 4;
	}

	return retVal;
}


bool cSensor::CopyCaptureBuffer(cRenderer &renderer, const char *saveFileName)
{
	common::AutoCSLock cs(m_cs);

	if (m_buffer.m_time < m_tempBuffer.m_time)
		m_buffer.ReadDatFile(renderer, m_tempBuffer);

	// save PCD file
	if (g_root.m_isAutoSaveCapture)
	{
		if (m_isShow)
		{
			const double curT = g_root.m_timer.GetMilliSeconds();
			if ((curT - m_writeTime) > 500.f) // 0.5 초에 하나씩 저장한다.
			{
				m_writeTime = curT;
				// save only processing camera depthmap
				m_tempBuffer.Write(saveFileName);
				//m_tempBuffer.WriteAscii(saveFileName);
			}
		}
	}

	return true;
}


void cSensor::PrepareAcquisition()
{
	RET(!m_camera);

	const size_t nBuffers = 3;  // Number of buffers to be used for grabbing.

	// Let the camera class use our allocator. 
	// When the application doesn't provide an allocator, a default one that allocates memory buffers
	// on the heap will be used automatically.
	m_camera->SetBufferAllocator(new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

	// Allocate the memory buffers and prepare image exposure.
	m_camera->PrepareAcquisition(nBuffers);

	// Enqueue all buffers to be filled with image data.
	for (size_t j = 0; j < nBuffers; ++j)
		m_camera->QueueBuffer(j);
}


void cSensor::BeginAcquisition()
{
	RET(!m_camera);

	// Start the acquisition engine.
	m_camera->StartAcquisition();

	// Now, the acquisition can be started on the camera.
	m_camera->IssueAcquisitionStartCommand(); // The camera continuously sends data now.
}


void cSensor::EndAcquisition()
{
	RET(!m_camera);

	// Stop the acquisition engine.
	m_camera->StopAcquisition();

	m_camera->IssueAcquisitionStopCommand();
}


bool cSensor::IsEnable()
{
	//RETV(!m_camera, false);
	return m_isEnable;
}


void cSensor::Clear()
{
	if (m_camera)
	{
		m_camera->IssueAcquisitionStopCommand();
		m_camera->FinishAcquisition();
		m_camera->Close();
	}

	SAFE_DELETE(m_camera);
}
