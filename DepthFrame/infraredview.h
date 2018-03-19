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


protected:
	void UpdateInfraredImage();
	void ProcessInfrared(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight);


public:
	UINT16 * m_infraredBuffer;
	graphic::cTexture m_infraredTexture;
};
