//
// 2018-05-19, jjuiddong
// Basler Depth Sensor
//	- multithread grab depth buffer
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
	int Grab();
	bool CopyCaptureBuffer(graphic::cRenderer &renderer, const char *saveFileName);
	void PrepareAcquisition();
	void BeginAcquisition();
	void EndAcquisition();
	
	bool IsEnable();
	void Clear();


protected:
	void CheckAndUpdateParameters();


public:
	int m_id;
	bool m_isEnable;
	bool m_isMaster;
	bool m_isShow;
	double m_writeTime;
	int m_totalGrabCount;
	int m_grabSeconds;
	float m_grabFPS;
	double m_grabTime;
	int m_outlierTolerance;
	int m_confidenceThreshold;
	int m_oldOutlierTolerance;
	int m_oldConfidenceThreshold;

	CameraInfo m_info;
	CToFCamera *m_camera;
	cDatReader m_tempBuffer; // temporary buffer
	cSensorBuffer m_buffer;
	common::CriticalSection m_cs;
};
