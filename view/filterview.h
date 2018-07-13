//
// 2018-03-24, jjuiddong
// Filter View
//
#pragma once


class cFilterView : public framework::cDockWindow
{
public:
	cFilterView(const string &name);
	virtual ~cFilterView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;
	void Process();


public:
	void ProcessDepth();
	void UpdateTexture();


public:
	graphic::cTexture m_depthTexture;
};
