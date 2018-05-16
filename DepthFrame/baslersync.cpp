
#include "stdafx.h"
#include "baslersync.h"
#include "3dview.h"
#include "depthframe.h"
#include "StopWatch.h"

using namespace GenApi;

void BaslerCameraThread(cBaslerCameraSync *basler);

void mSleep(int sleepMs)      // Use a wrapper  
{
	Sleep(sleepMs);           // Sleep expects time in ms
}


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



cBaslerCameraSync::cBaslerCameraSync(const bool isThreadMode //= false
)
	: m_isSetupSuccess(false)
	, m_NumCams(0)
	, m_isThreadMode(isThreadMode)
	, m_state(eThreadState::NONE)
	, m_isTrySyncTrigger(false)
	, m_isGrabLog(true)
{
}

cBaslerCameraSync::~cBaslerCameraSync()
{
	Clear();
}


bool cBaslerCameraSync::Init()
{
	m_isSetupSuccess = false;
	m_timer.Create();

	if (m_isThreadMode)
	{
		m_state = eThreadState::CONNECT_TRY;
		m_thread = std::thread(BaslerCameraThread, this);
	}
	else
	{
		m_state = eThreadState::NONE;

		try
		{
			CToFCamera::InitProducer();

			if (EXIT_SUCCESS != BaslerCameraSetup())
				return false;
		}
		catch (GenICam::GenericException& e)
		{
			common::Str128 msg = "Exception occurred: ";
			msg += e.GetDescription();
			::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);

			return false;
		}
	}

	return true;
}


int cBaslerCameraSync::BaslerCameraSetup()
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

		findMaster();

		syncCameras();

		setTriggerDelays();

		// Prepare cameras and buffers for image exposure.
		for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
		{
			// Let the camera class use our allocator. 
			// When the application doesn't provide an allocator, a default one that allocates memory buffers
			// on the heap will be used automatically.
			m_Cameras.at(camIdx)->SetBufferAllocator(new CustomAllocator(), true); // m_Camera takes ownership and will clean-up allocator.

			// Allocate the memory buffers and prepare image exposure.
			m_Cameras.at(camIdx)->PrepareAcquisition(nBuffers);

			// Enqueue all buffers to be filled with image data.
			for (size_t j = 0; j < nBuffers; ++j)
			{
				m_Cameras.at(camIdx)->QueueBuffer(j);
			}

			// Start the acquisition engine.
			m_Cameras.at(camIdx)->StartAcquisition();

			// Now, the acquisition can be started on the camera.
			m_Cameras.at(camIdx)->IssueAcquisitionStartCommand(); // The camera continuously sends data now.
		}

	}
	catch (const GenICam::GenericException& e)
	{
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();

		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		for (size_t camIdx = 0; camIdx < m_Cameras.size(); camIdx++)
		{
			if (m_Cameras.at(camIdx)->IsOpen() && !m_Cameras.at(camIdx)->IsConnected())
			{
				msg += "Camera has been removed.";
			}
		}

		::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



void cBaslerCameraSync::setupCamera()
{
	common::dbg::Logp("Searching for cameras ... \n");
	m_CameraList = CToFCamera::EnumerateCameras();
	common::dbg::Logp("found %d ToF cameras \n", m_CameraList.size());

	// Store number of cameras.
	m_NumCams = (int)m_CameraList.size();
	size_t camIdx = 0;

	// Initialize array with master/slave info.
	for (size_t i = 0; i < MAX_CAMS; i++)
	{
		m_IsMaster[i] = false;
	}

	CameraList::const_iterator iterator;

	// Iterate over list of cameras.
	for (iterator = m_CameraList.begin(); iterator != m_CameraList.end(); ++iterator)
	{
		CameraInfo cInfo = *iterator;
		common::dbg::Logp("Configuring Camera %d : %s\n", camIdx, cInfo.strDisplayName.c_str());

		// Create shared pointer to ToF camera.
		std::shared_ptr<CToFCamera> cam(new CToFCamera());

		// Store shared pointer for later use.
		m_Cameras.push_back(cam);

		// Open camera with camera info.
		cam->Open(cInfo);

		//
		// Configure camera for synchronous free run.
		// Do not yet configure trigger delays.
		//

		// Enable IEEE1588.
		CBooleanPtr ptrIEEE1588Enable = cam->GetParameter("GevIEEE1588");
		ptrIEEE1588Enable->SetValue(true);

		// Enable trigger.
		GenApi::CEnumerationPtr ptrTriggerMode = cam->GetParameter("TriggerMode");

		// Set trigger mode to "on".
		ptrTriggerMode->FromString("On");

		// Configure the sync timer as trigger source.
		GenApi::CEnumerationPtr ptrTriggerSource = cam->GetParameter("TriggerSource");
		ptrTriggerSource->FromString("SyncTimer");

		{
			// Enable 3D (point cloud) data, intensity data, and confidence data 
			GenApi::CEnumerationPtr ptrComponentSelector = cam->GetParameter("ComponentSelector");
			GenApi::CBooleanPtr ptrComponentEnable = cam->GetParameter("ComponentEnable");
			GenApi::CEnumerationPtr ptrPixelFormat = cam->GetParameter("PixelFormat");

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

		// Proceed to next camera.
		camIdx++;
	}

	m_isSetupSuccess = true;
}


void cBaslerCameraSync::findMaster()
{
	// Number of masters found ( != 1 ) 
	unsigned int nMaster;

	common::dbg::Logp("waiting for cameras to negotiate master role ...\n");

	do
	{

		nMaster = 0;
		//
		// Wait until a master camera (if any) and the slave cameras have been chosen.
		// Note that if a PTP master clock is present in the subnet, all TOF cameras
		// ultimately assume the slave role.
		//
		for (size_t i = 0; i < m_NumCams; i++)
		{
			// Latch IEEE1588 status.
			CCommandPtr ptrGevIEEE1588DataSetLatch = m_Cameras.at(i)->GetParameter("GevIEEE1588DataSetLatch");
			ptrGevIEEE1588DataSetLatch->Execute();

			// Read back latched status.
			GenApi::CEnumerationPtr ptrGevIEEE1588StatusLatched = m_Cameras.at(i)->GetParameter("GevIEEE1588StatusLatched");
			// The smart pointer holds the node, not the value.
			// The node value is always up to date. 
			// Therefore, there is no need to repeatedly retrieve the pointer here.
			while (ptrGevIEEE1588StatusLatched->ToString() == "Listening")
			{
				// Latch GevIEEE1588 status.
				ptrGevIEEE1588DataSetLatch->Execute();
				//cout << "." << std::flush;
				mSleep(1000);
			}

			//
			// Store the information whether the camera is master or slave.
			//
			if (ptrGevIEEE1588StatusLatched->ToString() == "Master")
			{
				m_IsMaster[i] = true;
				nMaster++;
			}
			else
			{
				m_IsMaster[i] = false;
			}
		}
	} while ((nMaster > 1)
		&& (!m_isThreadMode || (m_isThreadMode && (eThreadState::DISCONNECT_TRY != m_state)))
		);    // Repeat until there is at most one master left.

	// Use this variable to check whether there is an external master clock.
	bool externalMasterClock = true;

	for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
	{
		if (true == m_IsMaster[camIdx])
		{
			common::dbg::Logp("   camera %d is master\n", camIdx);
			externalMasterClock = false;
		}
	}

	if (true == externalMasterClock)
	{
		common::dbg::Logp("External master clock present in subnet: All cameras are slaves.\n");
	}
}


void cBaslerCameraSync::syncCameras() 
{
	// Maximum allowed offset from master clock. 
	const uint64_t tsOffsetMax = 10000; // 10 ms

	common::dbg::Logp("Wait until offsets from master clock have settled below %d ns\n", tsOffsetMax);

	for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
	{
		// Check all slaves for deviations from master clock.
		if (false == m_IsMaster[camIdx])
		{
			uint64_t tsOffset;
			do
			{
				tsOffset = GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(camIdx, 1.0, 0.1);
				common::dbg::Logp("max offset of cam %d = %d ns\n", camIdx, tsOffsetMax);

			} while ((tsOffset >= tsOffsetMax) 
				&& (!m_isThreadMode || (m_isThreadMode && (eThreadState::DISCONNECT_TRY != m_state))) 
				);
		}
	}
}


int64_t cBaslerCameraSync::GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(size_t camIdx, double timeToMeasureSec, double timeDeltaSec)
{
	CCommandPtr ptrGevIEEE1588DataSetLatch = m_Cameras.at(camIdx)->GetParameter("GevIEEE1588DataSetLatch");
	ptrGevIEEE1588DataSetLatch->Execute();
	CIntegerPtr ptrGevIEEE1588OffsetFromMaster = m_Cameras.at(camIdx)->GetParameter("GevIEEE1588OffsetFromMaster");
	StopWatch m_StopWatch;
	m_StopWatch.reset();
	// Maximum of offsets from master
	int64_t maxOffset = 0;
	// Number of samples
	uint32_t n(0);
	// Current time
	double currTime(0);
	do
	{
		// Update current time.
		currTime = m_StopWatch.get(false);
		if (currTime >= n * timeDeltaSec)
		{
			// Time for next sample has elapsed.
			// Latch IEEE1588 data set to get offset from master.
			ptrGevIEEE1588DataSetLatch->Execute();
			// Maximum of offsets from master.
			maxOffset = std::max(maxOffset, std::abs(ptrGevIEEE1588OffsetFromMaster->GetValue()));
			// Increase number of samples.
			n++;
		}
		mSleep(1);
	} while (currTime <= timeToMeasureSec);
	// Return maximum of offsets from master for given time interval.
	return maxOffset;
}


void cBaslerCameraSync::setTriggerDelays() 
{
	RET(!m_isSetupSuccess);

	// Current timestamp
	uint64_t timestamp, syncStartTimestamp;

	// The low and high part of the timestamp
	uint64_t tsLow, tsHigh;

	// Initialize trigger delay.
	m_TriggerDelay = 0;

	common::dbg::Logp("configuring start time and trigger delays ...\n");

	//
	// Cycle through cameras and set trigger delay.
	//
	for (size_t camIdx = 0; camIdx < m_Cameras.size(); camIdx++)
	{
		common::dbg::Logp("Camera %d : \n", camIdx);
		//
		// Read timestamp and exposure time.
		// Calculation of synchronous free run timestamps will all be based 
		// on timestamp and exposure time(s) of first camera.
		//
		if (camIdx == 0)
		{
			// Latch timestamp registers.
			CCommandPtr ptrTimestampLatch = m_Cameras.at(camIdx)->GetParameter("TimestampLatch");
			ptrTimestampLatch->Execute();

			// Read the two 32-bit halves of the 64-bit timestamp. 
			CIntegerPtr ptrTimeStampLow(m_Cameras.at(camIdx)->GetParameter("TimestampLow"));
			CIntegerPtr ptrTimeStampHigh(m_Cameras.at(camIdx)->GetParameter("TimestampHigh"));
			tsLow = ptrTimeStampLow->GetValue();
			tsHigh = ptrTimeStampHigh->GetValue();

			// Assemble 64-bit timestamp and keep it.
			timestamp = tsLow + (tsHigh << 32);
			common::dbg::Logp("Reading time stamp from first camera. timestamp = %I64d\n", timestamp);

			//cout << "Reading exposure times from first camera:" << endl;

			// Get exposure time count (in case of HDR there will be 2, otherwise 1).
			CIntegerPtr ptrExposureTimeSelector = m_Cameras.at(camIdx)->GetParameter("ExposureTimeSelector");
			size_t n_expTimes = 1 + (size_t)ptrExposureTimeSelector->GetMax();

			// Sum up exposure times.
			CFloatPtr ptrExposureTime = m_Cameras.at(camIdx)->GetParameter("ExposureTime");
			for (size_t l = 0; l < n_expTimes; l++)
			{
				ptrExposureTimeSelector->SetValue(l);
				common::dbg::Logp("exposure time %d = %f\n", l, ptrExposureTime->GetValue());

				m_TriggerDelay += (int64_t)(1000 * ptrExposureTime->GetValue());   // Convert from us -> ns
			}

			common::dbg::Logp("Calculating trigger delay.\n");

			// Add readout time.
			m_TriggerDelay += (n_expTimes - 1) * m_ReadoutTime;

			// Add safety margin for clock jitter.
			m_TriggerDelay += 1000000;

			// Calculate synchronous trigger rate.
			common::dbg::Logp("Calculating maximum synchronous trigger rate ... \n");
			m_SyncTriggerRate = 1000000000 / (m_NumCams * m_TriggerDelay);

			// If the calculated value is greater than the maximum supported rate, 
			// adjust it. 
			CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate"));
			if (m_SyncTriggerRate > ptrSyncRate->GetMax())
			{
				m_SyncTriggerRate = (uint64_t)ptrSyncRate->GetMax();
			}

			// Print trigger delay and synchronous trigger rate.
			common::dbg::Logp("Trigger delay = %I64d ms\n", m_TriggerDelay / 1000000 );
			common::dbg::Logp("Setting synchronous trigger rate to %I64d fps\n", m_SyncTriggerRate);
		}

		// Set synchronization rate.
		CFloatPtr ptrSyncRate(m_Cameras.at(camIdx)->GetParameter("SyncRate"));
		ptrSyncRate->SetValue((double)m_SyncTriggerRate);

		// Calculate new timestamp by adding trigger delay.
		// First camera starts after triggerBaseDelay, nth camera is triggered 
		// after a delay of triggerBaseDelay +  n * triggerDelay.
		syncStartTimestamp = timestamp + m_TriggerBaseDelay + camIdx * m_TriggerDelay;

		// Disassemble 64-bit timestamp.
		tsHigh = syncStartTimestamp >> 32;
		tsLow = syncStartTimestamp - (tsHigh << 32);

		// Get pointers to the two 32-bit registers, which together hold the 64-bit 
		// synchronous free run start time.
		CIntegerPtr ptrSyncStartLow(m_Cameras.at(camIdx)->GetParameter("SyncStartLow"));
		CIntegerPtr ptrSyncStartHigh(m_Cameras.at(camIdx)->GetParameter("SyncStartHigh"));

		ptrSyncStartLow->SetValue(tsLow);
		ptrSyncStartHigh->SetValue(tsHigh);

		// Latch synchronization start time & synchronization rate registers.
		// Until the values have been latched, they won't have any effect.
		CCommandPtr ptrSyncUpdate = m_Cameras.at(camIdx)->GetParameter("SyncUpdate");
		ptrSyncUpdate->Execute();

		// Show new synchronous free run start time.
		//cout <<  << endl;
		common::dbg::Logp("Setting Sync Start time stamp\n");
		common::dbg::Logp("SyncStartLow = %I64d\n", ptrSyncStartLow->GetValue());
		common::dbg::Logp("SyncStartHigh = %I64d\n", ptrSyncStartHigh->GetValue());
	}
}


// Grab
bool cBaslerCameraSync::Grab()
{
	RETV(!m_isSetupSuccess, false);
	static int grabcnt = 0;

	try
	{
		//for (size_t camIdx = 0; camIdx < m_NumCams; camIdx++)
		for (size_t camIdx = 0; camIdx < 1; camIdx++)
		{
			GrabResult grabResult;
			m_Cameras.at(camIdx)->GetGrabResult(grabResult, 100);

			if (grabResult.status == GrabResult::Timeout)
			{
				if (m_isGrabLog)
					common::dbg::ErrLogp("Err Timeout occurred. camIdx = %d\n", camIdx);

				if (!m_Cameras.at(camIdx)->IsConnected())
				{
					if (m_isGrabLog)
						common::dbg::ErrLogp("Err Timeout because Connection Faile. camIdx = %d\n", camIdx);
				}
				continue;
			}

			if (grabResult.status != GrabResult::Ok)
			{
				if (m_isGrabLog)
					common::dbg::ErrLog("Err Got a buffer, but it hasn't been successfully grabbed. camIdx = %d\n", camIdx);
				continue;
			}

			if (grabResult.status == GrabResult::Ok)
			{
				if (m_isGrabLog)
					common::dbg::Logp("camIdx = %d, grab = %d\n", camIdx, grabcnt++);

				common::AutoCSLock cs(m_cs);

				BufferParts parts;
				m_Cameras.at(camIdx)->GetBufferParts(grabResult, parts);

				// Retrieve the values for the center pixel
				CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
				uint16_t *pIntensity = (uint16_t*)parts[1].pData;
				const size_t nPixel = parts[0].width * parts[0].height;
				memcpy(&m_captureBuff[camIdx].m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);
				memcpy(&m_captureBuff[camIdx].m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
				m_captureBuff[camIdx].m_time = m_timer.GetMilliSeconds(); // Update Time
			}

			if (grabResult.status != GrabResult::Timeout)
			{
				m_Cameras.at(camIdx)->QueueBuffer(grabResult.hBuffer);
			}
		}
	}
	catch (const GenICam::GenericException& )
	{
		return false;
	}

	return true;
}


bool cBaslerCameraSync::CopyCaptureBuffer(graphic::cRenderer &renderer)
{
	common::AutoCSLock cs(m_cs);

	const string curTime = common::GetCurrentDateTime();

	if (g_root.m_sensorBuff[0].m_time < m_captureBuff[0].m_time)
		g_root.m_sensorBuff[0].ReadDatFile(renderer, m_captureBuff[0]);
	if (g_root.m_sensorBuff[1].m_time < m_captureBuff[1].m_time)
		g_root.m_sensorBuff[1].ReadDatFile(renderer, m_captureBuff[1]);

	// save PCD file
	for (int i = 0; i < 2; ++i)
	{
		if (g_root.m_isAutoSaveCapture)
		{
			common::StrPath fileName;
			fileName.Format("../media/depthMulti/%d/%s.pcd", i, curTime.c_str());
			m_captureBuff[i].Write(fileName.c_str());
		}
	}

	return true;
}


void cBaslerCameraSync::processData(const size_t camIdx, const GrabResult& grabResult)
{
	BufferParts parts;
	m_Cameras.at(camIdx)->GetBufferParts(grabResult, parts);

	// Retrieve the values for the center pixel
	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
	uint16_t *pIntensity = (uint16_t*)parts[1].pData;
	uint16_t *pConfidence = (uint16_t*)parts[2].pData;
	const size_t nPixel = parts[0].width * parts[0].height;

	cDatReader reader;
	memcpy(&reader.m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);
	memcpy(&reader.m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
	memcpy(&reader.m_confidence[0], pConfidence, sizeof(unsigned short) * 640 * 480);

	g_root.m_sensorBuff[camIdx].ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer(), reader);

	// save *.pcd file
	if (g_root.m_isAutoSaveCapture)
	{
		using namespace std;
		common::StrPath fileName;
		fileName.Format("../media/depthMulti/%d/%s.pcd", camIdx, common::GetCurrentDateTime().c_str());

		ofstream o(fileName.c_str(), ios::binary);
		if (o)
		{
			o.write((char*)p3DCoordinate, sizeof(float)*nPixel * 3);
			o.write((char*)pIntensity, sizeof(uint16_t)*nPixel);
			o.write((char*)pConfidence, sizeof(uint16_t)*nPixel);
		}
	}
}


void cBaslerCameraSync::Clear()
{
	if (m_isThreadMode)
	{
		m_state = eThreadState::DISCONNECT_TRY;

		if (m_thread.joinable())
			m_thread.join();
	}
	else
	{
		if (m_isSetupSuccess && !m_Cameras.empty())
		{
			for (size_t camIdx = 0; camIdx < m_Cameras.size(); camIdx++)
			{
				// Stop the camera.
				m_Cameras.at(camIdx)->IssueAcquisitionStopCommand();

				// Stop the acquisition engine and release memory buffers and other resources used for grabbing.
				m_Cameras.at(camIdx)->FinishAcquisition();

				// Close connection to camera.
				m_Cameras.at(camIdx)->Close();
			}
		}

		if (!m_Cameras.empty())
		{
			m_Cameras.clear();
			m_NumCams = 0;

			if (CToFCamera::IsProducerInitialized())
				CToFCamera::TerminateProducer();  // Won't throw any exceptions
		}
	}
}


// 쓰레드 처리
void BaslerCameraThread(cBaslerCameraSync *basler)
{
	try
	{
		CToFCamera::InitProducer();

		if (EXIT_SUCCESS != basler->BaslerCameraSetup())
		{
			basler->m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
			return;
		}

		double triggerDelayTime = basler->m_timer.GetMilliSeconds();

		if (cBaslerCameraSync::eThreadState::CONNECT_TRY == basler->m_state)
			basler->m_state = cBaslerCameraSync::eThreadState::CAPTURE;

		while (cBaslerCameraSync::eThreadState::CAPTURE == basler->m_state)
		{
			if (basler->m_timer.GetMilliSeconds() - triggerDelayTime > 1000)
			{
				//basler->setTriggerDelays();
				triggerDelayTime = basler->m_timer.GetMilliSeconds();
			}

			if (basler->m_isTrySyncTrigger)
			{
				basler->setTriggerDelays();
				basler->m_isTrySyncTrigger = false;
			}

			basler->Grab();
		}

		basler->m_state = cBaslerCameraSync::eThreadState::DISCONNECT;
	}
	catch (GenICam::GenericException& e)
	{
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();
		::MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);

		basler->m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
	}

	for (size_t camIdx = 0; camIdx < basler->m_Cameras.size(); camIdx++)
	{
		// Stop the camera.
		basler->m_Cameras.at(camIdx)->IssueAcquisitionStopCommand();
		// Stop the acquisition engine and release memory buffers and other resources used for grabbing.
		basler->m_Cameras.at(camIdx)->FinishAcquisition();
		// Close connection to camera.
		basler->m_Cameras.at(camIdx)->Close();
	}
	basler->m_Cameras.clear();
	basler->m_NumCams = 0;

	if (CToFCamera::IsProducerInitialized())
		CToFCamera::TerminateProducer();  // Won't throw any exceptions

	basler->m_state = cBaslerCameraSync::eThreadState::DISCONNECT;
}

