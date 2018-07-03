
#include "stdafx.h"
#include "logview.h"

using namespace common;
using namespace graphic;


cLogView::cLogView(const string &name)
	: framework::cDockWindow(name)
	, m_isScroll(false)
{
	m_first = 0;
	m_last = 0;
	m_logs.resize(500);
}

cLogView::~cLogView()
{
}


void cLogView::OnRender(const float deltaSeconds)
{
	if (ImGui::Button("Clear"))
	{
		m_first = 0;
		m_last = 0;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Sub2", ImVec2(0, m_rect.Height()-85), true);

	{
		common::AutoCSLock cs(m_cs);
		u_int i = (u_int)m_first;
		while (i != (u_int)m_last)
		{
			ImGui::Selectable(m_logs[i].c_str());
			i = (i + 1) % m_logs.size();
		}

		if (m_isScroll)
		{
			ImGui::SetScrollHere();
			m_isScroll = false;
		}
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
}


// push circular queue
void cLogView::AddLog(const string &str)
{
	common::AutoCSLock cs(m_cs);

	m_logs[m_last++] = str.c_str();

	if ((u_int)m_last >= m_logs.size())
	{
		m_last = 0;
		m_first = (m_first + 1) % m_logs.size();
	}

	m_isScroll = true;
}


void AddLog(const string &str)
{
	g_root.m_logView->AddLog(str);
	//common::dbg::Print(str);
}
