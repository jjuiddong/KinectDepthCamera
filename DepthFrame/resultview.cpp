
#include "stdafx.h"
#include "resultview.h"
#include "depthframe.h"
#include "inputview.h"
#include "filterview.h"

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

	if (ImGui::Button(u8"���� ����"))
	{
		((cViewer*)g_application)->m_inputView->DelayMeasure();
		((cViewer*)g_application)->m_filterView->ClearBoxVolumeAverage();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	//for (u_int i = 0; i < g_root.m_boxes.size(); ++i)
	//{
	//	auto &box = g_root.m_boxes[i];

	//	// �Ҽ� ù ��° �ڸ����� �ݿø�
	//	// ������ ����, ª�� ���� ����
	//	const int l1 = std::max((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
	//	const int l2 = std::min((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
	//	const int l3 = (int)(box.volume.y + 0.5f);

	//	ImGui::Text("Box%d", i + 1);
	//	ImGui::Text(u8"\t ���� = %d", l1);
	//	ImGui::Text(u8"\t ���� = %d", l2);
	//	ImGui::Text(u8"\t ���� = %d", l3);
	//	ImGui::Text(u8"\t V/W = %.1f", box.minVolume / 6000.f);
	//	//ImGui::Text(u8"\t V/W = %.1f", (box.volume.x * box.volume.y * box.volume.z) / 6000.f);
	//	ImGui::Spacing();
	//	ImGui::Separator();
	//}

	vector<cFilterView::sAvrContour> &avrContours = ((cViewer*)g_application)->m_filterView->m_avrContours;
	for (u_int i = 0; i < avrContours.size(); ++i)
	{
		auto &box = avrContours[i].result;

		const float distribCnt = (float)avrContours[i].count / (float)((cViewer*)g_application)->m_filterView->m_calcAverageCount;
		if (distribCnt < 0.5f)
			continue;		

		// �Ҽ� ù ��° �ڸ����� �ݿø�
		// ������ ����, ª�� ���� ����
		//const int l1 = std::max((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
		//const int l2 = std::min((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
		const float l1 = std::max(box.volume.x, box.volume.z);
		const float l2 = std::min(box.volume.x, box.volume.z);
		const float l3 = box.volume.y;

		ImGui::Text("Box%d", i + 1);
		ImGui::Text(u8"\t ���� = %.1f", l1);
		ImGui::Text(u8"\t ���� = %.1f", l2);
		ImGui::Text(u8"\t ���� = %.1f", l3);
		ImGui::Text(u8"\t V/W = %.1f", box.minVolume / 6000.f);
		ImGui::Text(u8"\t Cnt = %d", avrContours[i].count);
		//ImGui::Text(u8"\t V/W = %.1f", (box.volume.x * box.volume.y * box.volume.z) / 6000.f);
		ImGui::Spacing();
		ImGui::Separator();
	}


	ImGui::EndChild();
	ImGui::PopStyleVar();


	//---------------------------------------------------------------------------------------------------------
	// ���� ���� ���
	if (isSecondColumn)
	{
		ImGui::SetCursorPos(ImVec2(w, pos.y));

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
		ImGui::BeginChild("Sub3", ImVec2(w, m_rect.Height() - 45), true);

		ImGui::Text(u8"���� ���");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		for (u_int i = 0; i < g_root.m_boxesStored.size(); ++i)
		{
			auto &box = g_root.m_boxesStored[i];

			// �Ҽ� ù ��° �ڸ����� �ݿø�
			// ������ ����, ª�� ���� ����
			const float l1 = std::max(box.volume.x, box.volume.z);
			const float l2 = std::min(box.volume.x, box.volume.z);
			const float l3 = box.volume.y;

			ImGui::Text("Box%d", i + 1);
			ImGui::Text(u8"\t ���� = %.1f", l1);
			ImGui::Text(u8"\t ���� = %.1f", l2);
			ImGui::Text(u8"\t ���� = %.1f", l3);
			ImGui::Text(u8"\t V/W = %.1f", box.minVolume / 6000.f);
			//ImGui::Text(u8"\t V/W = %.1f", (box.volume.x * box.volume.y * box.volume.z) / 6000.f);
			ImGui::Spacing();
			ImGui::Separator();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	ImGui::PopFont();
}

