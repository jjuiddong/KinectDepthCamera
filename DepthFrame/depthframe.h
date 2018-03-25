//
// 2018-03-13, jjuiddong
//
#pragma once


class c3DView;
class cColorView;
class cDepthView;
class cDepthView2;
class cInfraredView;
class cAnalysisView;
class cInputView;
class cFilterView;
class cLogView;

class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;


public:
	c3DView *m_3dView;
	cColorView *m_colorView;
	cDepthView *m_depthView;
	cDepthView2 *m_depthView2;
	cInfraredView *m_infraredView;
	cAnalysisView *m_analysisView;
	cInputView *m_inputView;
	cFilterView *m_filterView;
	cLogView *m_logView;
};
