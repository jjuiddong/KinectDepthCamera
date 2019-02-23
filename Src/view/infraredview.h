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
	void Process(const size_t camIdx = 0);


protected:
	void UpdateKinectInfraredImage();
	void ProcessInfrared(const UINT16* pBuffer, int nWidth, int nHeight);


public:
	graphic::cTexture m_infraredTexture;
};
