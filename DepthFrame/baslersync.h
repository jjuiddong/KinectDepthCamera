//
// 2018-04-07, jjuiddong
// Basler Multi Camera Sync
//
#pragma once


#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;


class cBaslerCameraSync
{
public:
	cBaslerCameraSync();
	virtual ~cBaslerCameraSync();

	bool Init();
	bool Capture();
	void Clear();
	bool IsConnect() const;
	void setTriggerDelays();


protected:
	int BaslerCameraSetup();
	void setupCamera();
	void findMaster();
	void syncCameras();
	void processData(const size_t camIdx, const GrabResult& grabResult);
	int64_t GetMaxAbsGevIEEE1588OffsetFromMasterInTimeWindow(size_t camIdx, double timeToMeasureSec, double timeDeltaSec);


public:
	enum { MAX_CAMS = 10 };

	bool m_isSetupSuccess;
	CameraList m_CameraList;
	vector<std::shared_ptr<CToFCamera>> m_Cameras;
	size_t m_NumCams;
	bool m_IsMaster[MAX_CAMS];
	uint64_t m_SyncTriggerRate;
	int64_t m_TriggerDelay;
	static const int64_t m_ReadoutTime = 21000000;
	static const int64_t m_TriggerBaseDelay = 250000000;    // 250 ms
};


inline bool cBaslerCameraSync::IsConnect() const { return m_isSetupSuccess; }
