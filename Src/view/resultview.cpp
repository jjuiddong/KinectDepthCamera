
#include "stdafx.h"
#include "resultview.h"
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

	const bool isSecondColumn = !g_root.m_measure.m_boxesStored.empty();
	const float w = isSecondColumn? (m_rect.Width() / 2.f) - 5 : 0;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Sub2", ImVec2(w, m_rect.Height() - 45), true);

	static bool isMeasureVolume = false;
	if (ImGui::Button(u8"측정(200)"))
	{
		++g_root.m_measure.m_measureId;
		isMeasureVolume = true;
		g_root.m_inputView->DelayMeasure();
		//g_root.m_filterView->ClearBoxVolumeAverage();
		g_root.m_measure.ClearBoxVolumeAverage();
	}

	ImGui::SameLine();
	if (ImGui::Button(u8"측정(20)"))
	{
		++g_root.m_measure.m_measureId;
		isMeasureVolume = true;
		g_root.m_inputView->DelayMeasure10();
		//g_root.m_filterView->ClearBoxVolumeAverage();
		g_root.m_measure.ClearBoxVolumeAverage();
	}

	if (ImGui::Button(u8"취소"))
	{
		isMeasureVolume = false;
		g_root.m_inputView->CancelDelayMeasure();
		//g_root.m_filterView->ClearBoxVolumeAverage();
		g_root.m_measure.ClearBoxVolumeAverage();
	}

	ImGui::SameLine();
	ImGui::Text("%d", g_root.m_inputView->m_measureCount);

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

//	vector<cFilterView::sAvrContour> &avrContours = g_root.m_filterView->m_avrContours;
	vector<sAvrContour> &avrContours = g_root.m_measure.m_avrContours;
	for (u_int i = 0; i < avrContours.size(); ++i)
	{
		auto &box = avrContours[i].result;

		const float distribCnt = (float)avrContours[i].count / (float)g_root.m_measure.m_calcAverageCount;
		if (distribCnt < 0.5f)
			continue;		

		// 소수 첫 번째 자리에서 반올림
		// 긴쪽이 가로, 짧은 쪽이 세로
		//const int l1 = std::max((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
		//const int l2 = std::min((int)(box.volume.x + 0.5f), (int)(box.volume.z + 0.5f));
		const float l1 = std::max(box.volume.x, box.volume.z);
		const float l2 = std::min(box.volume.x, box.volume.z);
		const float l3 = box.volume.y;

		ImGui::Text("Box%d", i + 1);
		ImGui::Text(u8"\t 가로 = %.1f", l1);
		ImGui::Text(u8"\t 세로 = %.1f", l2);
		ImGui::Text(u8"\t 높이 = %.1f", l3);
		ImGui::Text(u8"\t V/W = %.1f", box.minVolume / 6000.f);
		ImGui::Text(u8"\t Cnt = %d", avrContours[i].count);
		//ImGui::Text(u8"\t V/W = %.1f", (box.volume.x * box.volume.y * box.volume.z) / 6000.f);
		ImGui::Spacing();
		ImGui::Separator();
	}


	ImGui::EndChild();
	ImGui::PopStyleVar();


	//---------------------------------------------------------------------------------------------------------
	// 지연 측정 결과
	if (isSecondColumn)
	{
		if (isMeasureVolume && !g_root.m_measure.m_boxesStored.empty())
		{
			isMeasureVolume = false;
			sMeasureResult result;
			result.id = g_root.m_measure.m_measureId;
			result.type = 1; // delay measure
			int id = 0;
			for (auto &box : g_root.m_measure.m_boxesStored)
			{
				const float l1 = std::max(box.volume.x, box.volume.z);
				const float l2 = std::min(box.volume.x, box.volume.z);
				const float l3 = box.volume.y;

				sMeasureVolume info;
				info.integral = box.integral;
				info.id = id++;
				info.horz = l1;
				info.vert = l2;
				info.height = l3;
				info.pos = box.pos;
				info.volume = box.minVolume;
				info.vw = box.minVolume / 6000.f;
				info.pointCount = box.pointCnt;
				result.volumes.push_back(info);
			}

			if (!result.volumes.empty())
				g_root.m_dbClient.Insert(result);
		}


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

		for (u_int i = 0; i < g_root.m_measure.m_boxesStored.size(); ++i)
		{
			auto &box = g_root.m_measure.m_boxesStored[i];

			// 소수 첫 번째 자리에서 반올림
			// 긴쪽이 가로, 짧은 쪽이 세로
			const float l1 = std::max(box.volume.x, box.volume.z);
			const float l2 = std::min(box.volume.x, box.volume.z);
			const float l3 = box.volume.y;

			ImGui::Text("Box%d", i + 1);
			ImGui::Text(u8"\t 가로 = %.1f", l1);
			ImGui::Text(u8"\t 세로 = %.1f", l2);
			ImGui::Text(u8"\t 높이 = %.1f", l3);
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

