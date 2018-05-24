
#include "stdafx.h"
#include "analysisview.h"
#include "3dview.h"
#include "depthframe.h"

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
	if (ImGui::Button("Volume Measure"))
	{
		g_root.MeasureVolume(true);
	}

	ImGui::SameLine();
	if (ImGui::Button("Analysis Depth"))
	{
		if (!g_root.m_baslerCam.m_sensors.empty())
		{
			cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
			sensor1->m_buffer.AnalysisDepth();
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	//ImGui::Text("Height Area Count = %d", g_root.m_areaCount);
	ImGui::Text("Distribute Count = %d", g_root.m_distribCount);

	ImGui::Spacing();
	ImGui::Spacing();

	// Height Distribute 
	{
		ImGui::Text("Height Distribute");
		static int range1 = ARRAYSIZE(g_root.m_hDistrib);
		static int scroll1 = 0;
		ImGui::PlotLines("Height Distribute", &g_root.m_hDistrib[scroll1]
			, range1
			, 0, "", 0, 3000, ImVec2(0, 100));

		if (ImGui::SliderInt("Range", &range1, 100, ARRAYSIZE(g_root.m_hDistrib)))
			scroll1 = 0;
		ImGui::SliderInt("Scroll", &scroll1, 0, ARRAYSIZE(g_root.m_hDistrib) - range1);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Height Distribute2
	{
		ImGui::Text("Height Distribute2");
		static int range = ARRAYSIZE(g_root.m_hDistrib2);
		static int scroll = 0;
		ImGui::PlotLines("Height Distribute2", &g_root.m_hDistrib2[scroll]
			, range
			, 0, "", 0, 1, ImVec2(0, 100));

		if (ImGui::SliderInt("Range2", &range, 100, ARRAYSIZE(g_root.m_hDistrib2)))
			scroll = 0;
		ImGui::SliderInt("Scroll2", &scroll, 0, ARRAYSIZE(g_root.m_hDistrib2) - range);
	}


	ImGui::Spacing();
	ImGui::Spacing();

	// Diff Average
	{
		ImGui::Text("Height Different Average");

		if (!g_root.m_baslerCam.m_sensors.empty())
		{
			cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
			ImGui::PlotLines("Height Different Average", sensor1->m_buffer.m_diffAvrs.values
				, sensor1->m_buffer.m_diffAvrs.size
				, sensor1->m_buffer.m_diffAvrs.idx, "", 0, .5f, ImVec2(0, 100));
		}
	}


	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	for (int i = 0; i < g_root.m_areaFloorCnt; ++i)
	{
		cRoot::sAreaFloor *area = g_root.m_areaBuff[i];
		ImGui::Text("Area-%d", i + 1);
		ImGui::Text("AreaSize : %d", area->areaCnt);
		ImGui::PlotLines("Area ", area->areaGraph.values
			, ARRAYSIZE(area->areaGraph.values)
			, area->areaGraph.idx, "", 0, 2000, ImVec2(0, 100));
	}


	//--------------------------------------------------------------------------
	// debug, offset edit
	if (g_root.m_baslerCam.m_sensors.size() >= 2)
	{
		cSensor *sensor2 = g_root.m_baslerCam.m_sensors[1];
		ImGui::DragFloat3("offset2", (float*)&sensor2->m_buffer.m_offset.pos, 0.01f);
	}

	ImGui::Checkbox("range culling", &g_root.m_isRangeCulling);
	ImGui::SameLine();
	if (ImGui::Button("Apply Range Culling"))
	{
		if (g_root.m_baslerCam.IsReadyCapture())
		{
			for (auto sensor : g_root.m_baslerCam.m_sensors)
				sensor->m_buffer.UpdatePointCloudItBySelf( ((cViewer*)g_application)->m_3dView->GetRenderer() );
			g_root.MeasureVolume();
		}
	}

	ImGui::DragFloat3("range min", (float*)&g_root.m_cullRangeMin, 1);
	ImGui::DragFloat3("range max", (float*)&g_root.m_cullRangeMax, 1);
	ImGui::Spacing();
	ImGui::Spacing();
}
