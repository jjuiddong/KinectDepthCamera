//
// 2018-04-07, jjuiddong
// Basler Multi Camera Sync with Multithreading
//
#pragma once


#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;

#include "datreader.h"

class cSensor;

class cBaslerCameraSync
{
public:
	cBaslerCameraSync(const bool isThreadMode =false);
	virtual ~cBaslerCameraSync();

	bool Init();
	bool Grab();
	void ProcessCmd();
	bool CopyCaptureBuffer(graphic::cRenderer &renderer);
	void Clear();
	bool IsConnect() const;

	int BaslerCameraSetup();
	void setupCamera();
	void findMaster();
	void syncCameras();
	void setTriggerDelays();


protected:
	int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(CToFCamera *tofCam, double timeToMeasureSec, double timeDeltaSec);
	//void processData(const size_t camIdx, const GrabResult& grabResult);


public:
	enum { MAX_CAMS = 10 };

	bool m_isThreadMode;
	bool m_isSetupSuccess;
	vector<cSensor*> m_sensors;
	bool m_oldCameraEnable[MAX_CAMS]; // 바뀐 값을 비교하기 위한 변수
	bool m_isTrySyncTrigger;
	uint64_t m_SyncTriggerRate;
	int64_t m_TriggerDelay;
	static const int64_t m_ReadoutTime = 21000000;
	static const int64_t m_TriggerBaseDelay = 250000000;    // 250 ms

	// multi threading flag
	struct eThreadState { 
		enum Enum { NONE, CONNECT_TRY, CONNECT_FAIL, CONNECT, CAPTURE, DISCONNECT_TRY, DISCONNECT};
	};

	eThreadState::Enum m_state;
	common::cTimer m_timer;
	std::thread m_thread;
	common::CriticalSection m_cs;
	cDatReader m_captureBuff[3]; // camera1,2,3
};


inline bool cBaslerCameraSync::IsConnect() const { return m_isSetupSuccess; }
