
#include "stdafx.h"
#include "cameraview.h"

using namespace graphic;


cCameraView::cCameraView(const string &name)
	: framework::cDockWindow(name)
{

}

cCameraView::~cCameraView()
{

}


void cCameraView::OnRender(const float deltaSeconds)
{
	if (!g_root.m_baslerCam.IsReadyCapture())
		return;

	int camIdx = 0;
	for (auto &sensor : g_root.m_baslerCam.m_sensors)
	{
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
		if (ImGui::TreeNode((void*)sensor, sensor->m_info.strDisplayName.c_str()))
		{
			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				;// | ((idx == m_selFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			int idx = 0;
			Str128 text;
			if (ImGui::Checkbox("Enable", &sensor->m_isEnable))
			{
				sensor->m_isShow = sensor->m_isEnable;
			}

			text.Format("Model Name = %s", sensor->m_info.strModelName.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags,text.c_str());
			text.Format("Device ID = %s", sensor->m_info.strDeviceID.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
			text.Format("Serial Number = %s", sensor->m_info.strSerialNumber.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
			text.Format("IpAddress = %s", sensor->m_info.strIpAddress.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
	
			ImGui::TreePop();
		}

		++camIdx;
	}
}
