//
// 2018-04-08, jjuiddong
// Kinect V2 wrapping class
//
#pragma once


class cKinect
{
public:
	cKinect();
	virtual ~cKinect();

	bool Init();
	void Capture(graphic::cRenderer &renderer);
	void Clear();
	bool IsConnect() const;


public:
	bool m_isSetupSuccess;
	IKinectSensor * m_pKinectSensor;
	IDepthFrameReader *m_pDepthFrameReader;
	IColorFrameReader *m_pColorFrameReader;
	IInfraredFrameReader *m_pInfraredFrameReader;
};


inline bool cKinect::IsConnect() const { return m_isSetupSuccess; }
