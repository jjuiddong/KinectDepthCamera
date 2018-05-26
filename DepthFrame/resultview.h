//
// 2018-03-27, jjuiddong
// ResultView
//
#pragma once


class cResultView : public framework::cDockWindow
{
public:
	cResultView(const string &name);
	virtual ~cResultView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnRender(const float deltaSeconds) override;


protected:
	ImFont * m_font;
	cDBClient m_dbClient;
};
