
#include "stdafx.h"
#include "analysisview.h"
#include "3dview.h"

using namespace graphic;
using namespace framework;

cAnalysisView::cAnalysisView(const string &name)
	: framework::cDockWindow(name)
{
}

cAnalysisView::~cAnalysisView()
{
}


void cAnalysisView::OnRender(const float deltaSeconds)
{
	ImGui::Checkbox("calc horz", &g_root.m_isCalcHorz);

	if (ImGui::Button("Volume Measure"))
	{
		g_root.MeasureVolume();
	}

	//ImGui::SameLine();
	//if (ImGui::Button("Analysis Depth"))
	//{
	//	if (!g_root.m_baslerCam.m_sensors.empty())
	//	{
	//		cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
	//		sensor1->m_buffer.AnalysisDepth();
	//	}
	//}

	//ImGui::Spacing();
	//ImGui::Separator();
	//ImGui::Text("Height Area Count = %d", g_root.m_areaCount);
	//ImGui::Text("Distribute Count = %d", g_root.m_distribCount);
	//ImGui::Spacing();
	//ImGui::Spacing();

	ImGui::PushItemWidth(m_rect.Width()-70);

	// Height Distribute 
	{
		ImGui::Text("Height Distribute");
		static int range1 = ARRAYSIZE(g_root.m_measure.m_hDistrib);
		static int scroll1 = 0;
		ImGui::PlotLines2("Height Distribute", &g_root.m_measure.m_hDistrib[scroll1]
			, range1, 0, scroll1, "", 0, 3000, ImVec2(0, 200));

		if (ImGui::SliderInt("Range", &range1, 100, ARRAYSIZE(g_root.m_measure.m_hDistrib)))
			scroll1 = 0;
		ImGui::SliderInt("Scroll", &scroll1, 0, ARRAYSIZE(g_root.m_measure.m_hDistrib) - range1);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Height Distribute2
	{
		ImGui::Text("Height Distribute2");
		static int range = ARRAYSIZE(g_root.m_measure.m_hDistrib2);
		static int scroll = 0;
		ImGui::PlotLines2("Height Distribute2", &g_root.m_measure.m_hDistrib2[scroll]
			, range, 0, scroll, "", 0, 1, ImVec2(0, 200));

		if (ImGui::SliderInt("Range2", &range, 100, ARRAYSIZE(g_root.m_measure.m_hDistrib2)))
			scroll = 0;
		ImGui::SliderInt("Scroll2", &scroll, 0, ARRAYSIZE(g_root.m_measure.m_hDistrib2) - range);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Volume Distribute
	{
		ImGui::Text("Volume Distribute");
		static int range = ARRAYSIZE(g_root.m_measure.m_volDistrib);
		static int scroll = 0;
		ImGui::PlotLines2("Volume Distribute", &g_root.m_measure.m_volDistrib[scroll]
			, range, 0, scroll, "", 0, 100, ImVec2(0, 200));

		if (ImGui::SliderInt("Range3", &range, 100, ARRAYSIZE(g_root.m_measure.m_volDistrib)))
			scroll = 0;
		ImGui::SliderInt("Scroll3", &scroll, 0, ARRAYSIZE(g_root.m_measure.m_volDistrib) - range);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Diff Average
	{
		ImGui::Text("Height Different Average");

		if (!g_root.m_baslerCam.m_sensors.empty())
		{
			cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
			ImGui::PlotLines2("Height Different Average", sensor1->m_buffer.m_diffAvrs.values
				, sensor1->m_buffer.m_diffAvrs.size
				, sensor1->m_buffer.m_diffAvrs.idx, 0, "", 0, .5f, ImVec2(0, 200));
		}
	}


	//ImGui::Spacing();
	//ImGui::Separator();
	//ImGui::Spacing();
	//for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	//{
	//	cRoot::sAreaFloor *area = g_root.m_areaBuff[i];
	//	ImGui::Text("Area-%d", i + 1);
	//	ImGui::Text("AreaSize : %d", area->areaCnt);
	//	ImGui::PlotLines("Area ", area->areaGraph.values
	//		, ARRAYSIZE(area->areaGraph.values)
	//		, area->areaGraph.idx, "", 0, 2000, ImVec2(0, 100));
	//}

	ImGui::PopItemWidth();
}
