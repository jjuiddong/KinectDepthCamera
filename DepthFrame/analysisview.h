//
// 2018-03-15, jjuiddong
// Analysis View
//
#pragma once


class cAnalysisView : public framework::cDockWindow
{
public:
	cAnalysisView(const string &name);
	virtual ~cAnalysisView();

	virtual void OnRender(const float deltaSeconds) override;

};
