
#include "stdafx.h"
#include "baslersync.h"
#include "StopWatch.h"
#include "sensor.h"
#include <direct.h>

using namespace GenApi;

void BaslerCameraThread(cBaslerCameraSync *basler);

void mSleep(int sleepMs)      // Use a wrapper  
{
	Sleep(sleepMs);           // Sleep expects time in ms
}


cBaslerCameraSync::cBaslerCameraSync(const bool isThreadMode //= false
)
	: m_isThreadMode(isThreadMode)
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
			::MessageBoxA(g_root.m_hwnd, msg.c_str(), "Error", MB_OK);
			common::dbg::Logp(msg.c_str());

			return false;
		}
	}

	return true;
}


bool cBaslerCameraSync::CreateSensor(const int sensorCount)
{
	while (eThreadState::CONNECT_TRY == m_state)
		Sleep(10);

	if (m_sensors.size() >= (u_int)sensorCount)
	{
		// Remove Sensor
		common::AutoCSLock cs(m_cs);
		for (u_int i = (u_int)sensorCount; i < m_sensors.size(); ++i)
			SAFE_DELETE(m_sensors[i]);
		const u_int sensorSize = m_sensors.size();
		for (u_int i = (u_int)sensorCount; i < sensorSize; ++i)
			m_sensors.pop_back();
		return false;
	}

	const int initSensorCnt = sensorCount - m_sensors.size();
	const int firstId = m_sensors.size();
	for (int i = 0; i < initSensorCnt; ++i)
	{
		cSensor *sensor = new cSensor();
		sensor->m_id = firstId + i;
		sensor->m_isEnable = true;
		sensor->m_isShow = true;
		sensor->m_info.strDisplayName = common::format("temp camera%d", i + 1);
		sensor->m_buffer.m_offset = g_root.m_cameraOffset[i];
		sensor->m_buffer.m_planeSub = g_root.m_planeSub[i];
		sensor->m_buffer.m_cullRect = g_root.m_cullRect[i];
		sensor->m_buffer.m_cullExtraRect = g_root.m_extraCullRect[i];
		sensor->m_buffer.m_mergeOffset = (i == 1);

		m_sensors.push_back(sensor);
	}

	return true;
}


int cBaslerCameraSync::BaslerCameraSetup()
{
	try
	{
		//
		// Open and parameterize the camera.
		//

		setupCamera();

		if (!m_sensors.empty() 
			&& (cBaslerCameraSync::eThreadState::CONNECT_TRY == m_state))
			m_state = eThreadState::CONNECT_CONFIG;

		findMaster();

		syncCameras();

		setTriggerDelays();

		// Prepare cameras and buffers for image exposure.
		for (cSensor *sensor : m_sensors)
		{
			sensor->PrepareAcquisition();

			if (!sensor->IsEnable())
				continue;

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

		::MessageBoxA(g_root.m_hwnd, msg.c_str(), "Error", MB_OK);
		common::dbg::Logp(msg.c_str());

		return EXIT_FAILURE;
	}

	if (m_sensors.empty())
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


void cBaslerCameraSync::setupCamera()
{
	common::dbg::Logp("Searching for cameras ... \n");
	CameraList camList = CToFCamera::EnumerateCameras();
	common::dbg::Logp("found %d ToF cameras \n", camList.size());

	// Parsing camera-connect
	// ex) Camera1,Camera2,Camera3
	string cfgCameraConnect = g_root.m_config.GetString("camera-connect");
	vector<string> conCams;
	common::tokenizer(cfgCameraConnect, ",", "", conCams);

	// Parsing camera-mapping
	// ex) Camera1:0,Camera2:1
	string cfgCameraOffsetmap = g_root.m_config.GetString("camera-mapping");
	vector<string> tokCameraOffsetmap;
	common::tokenizer(cfgCameraOffsetmap, ",", "", tokCameraOffsetmap);

	vector<std::pair<string, int>> cameraOffsetmap;
 	if (!tokCameraOffsetmap.empty())
	{
		for (const auto &tok : tokCameraOffsetmap)
		{
			if (tok.empty())
				continue;

			vector<string> out;
			common::tokenizer(tok, ":", "", out);
			if (out.size() < 2)
			{
				assert(0); // error occur
				continue;
			}

			cameraOffsetmap.push_back( std::make_pair<string, int>(out[0].c_str(), atoi(out[1].c_str())) );
		}
	}

	// Iterate over list of cameras.
	size_t camIdx = 0;
	for (CameraInfo &cInfo : camList)
	{
		if (eThreadState::CONNECT_TRY != m_state)
			continue;

		if (!conCams.empty())
		{
			auto it = std::find(conCams.begin(), conCams.end(), cInfo.strDisplayName);
			if (conCams.end() == it)
				continue;
		}

		cSensor *sensor = new cSensor;
		sensor->InitCamera(camIdx, cInfo);

		// Setting Offset Parameter
		if (cameraOffsetmap.empty())
		{
			sensor->m_buffer.m_offset = g_root.m_cameraOffset[camIdx];
			sensor->m_buffer.m_planeSub = g_root.m_planeSub[camIdx];
		}
		else
		{
			int parameterIdx = -1;
			for (u_int i=0; i < cameraOffsetmap.size(); ++i)
			{
				const auto &val = cameraOffsetmap[i];
				if (val.first == cInfo.strDisplayName)
				{
					parameterIdx = val.second;
					common::dbg::Logp("%s Camera Parameter Idx = %d\n", cInfo.strDisplayName.c_str(), val.second);
					break;
				}
			}
			if ((parameterIdx < 0) || (parameterIdx >= MAX_CAMERA))
			{
				assert(0);
				parameterIdx = camIdx;
			}

			sensor->m_buffer.m_offset = g_root.m_cameraOffset[parameterIdx];
			sensor->m_buffer.m_planeSub = g_root.m_planeSub[parameterIdx];
			sensor->m_buffer.m_cullRect = g_root.m_cullRect[parameterIdx];
			sensor->m_buffer.m_cullExtraRect = g_root.m_extraCullRect[parameterIdx];
		}
		
		sensor->m_buffer.m_mergeOffset = (camIdx == 1);
		m_oldCameraEnable[camIdx] = true;
		//m_oldCameraEnable[camIdx] = false;

		m_sensors.push_back(sensor);

		++camIdx;
	}
}


void cBaslerCameraSync::findMaster()
{
	if (m_sensors.size() <= 1)
		return;

	// Number of masters found ( != 1 ) 
	unsigned int nMaster;

	common::dbg::Logp("waiting for cameras to negotiate master role ...\n");

	int tryCount = 0;
	do
	{
		common::dbg::Logp("find master camera...%d \n", tryCount++);

		nMaster = 0;
		//
		// Wait until a master camera (if any) and the slave cameras have been chosen.
		// Note that if a PTP master clock is present in the subnet, all TOF cameras
		// ultimately assume the slave role.
		//
		for (cSensor *sensor : m_sensors)
		{
			//if (!sensor->IsEnable())
			//	continue;

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
		//if (!sensor->IsEnable())
		//	continue;

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
		//if (!sensor->IsEnable())
		//	continue;

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

		try
		{
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
		}
		catch (GenICam::GenericException& e)
		{
			common::Str128 msg = "Exception occurred: ";
			msg += e.GetDescription();
			::MessageBoxA(g_root.m_hwnd, msg.c_str(), "Error", MB_OK);
			common::dbg::Logp(msg.c_str());

			//m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
		}

		m_oldCameraEnable[i] = m_sensors[i]->IsEnable();
	}
}


// Grab
bool cBaslerCameraSync::Grab()
{
	// Frame Control
	static common::cTimer s_timer;
	static bool onlyOne = true;
	if (onlyOne)
	{
		onlyOne = false;
		s_timer.Create();
	}

	static int grabcnt = 0;
	static double oldT = s_timer.GetSeconds();
	const double curT = s_timer.GetSeconds();
	if (curT - oldT < 0.03f)
		return true;
	oldT = curT;
	//

	for (cSensor *sensor : m_sensors)
	{
		if (!sensor->IsEnable())
			continue;

		bool continueGrab = true;
		int cnt = 0;
		while (continueGrab && (cnt++ < 1))
		{
			const int result = sensor->Grab();
			switch (result)
			{
			case 1: // success
				if (g_root.m_isGrabLog)
					common::dbg::Logp("camIdx = %d, grab = %d\n", sensor->m_id, grabcnt++);
				continueGrab = false;
				break;

			case 2: // not ready
				continueGrab = false;
				break;

			case 3: // time out
			case 4: // grabbed fail
				if (g_root.m_isGrabErrLog)
				{
					common::dbg::Logp("Error Grabbed [ %s ], cnt = %d, code = %s \n"
						, sensor->m_info.strDisplayName.c_str(), cnt, (result==3)? "time out" : "grab fail");
				}
				continueGrab = true;
				break;
			}
		}
	}

	return true;
}


bool cBaslerCameraSync::CopyCaptureBuffer(graphic::cRenderer &renderer)
{
	const string curTime = common::GetCurrentDateTime();
	for (cSensor *sensor : m_sensors)
	{
		if (!sensor->IsEnable())
			continue;

		const char *dir = "../media/DepthSave";
		common::StrPath fileName;
		fileName.Format("%s/%d/%s.pcd", dir, sensor->m_id, curTime.c_str());

		// Check Exist Directory, if no such file directory, create directory
		{
			common::StrPath dir0(dir);
			if (!dir0.IsFileExist())
				_mkdir(dir);

			common::StrPath dir1;
			dir1.Format("%s/%d", dir, sensor->m_id);
			if (!dir1.IsFileExist())
				_mkdir(dir1.c_str());
		}

		sensor->CopyCaptureBuffer(renderer, fileName.c_str());
	}

	return true;
}


bool cBaslerCameraSync::IsTryConnect() const
{
	switch (m_state)
	{
	case eThreadState::CONNECT_TRY:
	case eThreadState::CONNECT_CONFIG:
		return true;
	}
	return false;
}


bool cBaslerCameraSync::IsReadyCapture() const
{
	switch (m_state)
	{
	case eThreadState::NONE:
	case eThreadState::CONNECT_TRY:
	case eThreadState::CONNECT_CONFIG:
		return false;
	}
	return true;
}


void cBaslerCameraSync::Clear()
{
	if (m_isThreadMode)
	{
		m_state = eThreadState::DISCONNECT_TRY;

		if (m_thread.joinable())
			m_thread.join();
		else
			m_state = eThreadState::DISCONNECT;
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


// 쓰레드 처리
void BaslerCameraThread(cBaslerCameraSync *basler)
{
	try
	{
		CToFCamera::InitProducer();

		if (EXIT_SUCCESS == basler->BaslerCameraSetup())
		{
			double triggerDelayTime = g_root.m_timer.GetMilliSeconds();

			if (cBaslerCameraSync::eThreadState::CONNECT_CONFIG == basler->m_state)
				basler->m_state = cBaslerCameraSync::eThreadState::CAPTURE;

			while (cBaslerCameraSync::eThreadState::CAPTURE == basler->m_state)
			{
				if (g_root.m_timer.GetMilliSeconds() - triggerDelayTime > 1000)
				{
					//basler->setTriggerDelays();
					triggerDelayTime = g_root.m_timer.GetMilliSeconds();
				}

				if (basler->m_isTrySyncTrigger)
				{
					basler->setTriggerDelays();
					basler->m_isTrySyncTrigger = false;
				}

				// Grab
				{
					common::AutoCSLock cs(basler->m_cs);
					basler->ProcessCmd();
					basler->Grab();
				}
			}

			basler->m_state = cBaslerCameraSync::eThreadState::DISCONNECT;
		}
		else
		{
			basler->m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
		}
	}
	catch (GenICam::GenericException& e)
	{
		common::Str128 msg = "Exception occurred: ";
		msg += e.GetDescription();
		::MessageBoxA(g_root.m_hwnd, msg.c_str(), "Error", MB_OK);
		common::dbg::Logp(msg.c_str());

		basler->m_state = cBaslerCameraSync::eThreadState::CONNECT_FAIL;
	}

	// remove sensor
	for (cSensor *sensor : basler->m_sensors)
		delete sensor;

	if (!basler->m_sensors.empty())
	{
		basler->m_sensors.clear();
	}

	if (CToFCamera::IsProducerInitialized())
		CToFCamera::TerminateProducer();  // Won't throw any exceptions

	basler->m_state = cBaslerCameraSync::eThreadState::DISCONNECT;
}

