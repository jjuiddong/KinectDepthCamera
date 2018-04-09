//
// 2018-04-07, jjuiddong
// Basler Camera
//
#pragma once


#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;


class cBaslerCamera
{
public:
	cBaslerCamera(const bool isThreadMode = false);
	virtual ~cBaslerCamera();

	bool Init();
	bool Capture();
	void Clear();
	bool IsConnect() const;
	void setTriggerDelays() {}


protected:
	int BaslerCameraSetup();
	void setupCamera();
	void processData(const GrabResult& grabResult);


public:
	bool m_isSetupSuccess;
	CToFCamera * m_Camera;
};


inline bool cBaslerCamera::IsConnect() const { return m_isSetupSuccess;  }
