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
	cBaslerCamera();
	virtual ~cBaslerCamera();

	bool Init();
	bool Capture();
	void Clear();


protected:
	int BaslerCameraSetup();
	void setupCamera();
	void processData(const GrabResult& grabResult);


public:
	bool m_isSetupSuccess;
	CToFCamera * m_Camera;
};
