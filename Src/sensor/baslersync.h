//
// 2018-04-07, jjuiddong
// Basler Multi Camera Sync with Multithreading
//
#pragma once

#include "volume.h"
#include "datreader.h"
#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;


class cSensor;

class cBaslerCameraSync
{
public:
	cBaslerCameraSync(const bool isThreadMode =false);
	virtual ~cBaslerCameraSync();

	bool Init();
	bool CopyCaptureBuffer(graphic::cRenderer &renderer);
	bool CreateSensor(const int sensorCount);
	void Clear();
	bool IsTryConnect() const;
	bool IsReadyCapture() const;

	int BaslerCameraSetup();
	void setupCamera();
	void findMaster();
	void syncCameras();
	void setTriggerDelays();
	void ProcessCmd();
	bool Grab();


protected:
	int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(CToFCamera *tofCam, double timeToMeasureSec, double timeDeltaSec);


public:
	bool m_isThreadMode;
	vector<cSensor*> m_sensors;
	bool m_oldCameraEnable[MAX_CAMERA]; // 바뀐 값을 비교하기 위한 변수
	bool m_isTrySyncTrigger;
	uint64_t m_SyncTriggerRate;
	int64_t m_TriggerDelay;
	static const int64_t m_ReadoutTime = 21000000;
	static const int64_t m_TriggerBaseDelay = 250000000;    // 250 ms

	// multi threading flag
	struct eThreadState { 
		enum Enum { NONE, CONNECT_TRY, CONNECT_CONFIG, CONNECT_FAIL, CONNECT, CAPTURE, DISCONNECT_TRY, DISCONNECT};
	};

	eThreadState::Enum m_state;
	std::thread m_thread;
	common::CriticalSection m_cs;
};

