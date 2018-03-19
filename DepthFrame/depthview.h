//
// 2018-03-14, jjuiddong
// Depth View
//
#pragma once


class cDepthView : public framework::cDockWindow
{
public:
	cDepthView(const string &name);
	virtual ~cDepthView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void ProcessDepth(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);


public:
	graphic::cTexture m_depthTexture;
};