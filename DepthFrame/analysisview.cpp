
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
		g_root.m_sensorBuff.MeasureVolume(((cViewer*)g_application)->m_3dView->GetRenderer());
	}

	ImGui::SameLine();
	if (ImGui::Button("Analysis Depth"))
	{
		g_root.m_sensorBuff.AnalysisDepth();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Text("Height Area Count = %d", g_root.m_areaCount);
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

	// Height Distribute Differential
	{
		ImGui::Text("Height Distribute Differential - 2");
		static int range2 = g_root.m_hDistribDifferential.size;
		static int scroll2 = 0;
		ImGui::PlotLines("Height Distribute Differential - 2", &g_root.m_hDistribDifferential.values[scroll2]
			//, ARRAYSIZE(g_root.m_hDistribDifferential.values)
			, range2
			, 0, "", 0, 3000, ImVec2(0, 100));

		if (ImGui::SliderInt("Range2", &range2, 100, g_root.m_hDistribDifferential.size))
			scroll2 = 0;
		ImGui::SliderInt("Scroll2", &scroll2, 0, g_root.m_hDistribDifferential.size - range2);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Analysis Intensity
	{
		ImGui::Text("Analysis Intensity");
		static int range = g_root.m_sensorBuff.m_analysis1.size;
		static int scroll = 0;
		ImGui::PlotLines("Analysis Intensity", &g_root.m_sensorBuff.m_analysis1.values[scroll]
			, range
			, 0, "", 0, 500, ImVec2(0, 100));

		if (ImGui::SliderInt("Range3", &range, 100, g_root.m_sensorBuff.m_analysis1.size))
			scroll = 0;
		ImGui::SliderInt("Scroll3", &scroll, 0, g_root.m_sensorBuff.m_analysis1.size - range);
	}

	// Analysis Confidence
	{
		ImGui::Text("Analysis Confidence");
		static int range = g_root.m_sensorBuff.m_analysis2.size;
		static int scroll = 0;
		ImGui::PlotLines("Analysis Confidence", &g_root.m_sensorBuff.m_analysis2.values[scroll]
			, range
			, 0, "", 0, 500, ImVec2(0, 100));

		if (ImGui::SliderInt("Range4", &range, 100, g_root.m_sensorBuff.m_analysis2.size))
			scroll = 0;
		ImGui::SliderInt("Scroll4", &scroll, 0, g_root.m_sensorBuff.m_analysis2.size - range);
	}

	//ImGui::Text("Area Min = %d, Max = %d, Avr = %f", g_root.m_areaMin, g_root.m_areaMax
	//	, g_root.m_areaGraph.GetAverage());
	//ImGui::SameLine();
	//if (ImGui::Button("Clear"))
	//{
	//	g_root.m_areaMin = INT_MAX;
	//	g_root.m_areaMax = 0;
	//}

	//ImGui::PlotLines("Area ", g_root.m_areaGraph.values
	//	, ARRAYSIZE(g_root.m_areaGraph.values)
	//	, g_root.m_areaGraph.idx, "", 0, 2000, ImVec2(0, 100));
}
