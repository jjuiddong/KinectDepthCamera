//
// 2018-05-19, jjuiddong
// Basler Depth Sensor
//
//
#pragma once


#include <ConsumerImplHelper/ToFCamera.h>
using namespace GenTLConsumerImplHelper;


class cSensor
{
public:
	cSensor();
	virtual ~cSensor();

	bool InitCamera(const int id, const CameraInfo &cinfo);
	bool Grab();
	bool CopyCaptureBuffer(graphic::cRenderer &renderer, const char *saveFileName);
	void PrepareAcquisition();
	void BeginAcquisition();
	void EndAcquisition();
	bool IsEnable();
	void Clear();


public:
	int m_id;
	bool m_isEnable;
	bool m_isMaster;
	bool m_isShow;
	CameraInfo m_info;
	CToFCamera *m_camera;
	cDatReader m_tempBuffer; // temporary buffer
	cSensorBuffer m_buffer;
	common::CriticalSection m_cs;
	common::Transform m_offset;
};
