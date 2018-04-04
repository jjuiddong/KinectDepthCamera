//
// 2018-03-14, jjuiddong
// Depth2 View
//
#pragma once


class cDepthView2 : public framework::cDockWindow
{
public:
	cDepthView2(const string &name);
	virtual ~cDepthView2();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();


protected:
	void ProcessDepth(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);


public:
	graphic::cTexture m_depthTexture;
	int m_thresholdMin;
	int m_thresholdMax;
};
