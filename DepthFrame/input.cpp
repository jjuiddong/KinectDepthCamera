
#include "stdafx.h"
#include "input.h"
#include "plyreader.h"
#include "datreader.h"
#include "depthframe.h"
#include "3dview.h"
#include "depthview.h"
#include "depthview2.h"
#include "filterview.h"


using namespace graphic;
using namespace framework;

cInputView::cInputView(const string &name)
	: framework::cDockWindow(name)
	, m_isCaptureContinuos(false)
	, m_captureTime(false)
	, m_isFileAnimation(false)
	, m_aniIndex(0)
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

	ImGui::Text(g_root.m_baslerSetupSuccess? "BaslerCamera - Connect" : "BaslerCamera - Off");
	ImGui::SameLine();
	if (ImGui::Button("BaslerCamera Capture"))
		if (g_root.m_baslerSetupSuccess)
			g_root.BaslerCapture();

	if (m_isCaptureContinuos)
	{
		if (ImGui::Button("BaslerCamera Capture Continuous - Off"))
			m_isCaptureContinuos = false;
	}
	else
	{
		if (ImGui::Button("BaslerCamera Capture Continuous - On"))
			m_isCaptureContinuos = true;
	}

	ImGui::Checkbox("Basler Connect", &g_root.m_isConnectBasler);
	ImGui::SameLine();
	ImGui::Checkbox("AutoSave", &g_root.m_isAutoSaveCapture);
	ImGui::SameLine();
	ImGui::Checkbox("AutoMeasure", &g_root.m_isAutoMeasure);

	if (m_isCaptureContinuos && g_root.m_baslerSetupSuccess)
	{
		m_captureTime += deltaSeconds;
		if (m_captureTime > 0.1f)
		{
			m_captureTime = 0;
			g_root.BaslerCapture();
		}
	}
	else if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if (m_aniTime > .1f)
		{
			if (m_files.size() > (u_int)m_aniIndex)
			{
				g_root.m_sensorBuff.ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
					, m_files[m_aniIndex].ansi().c_str());

				if (g_root.m_isAutoMeasure)
				{
					g_root.m_sensorBuff.MeasureVolume(GetRenderer());
				}
			
				// Update FilterView, DepthView, DepthView2
				((cViewer*)g_application)->m_3dView->Capture3D();
				((cViewer*)g_application)->m_filterView->ProcessDepth();
				((cViewer*)g_application)->m_depthView->ProcessDepth();
				((cViewer*)g_application)->m_depthView2->ProcessDepth();
			}

		
			m_aniIndex++;
			m_aniTime = 0.f;
			if ((u_int)m_aniIndex >= m_files.size())
				m_aniIndex = 0;
		}
	}

	// File Animation
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (m_isFileAnimation)
	{
		if (ImGui::Button("File Animation - Stop"))
		{
			m_isFileAnimation = false;
			m_aniIndex = 0;
		}
		ImGui::Text("%s", m_files[m_aniIndex].c_str());
	}
	else
	{
		if (ImGui::Button("File Animation - Play"))
		{
			m_isFileAnimation = true;
			m_aniIndex = 0;
		}
	}


	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	static ImGuiTextFilter filter;
	filter.Draw("File Search");

	ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
	if (ImGui::TreeNode((void*)0, "Input FileList"))
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Refresh"))
			UpdateFileList();

		//ImGui::Separator();

		static int selectIdx = -1;
		int i = 0;
		bool isOpenPopup = false;

		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode((void*)1, "*.ply, *.pcd files"))
		{
			ImGui::Columns(1, "modelcolumns5", false);
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
						common::StrPath ansifileName = str.ansi();// change UTF8 -> UTF16
						m_selectPath = ansifileName;

						if (string(".ply") == ansifileName.GetFileExt())
						{
							g_root.m_sensorBuff.ReadPlyFile(
								((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());

							if (g_root.m_isAutoMeasure)
							{
								g_root.m_sensorBuff.MeasureVolume(GetRenderer());
							}

							// Update FilterView, DepthView, DepthView2
							((cViewer*)g_application)->m_3dView->Capture3D();
							((cViewer*)g_application)->m_filterView->ProcessDepth();
							((cViewer*)g_application)->m_depthView->ProcessDepth();
							((cViewer*)g_application)->m_depthView2->ProcessDepth();
						}
						else if (string(".pcd") == ansifileName.GetFileExt())
						{
							g_root.m_sensorBuff.ReadDatFile(
								((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());
						
							if (g_root.m_isAutoMeasure)
							{
								g_root.m_sensorBuff.MeasureVolume(GetRenderer());
							}

							// Update FilterView, DepthView, DepthView2
							((cViewer*)g_application)->m_3dView->Capture3D();
							((cViewer*)g_application)->m_filterView->ProcessDepth();
							((cViewer*)g_application)->m_depthView->ProcessDepth();
							((cViewer*)g_application)->m_depthView2->ProcessDepth();
						}

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
		exts.push_back(L"pcd"); exts.push_back(L"PCD");
		exts.push_back(L"pcd2"); exts.push_back(L"PCD2");

		vector<WStrPath> out;
		out.reserve(256);
		common::CollectFiles(exts, L"../media/Depth7", out);

		m_files.reserve(256);
		for (auto &str : out)
			m_files.push_back(str.utf8());
	}
}
