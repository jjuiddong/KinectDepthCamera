//
// 2018-03-25, jjuiddong
// LogView
//
#pragma once


class cLogView : public framework::cDockWindow
{
public:
	cLogView(const string &name);
	virtual ~cLogView();

	virtual void OnRender(const float deltaSeconds) override;
	void AddLog(const string &str);


protected:
	// circular queue
	int m_first;
	int m_last;
	bool m_isScroll;
	vector<common::Str128> m_logs;
	common::CriticalSection m_cs;
};


extern void AddLog(const string &str);
