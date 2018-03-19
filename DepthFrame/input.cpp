
#include "stdafx.h"
#include "input.h"
#include "plyreader.h"


using namespace graphic;
using namespace framework;

cInputView::cInputView(const string &name)
	: framework::cDockWindow(name)
{
}

cInputView::~cInputView()
{
}


bool cInputView::Init(graphic::cRenderer &renderer)
{
	UpdateFileList();

	return true;
}


void cInputView::OnRender(const float deltaSeconds)
{
	ImGui::Text("Input Type");
	ImGui::SameLine();
	ImGui::RadioButton("File", (int*)&g_root.m_input, cRoot::eInputType::FILE);
	ImGui::SameLine();
	ImGui::RadioButton("Kinect", (int*)&g_root.m_input, cRoot::eInputType::KINECT);
	ImGui::SameLine();
	ImGui::RadioButton("Basler", (int*)&g_root.m_input, cRoot::eInputType::BASLER);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	static ImGuiTextFilter filter;
	filter.Draw("File Search");

	ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
	if (ImGui::TreeNode((void*)0, "Input FileList"))
	{
		ImGui::SameLine(150);
		if (ImGui::SmallButton("Refresh"))
			UpdateFileList();

		//ImGui::Separator();

		static int selectIdx = -1;
		int i = 0;
		bool isOpenPopup = false;

		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode((void*)1, "*.ply files"))
		{
			ImGui::Columns(5, "modelcolumns5", false);
			for (auto &str : m_files)
			{
				const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
					| ((i == selectIdx) ? ImGuiTreeNodeFlags_Selected : 0);

				StrPath fileName = str.GetFileName();
				if (filter.PassFilter(fileName.c_str()))
				{
					ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
						fileName.c_str());

					if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
					{
						selectIdx = i;
						m_selectPath = str.ansi(); // change UTF8 -> UTF16
						//g_root.m_resViewer->LoadResource(m_selectPath.c_str());

						cPlyReader plyReader;
						plyReader.Read(str.ansi().c_str());

						//cResourceManager::Get()->LoadRawMesh(str.ansi().c_str());

						// Popup Menu
						if (ImGui::IsItemClicked(1))
						{
							isOpenPopup = true;
						}
					}

					ImGui::NextColumn();
				}

				++i;
			}
			ImGui::TreePop();
		}

		////---------------------------------------------------------------------------
		//// Popup Menu
		//if (isOpenPopup)
		//	ImGui::OpenPopup("select");

		//ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
		//if (ImGui::BeginPopup("select"))
		//{
		//	if (ImGui::Selectable("Add Model      "))
		//		g_root.m_hierarchyWindow->AddModel();
		//	ImGui::EndPopup();
		//}
		//ImGui::PopStyleColor();
		////---------------------------------------------------------------------------

		ImGui::TreePop();
	}
}


void cInputView::UpdateFileList()
{
	{
		m_files.clear();

		vector<WStr32> exts;
		exts.reserve(16);
		exts.push_back(L"ply"); exts.push_back(L"PLY");
		vector<WStrPath> out;
		out.reserve(256);
		common::CollectFiles(exts, L"../media/", out);

		m_files.reserve(256);
		for (auto &str : out)
			m_files.push_back(str.utf8());
	}
}
