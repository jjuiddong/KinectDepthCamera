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


protected:
	int BaslerCameraSetup();
	void setupCamera();
	void processData(const GrabResult& grabResult);


public:
	enum { MAX_CAMS = 10 };

	bool m_isSetupSuccess;
	CToFCamera * m_Camera;
	CameraList m_CameraList;
	vector<std::shared_ptr<CToFCamera>> m_Cameras;
	size_t m_NumCams;
	bool m_IsMaster[MAX_CAMS];
};
