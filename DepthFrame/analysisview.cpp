
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
	ImGui::DragInt2("Height Error Upper, Lower", g_root.m_heightErr, 1, 0, 20);
	if (ImGui::Button("Volume Measure"))
	{
		//((cViewer*)g_application)->m_3dView->MeasureVolume();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Height Area Count = %d", g_root.m_areaCount);
	ImGui::Text("Distribute Count = %d", g_root.m_distribCount);
	
	ImGui::PlotLines("Height Distribute", g_root.m_hDistrib
		, ARRAYSIZE(g_root.m_hDistrib)
		, 0, "", 0, 600, ImVec2(0, 100));

	ImGui::PlotLines("Height Distribute Differential - 2", g_root.m_hDistribDifferential.values
		, ARRAYSIZE(g_root.m_hDistribDifferential.values)
		, 0, "", 0, 600, ImVec2(0, 100));

	ImGui::Text("Area Min = %d, Max = %d, Avr = %f", g_root.m_areaMin, g_root.m_areaMax
		, g_root.m_areaGraph.GetAverage());
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		g_root.m_areaMin = INT_MAX;
		g_root.m_areaMax = 0;
	}

	ImGui::PlotLines("Area ", g_root.m_areaGraph.values
		, ARRAYSIZE(g_root.m_areaGraph.values)
		, g_root.m_areaGraph.idx, "", 0, 2000, ImVec2(0, 100));
}
