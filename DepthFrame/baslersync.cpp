
#include "stdafx.h"
#include "baslersync.h"
#include "3dview.h"
#include "depthframe.h"
#include "StopWatch.h"
#include "sensor.h"

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
	, m_isThreadMode(isThreadMode)
	, m_state(eThreadState::NONE)
	, m_isTrySyncTrigger(false)
{
	ZeroMemory(&m_oldCameraEnable, sizeof(m_oldCameraEnable));

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
	//const size_t nImagesToGrab = 10; // Number of images to grab.
	//size_t nImagesGrabbed = 0;

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
		for (cSensor *sensor : m_sensors)
		{
			if (!sensor->IsEnable())
				continue;

			sensor->PrepareAcquisition();
			sensor->BeginAcquisition();
		}

	}
	catch (const GenICam::GenericException& e)
	{
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();

		// After successfully opening the camera, the IsConnected method can be used 
		// to check if the device is still connected.
		for (cSensor *sensor : m_sensors)
		{
			if (sensor->IsEnable() && sensor->m_camera->IsOpen() && !sensor->m_camera->IsConnected() )
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
	CameraList camList = CToFCamera::EnumerateCameras();
	common::dbg::Logp("found %d ToF cameras \n", camList.size());

	// Iterate over list of cameras.
	size_t camIdx = 0;
	for (CameraInfo &cInfo : camList)
	{
		cSensor *sensor = new cSensor;
		sensor->InitCamera(camIdx, cInfo);

		sensor->m_offset = g_root.m_cameraOffset[camIdx];
		m_oldCameraEnable[camIdx] = true;

		++camIdx;
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
		//for (size_t i = 0; i < .size(); i++)
		for (cSensor *sensor : m_sensors)
		{
			if (!sensor->IsEnable())
				continue;

			// Latch IEEE1588 status.
			CCommandPtr ptrGevIEEE1588DataSetLatch = sensor->m_camera->GetParameter("GevIEEE1588DataSetLatch");
			ptrGevIEEE1588DataSetLatch->Execute();

			// Read back latched status.
			GenApi::CEnumerationPtr ptrGevIEEE1588StatusLatched = sensor->m_camera->GetParameter("GevIEEE1588StatusLatched");
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
				sensor->m_isMaster = true;
				//m_IsMaster[i] = true;
				nMaster++;
			}
			else
			{
				sensor->m_isMaster = false;
				//m_IsMaster[i] = false;
			}
		}
	} while ((nMaster > 1)
		&& (!m_isThreadMode || (m_isThreadMode && (eThreadState::DISCONNECT_TRY != m_state)))
		);    // Repeat until there is at most one master left.

	// Use this variable to check whether there is an external master clock.
	bool externalMasterClock = true;

	//for (size_t camIdx = 0; camIdx < m_CameraInfos.size(); camIdx++)
	for (cSensor *sensor : m_sensors)
	{
		if (true == sensor->m_isMaster)
		{
			common::dbg::Logp("   camera %d is master\n", sensor->m_id);
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

	//for (size_t camIdx = 0; camIdx < m_CameraInfos.size(); camIdx++)
	for (cSensor *sensor : m_sensors)
	{
		if (!sensor->IsEnable())
			continue;

		// Check all slaves for deviations from master clock.
		if (false == sensor->m_isMaster)
		{
			uint64_t tsOffset;
			do
			{
				tsOffset = GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(sensor->m_camera, 1.0, 0.1);
				common::dbg::Logp("max offset of cam %d = %d ns\n", sensor->m_id, tsOffsetMax);

			} while ((tsOffset >= tsOffsetMax) 
				&& (!m_isThreadMode || (m_isThreadMode && (eThreadState::DISCONNECT_TRY != m_state))) 
				);
		}
	}
}


int64_t cBaslerCameraSync::GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(CToFCamera *tofCam, double timeToMeasureSec, double timeDeltaSec)
{
	CCommandPtr ptrGevIEEE1588DataSetLatch = tofCam->GetParameter("GevIEEE1588DataSetLatch");
	ptrGevIEEE1588DataSetLatch->Execute();
	CIntegerPtr ptrGevIEEE1588OffsetFromMaster = tofCam->GetParameter("GevIEEE1588OffsetFromMaster");
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
	//for (size_t camIdx = 0; camIdx < m_Cameras.size(); camIdx++)
	for (cSensor *sensor : m_sensors)
	{
		if (!sensor->IsEnable())
			continue;

		common::dbg::Logp("Camera %d : \n", sensor->m_id);
		//
		// Read timestamp and exposure time.
		// Calculation of synchronous free run timestamps will all be based 
		// on timestamp and exposure time(s) of first camera.
		//
		if (sensor->m_id == 0)
		{
			// Latch timestamp registers.
			CCommandPtr ptrTimestampLatch = sensor->m_camera->GetParameter("TimestampLatch");
			ptrTimestampLatch->Execute();

			// Read the two 32-bit halves of the 64-bit timestamp. 
			CIntegerPtr ptrTimeStampLow(sensor->m_camera->GetParameter("TimestampLow"));
			CIntegerPtr ptrTimeStampHigh(sensor->m_camera->GetParameter("TimestampHigh"));
			tsLow = ptrTimeStampLow->GetValue();
			tsHigh = ptrTimeStampHigh->GetValue();

			// Assemble 64-bit timestamp and keep it.
			timestamp = tsLow + (tsHigh << 32);
			common::dbg::Logp("Reading time stamp from first camera. timestamp = %I64d\n", timestamp);

			//cout << "Reading exposure times from first camera:" << endl;

			// Get exposure time count (in case of HDR there will be 2, otherwise 1).
			CIntegerPtr ptrExposureTimeSelector = sensor->m_camera->GetParameter("ExposureTimeSelector");
			size_t n_expTimes = 1 + (size_t)ptrExposureTimeSelector->GetMax();

			// Sum up exposure times.
			CFloatPtr ptrExposureTime = sensor->m_camera->GetParameter("ExposureTime");
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
			m_SyncTriggerRate = 1000000000 / (m_sensors.size() * m_TriggerDelay);

			// If the calculated value is greater than the maximum supported rate, 
			// adjust it. 
			CFloatPtr ptrSyncRate(sensor->m_camera->GetParameter("SyncRate"));
			if (m_SyncTriggerRate > ptrSyncRate->GetMax())
			{
				m_SyncTriggerRate = (uint64_t)ptrSyncRate->GetMax();
			}

			// Print trigger delay and synchronous trigger rate.
			common::dbg::Logp("Trigger delay = %I64d ms\n", m_TriggerDelay / 1000000 );
			common::dbg::Logp("Setting synchronous trigger rate to %I64d fps\n", m_SyncTriggerRate);
		}

		// Set synchronization rate.
		CFloatPtr ptrSyncRate(sensor->m_camera->GetParameter("SyncRate"));
		ptrSyncRate->SetValue((double)m_SyncTriggerRate);

		// Calculate new timestamp by adding trigger delay.
		// First camera starts after triggerBaseDelay, nth camera is triggered 
		// after a delay of triggerBaseDelay +  n * triggerDelay.
		syncStartTimestamp = timestamp + m_TriggerBaseDelay + sensor->m_id * m_TriggerDelay;

		// Disassemble 64-bit timestamp.
		tsHigh = syncStartTimestamp >> 32;
		tsLow = syncStartTimestamp - (tsHigh << 32);

		// Get pointers to the two 32-bit registers, which together hold the 64-bit 
		// synchronous free run start time.
		CIntegerPtr ptrSyncStartLow(sensor->m_camera->GetParameter("SyncStartLow"));
		CIntegerPtr ptrSyncStartHigh(sensor->m_camera->GetParameter("SyncStartHigh"));

		ptrSyncStartLow->SetValue(tsLow);
		ptrSyncStartHigh->SetValue(tsHigh);

		// Latch synchronization start time & synchronization rate registers.
		// Until the values have been latched, they won't have any effect.
		CCommandPtr ptrSyncUpdate = sensor->m_camera->GetParameter("SyncUpdate");
		ptrSyncUpdate->Execute();

		// Show new synchronous free run start time.
		//cout <<  << endl;
		common::dbg::Logp("Setting Sync Start time stamp\n");
		common::dbg::Logp("SyncStartLow = %I64d\n", ptrSyncStartLow->GetValue());
		common::dbg::Logp("SyncStartHigh = %I64d\n", ptrSyncStartHigh->GetValue());
	}
}


// Camera DepthImage Receive On/Off
void cBaslerCameraSync::ProcessCmd()
{
	for (u_int i = 0; i < m_sensors.size(); ++i)
	{
		if (m_oldCameraEnable[i] == m_sensors[i]->IsEnable())
			continue;

		if (m_sensors[i]->IsEnable())
		{
			// Start DepthImage Receive
			m_sensors[i]->m_camera->StartAcquisition();
			m_sensors[i]->m_camera->IssueAcquisitionStartCommand();
		}
		else
		{
			// Stop DepthImage Receive
			m_sensors[i]->m_camera->StopAcquisition();
			m_sensors[i]->m_camera->IssueAcquisitionStopCommand();
		}

		m_oldCameraEnable[i] = m_sensors[i]->IsEnable();
	}
}


// Grab
bool cBaslerCameraSync::Grab()
{
	RETV(!m_isSetupSuccess, false);
	static int grabcnt = 0;

	//for (size_t camIdx = 0; camIdx < m_CameraInfos.size(); camIdx++)
	for (cSensor *sensor : m_sensors)
	{
		if (!sensor->IsEnable())
			continue;

		if (sensor->Grab())
			if (g_root.m_isGrabLog)
				common::dbg::Logp("camIdx = %d, grab = %d\n", sensor->m_id, grabcnt++);
	}

	return true;
}


bool cBaslerCameraSync::CopyCaptureBuffer(graphic::cRenderer &renderer)
{
	//common::AutoCSLock cs(m_cs);

	//const string curTime = common::GetCurrentDateTime();

	//if (g_root.m_sensorBuff[0].m_time < m_captureBuff[0].m_time)
	//	g_root.m_sensorBuff[0].ReadDatFile(renderer, m_captureBuff[0]);
	//if (g_root.m_sensorBuff[1].m_time < m_captureBuff[1].m_time)
	//	g_root.m_sensorBuff[1].ReadDatFile(renderer, m_captureBuff[1]);
	//if (g_root.m_sensorBuff[2].m_time < m_captureBuff[2].m_time)
	//	g_root.m_sensorBuff[2].ReadDatFile(renderer, m_captureBuff[2]);

	//// save PCD file
	//if (g_root.m_isAutoSaveCapture)
	//{
	//	for (int i = 0; i < 3; ++i)
	//	{
	//		// save only processing camera depthmap
	//		if (g_root.m_showCamera[i])
	//		{
	//			common::StrPath fileName;
	//			fileName.Format("../media/depthMulti3/%d/%s.pcd", i, curTime.c_str());
	//			m_captureBuff[i].Write(fileName.c_str());
	//		}
	//	}
	//}

	return true;
}


//void cBaslerCameraSync::processData(const size_t camIdx, const GrabResult& grabResult)
//{
//	BufferParts parts;
//	m_Cameras.at(camIdx)->GetBufferParts(grabResult, parts);
//
//	// Retrieve the values for the center pixel
//	CToFCamera::Coord3D *p3DCoordinate = (CToFCamera::Coord3D*) parts[0].pData;
//	uint16_t *pIntensity = (uint16_t*)parts[1].pData;
//	uint16_t *pConfidence = (uint16_t*)parts[2].pData;
//	const size_t nPixel = parts[0].width * parts[0].height;
//
//	cDatReader reader;
//	memcpy(&reader.m_vertices[0], p3DCoordinate, sizeof(float) * 3 * 640 * 480);
//	memcpy(&reader.m_intensity[0], pIntensity, sizeof(unsigned short) * 640 * 480);
//	memcpy(&reader.m_confidence[0], pConfidence, sizeof(unsigned short) * 640 * 480);
//
//	g_root.m_sensorBuff[camIdx].ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer(), reader);
//
//	// save *.pcd file
//	if (g_root.m_isAutoSaveCapture)
//	{
//		using namespace std;
//		common::StrPath fileName;
//		fileName.Format("../media/depthMulti/%d/%s.pcd", camIdx, common::GetCurrentDateTime().c_str());
//
//		ofstream o(fileName.c_str(), ios::binary);
//		if (o)
//		{
//			o.write((char*)p3DCoordinate, sizeof(float)*nPixel * 3);
//			o.write((char*)pIntensity, sizeof(uint16_t)*nPixel);
//			o.write((char*)pConfidence, sizeof(uint16_t)*nPixel);
//		}
//	}
//}


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
		for (cSensor *sensor : m_sensors)
			delete sensor;

		if (!m_sensors.empty())
		{
			m_sensors.clear();

			if (CToFCamera::IsProducerInitialized())
				CToFCamera::TerminateProducer();  // Won't throw any exceptions
		}
	}
}


// ������ ó��
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

		// �÷��װ� �ٲ��, ī�޶� On/Off ó���Ѵ�.
		//bool isOldCamEnable[cBaslerCameraSync::MAX_CAMS];
		//memcpy(isOldCamEnable, basler->m_isCameraEnable, sizeof(basler->m_isCameraEnable));

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
			
			basler->ProcessCmd();
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

	// remove sensor
	for (cSensor *sensor : basler->m_sensors)
		delete sensor;

	if (!basler->m_sensors.empty())
	{
		basler->m_sensors.clear();

		if (CToFCamera::IsProducerInitialized())
			CToFCamera::TerminateProducer();  // Won't throw any exceptions
	}

	basler->m_state = cBaslerCameraSync::eThreadState::DISCONNECT;
}

