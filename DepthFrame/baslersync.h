//
// 2018-04-07, jjuiddong
// Basler Multi Camera Sync
//
#pragma once


#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;

#include "datreader.h"

class cBaslerCameraSync
{
public:
	cBaslerCameraSync(const bool isThreadMode =false);
	virtual ~cBaslerCameraSync();

	bool Init();
	bool Capture();
	bool CaptureThread();
	bool CopyCaptureBuffer(graphic::cRenderer &renderer);
	void Clear();
	bool IsConnect() const;

	int BaslerCameraSetup();
	void setupCamera();
	void findMaster();
	void syncCameras();
	void setTriggerDelays();


protected:
	int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(size_t camIdx, double timeToMeasureSec, double timeDeltaSec);
	void processData(const size_t camIdx, const GrabResult& grabResult);


public:
	enum { MAX_CAMS = 10 };

	bool m_isThreadMode;
	bool m_isSetupSuccess;
	CameraList m_CameraList;
	vector<std::shared_ptr<CToFCamera>> m_Cameras;
	size_t m_NumCams;
	bool m_IsMaster[MAX_CAMS];
	uint64_t m_SyncTriggerRate;
	int64_t m_TriggerDelay;
	static const int64_t m_ReadoutTime = 21000000;
	static const int64_t m_TriggerBaseDelay = 250000000;    // 250 ms

	// threading
	struct eThreadState { 
		enum Enum { NONE, CONNECT_TRY, CONNECT_FAIL, CONNECT, CAPTURE, DISCONNECT_TRY, DISCONNECT};
	};

	eThreadState::Enum m_state;
	std::thread m_thread;
	common::cTimer m_timer;

	common::CriticalSection m_cs;
	cDatReader m_captureBuff[2]; // camera1-2
};


inline bool cBaslerCameraSync::IsConnect() const { return m_isSetupSuccess; }
