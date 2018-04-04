
#include "stdafx.h"
#include "resultview.h"
#include "depthframe.h"
#include "inputview.h"

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
	const ImVec2 pos = ImGui::GetCursorPos();
	ImGui::PushFont(m_owner->m_fontBig);

	const bool isSecondColumn = !g_root.m_boxesStored.empty();
	const float w = isSecondColumn? (m_rect.Width() / 2.f) - 5 : 0;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Sub2", ImVec2(w, m_rect.Height() - 45), true);

	if (ImGui::Button(u8"길이 측정"))
	{
		((cViewer*)g_application)->m_inputView->DelayMeasure();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	for (u_int i = 0; i < g_root.m_boxes.size(); ++i)
	{
		auto &box = g_root.m_boxes[i];

		// 소수 첫 번째 자리에서 반올림
		// 긴쪽이 가로, 짧은 쪽이 세로
		ImGui::Text("Box%d", i + 1);
		ImGui::Text(u8"\t 가로 = %d", std::max((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f)));
		ImGui::Text(u8"\t 세로 = %d", std::min((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f)));
		ImGui::Text(u8"\t 높이 = %d", (int)(box.volume.y + 0.5f));
		ImGui::Spacing();
		ImGui::Separator();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();


	//---------------------------------------------------------------------------------------------------------
	// 지연 측정 결과
	if (isSecondColumn)
	{
		ImGui::SetCursorPos(ImVec2(w, pos.y));

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
		ImGui::BeginChild("Sub3", ImVec2(w, m_rect.Height() - 45), true);

		ImGui::Text(u8"측정 결과");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		for (u_int i = 0; i < g_root.m_boxesStored.size(); ++i)
		{
			auto &box = g_root.m_boxesStored[i];

			// 소수 첫 번째 자리에서 반올림
			// 긴쪽이 가로, 짧은 쪽이 세로
			ImGui::Text("Box%d", i + 1);
			ImGui::Text(u8"\t 가로 = %d", std::max((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f)));
			ImGui::Text(u8"\t 세로 = %d", std::min((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f)));
			ImGui::Text(u8"\t 높이 = %d", (int)(box.volume.y + 0.5f));
			ImGui::Spacing();
			ImGui::Separator();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	ImGui::PopFont();
}

