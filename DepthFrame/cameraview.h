//
// 2018-05-17, jjuiddong
// Camera Information View
//
#pragma once


class cCameraView : public framework::cDockWindow
{
public:
	cCameraView(const string &name);
	virtual ~cCameraView();

	virtual void OnRender(const float deltaSeconds) override;
};
