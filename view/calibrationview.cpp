
#include "stdafx.h"
#include "calibrationview.h"
#include "3dview.h"

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


	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	//--------------------------------------------------------------------------
	// debug, offset edit
	ImGui::Text("Camera Offset");
	if (g_root.m_baslerCam.m_sensors.size() >= 1)
	{
		cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
		const bool update1 = ImGui::DragFloat3("Offset1", (float*)&sensor1->m_buffer.m_offset.pos, 0.01f);
		static float rotateY = 0;
		const bool update2 = ImGui::DragFloat("RoateY1", &rotateY, 0.01f);

		if (update1 || update2)
		{
			Quaternion q;
			q.SetRotationY(rotateY);
			sensor1->m_buffer.m_offset.rot = q;

			sensor1->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
			sensor1->m_buffer.UpdatePointCloudItBySelf(g_root.m_3dView->GetRenderer());
		}
	}
	if (g_root.m_baslerCam.m_sensors.size() >= 2)
	{
		cSensor *sensor2 = g_root.m_baslerCam.m_sensors[1];
		const bool update1 = ImGui::DragFloat3("offset2", (float*)&sensor2->m_buffer.m_offset.pos, 0.01f);
		static float rotateY = 0;
		const bool update2 = ImGui::DragFloat("RoateY2", &rotateY, 0.01f);

		if (update1 || update2)
		{
			Quaternion q;
			q.SetRotationY(rotateY);
			sensor2->m_buffer.m_offset.rot = q;

			sensor2->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
			sensor2->m_buffer.UpdatePointCloudItBySelf(g_root.m_3dView->GetRenderer());
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Checkbox("Range Culling", &g_root.m_isRangeCulling);
	ImGui::SameLine();
	if (ImGui::Button("Apply Range Culling"))
	{
		if (g_root.m_baslerCam.IsReadyCapture())
		{
			for (auto sensor : g_root.m_baslerCam.m_sensors)
				sensor->m_buffer.UpdatePointCloudItBySelf(g_root.m_3dView->GetRenderer());
			g_root.MeasureVolume();
		}
	}

	ImGui::DragFloat3("range min", (float*)&g_root.m_cullRangeMin, 1);
	ImGui::DragFloat3("range max", (float*)&g_root.m_cullRangeMax, 1);
	ImGui::Spacing();
	ImGui::Spacing();
}
