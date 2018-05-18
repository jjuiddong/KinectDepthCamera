
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
	if ((cBaslerCameraSync::eThreadState::NONE == g_root.m_balserCam.m_state)
		|| (cBaslerCameraSync::eThreadState::CONNECT_TRY == g_root.m_balserCam.m_state))
		return;

	int camIdx = 0;
	for (auto &cam : g_root.m_balserCam.m_CameraInfos)
	{
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
		if (ImGui::TreeNode((void*)&cam, cam.strDisplayName.c_str()))
		{
			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				;// | ((idx == m_selFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			int idx = 0;
			Str128 text;
			ImGui::Checkbox("Enable", &g_root.m_balserCam.m_isCameraEnable[camIdx]);

			text.Format("Model Name = %s", cam.strModelName.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags,text.c_str());
			text.Format("Device ID = %s", cam.strDeviceID.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
			text.Format("Serial Number = %s", cam.strSerialNumber.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
			text.Format("IpAddress = %s", cam.strIpAddress.c_str());
			ImGui::TreeNodeEx((void*)(intptr_t)idx++, node_flags, text.c_str());
	
			ImGui::TreePop();
		}

		++camIdx;
	}
}
