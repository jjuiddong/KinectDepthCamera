
#include "stdafx.h"
#include "resultview.h"
#include "depthframe.h"

using namespace common;
using namespace graphic;


cResultView::cResultView(const string &name)
	: framework::cDockWindow(name)
	, m_font(NULL)
{
}

cResultView::~cResultView()
{
}


bool cResultView::Init(graphic::cRenderer &renderer)
{
	const float fontSize = 50;
	ImGuiIO& io = ImGui::GetIO();

	return true;
}


void cResultView::OnRender(const float deltaSeconds)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Sub2", ImVec2(0, m_rect.Height() - 85), true);
	ImGui::PushFont(m_owner->m_fontBig);

//	ImGui::Text("Test");
	ImGui::Spacing();
	ImGui::Spacing();

	for (u_int i = 0; i < g_root.m_boxes.size(); ++i)
	{
		auto &box = g_root.m_boxes[i];
		ImGui::Text("Box%d", i + 1);
		ImGui::Text("\t X = %f", box.volume.x);
		ImGui::Text("\t Y = %f", box.volume.z);
		ImGui::Text("\t H = %f", box.volume.y);
		ImGui::Spacing();
		ImGui::Separator();
	}

	ImGui::PopFont();
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

