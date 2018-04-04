//
// 2018-03-14, jjuiddong
// Infrared View
//
#pragma once


class cInfraredView : public framework::cDockWindow
{
public:
	cInfraredView(const string &name);
	virtual ~cInfraredView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();


protected:
	void UpdateKinectInfraredImage();
	void ProcessInfrared(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight);


public:
	graphic::cTexture m_infraredTexture;
};
