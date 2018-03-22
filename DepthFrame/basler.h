//
// 2018-03-22, jjuiddong
// Basler
//
#pragma once


class cBaslerView : public framework::cDockWindow
{
public:
	cBaslerView(const string &name);
	virtual ~cBaslerView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;


public:
};
