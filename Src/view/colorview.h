//
// 2018-03-14, jjuiddong
// Color View
//
#pragma once


class cColorView : public framework::cDockWindow
{
public:
	cColorView(const string &name);
	virtual ~cColorView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void UpdateColorImage();
	void ProcessColor(INT64 nTime, const BYTE* pBuffer, int nWidth, int nHeight);


public:
	BYTE * m_colorBuffer;
	graphic::cTexture m_colorTexture;
};
