
#include "stdafx.h"
#include "calibrationview.h"

using namespace graphic;


cCalibrationView::cCalibrationView(const string &name)
	: framework::cDockWindow(name)
	, m_isCalc(false)
{

}

cCalibrationView::~cCalibrationView()
{

}


void cCalibrationView::OnRender(const float deltaSeconds)
{
	if (!g_root.m_baslerCam.IsReadyCapture())
		return;

	ImGui::Spacing();
	ImGui::Spacing();

	static StrPath fileName = "depthframe_calibration.txt";
	ImGui::Text("FileName : ");
	ImGui::SameLine();
	ImGui::PushID(0);
	ImGui::InputText("", fileName.m_str, fileName.SIZE);
	ImGui::PopID();
	ImGui::SameLine();
	if (ImGui::Button("..."))
	{
	}

	//ImGui::SameLine();
	//if (ImGui::Button("Read Detail"))
	//{
	//}

	if (ImGui::Button("Calibration"))
	{
		cCalibration calib;
		m_isCalc = calib.CalibrationBasePlane(fileName.c_str(), m_result);
	}

	if (m_isCalc)
	{
		ImGui::Spacing();

		ImGui::Text("current sd = %f", m_result.curSD);
		ImGui::Text("calibration sd = %f", m_result.minSD);
		ImGui::Text("plane x=%f, y=%f, z=%f, d=%f"
			, m_result.plane.N.x, m_result.plane.N.y, m_result.plane.N.z, m_result.plane.D);
	}
}
