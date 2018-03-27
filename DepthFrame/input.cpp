
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
	, m_selFileIdx(-1)
	, m_state(eState::NORMAL)
	, m_measureTime(0)
	, m_minDifference(FLT_MAX)
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

	ImGui::Checkbox("Basler Connect", &g_root.m_isConnectBasler);
	ImGui::SameLine();
	ImGui::Checkbox("Kinect Connect", &g_root.m_isConnectKinect);
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

	ImGui::Spacing();
	ImGui::Checkbox("AutoSave", &g_root.m_isAutoSaveCapture);
	ImGui::SameLine();
	ImGui::Checkbox("AutoMeasure", &g_root.m_isAutoMeasure);
	ImGui::SameLine();
	ImGui::Checkbox("Palete", &g_root.m_isPalete);

	if (m_isCaptureContinuos && g_root.m_baslerSetupSuccess)
	{
		m_captureTime += deltaSeconds;
		if (m_captureTime > 0.1f)
		{
			m_captureTime = 0;
			g_root.BaslerCapture();

			if (eState::DELAY_MEASURE == m_state)
			{
				m_measureTime += deltaSeconds;
				if (m_measureTime > 1.f)
				{
					CalcDelayMeasure();
				}
				else
				{
					StoreMinimumDifferenceSensorBuffer();
				}
			}

			if (g_root.m_isAutoMeasure)
			{
				g_root.m_sensorBuff.MeasureVolume(GetRenderer());
			}

			// Update FilterView, DepthView, DepthView2
			((cViewer*)g_application)->m_depthView->ProcessDepth();
			((cViewer*)g_application)->m_depthView2->ProcessDepth();
			((cViewer*)g_application)->m_3dView->Capture3D();
			((cViewer*)g_application)->m_filterView->ProcessDepth();
		}
	}
	else if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if (m_aniTime > 0.1f)
		{
			if (m_files.size() > (u_int)m_aniIndex)
			{
				g_root.m_sensorBuff.ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
					, m_files[m_aniIndex].ansi().c_str());

				if (eState::DELAY_MEASURE == m_state)
				{
					m_measureTime += deltaSeconds;
					if (m_measureTime > 1.f)
					{
						CalcDelayMeasure();
					}
					else
					{
						StoreMinimumDifferenceSensorBuffer();
					}
				}

				if (g_root.m_isAutoMeasure)
				{
					g_root.m_sensorBuff.MeasureVolume(GetRenderer());
				}

				// Update FilterView, DepthView, DepthView2
				((cViewer*)g_application)->m_depthView->ProcessDepth();
				((cViewer*)g_application)->m_depthView2->ProcessDepth();
				((cViewer*)g_application)->m_3dView->Capture3D();
				((cViewer*)g_application)->m_filterView->ProcessDepth();
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
		}
		ImGui::Text("%s", m_files[m_aniIndex].c_str());
	}
	else
	{
		if (ImGui::Button("File Animation - Play"))
		{
			m_isFileAnimation = true;
		}
	}


	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::PushID(10);
	ImGui::InputText("", g_root.m_inputFilePath.m_str, g_root.m_inputFilePath.SIZE);
	ImGui::PopID();
	ImGui::SameLine();
	if (ImGui::SmallButton("Read"))
		UpdateFileList();

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Input File Window", ImVec2(0, m_rect.Height() - ImGui::GetCursorPos().y - 40), true);

	ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
	if (ImGui::TreeNode((void*)0, "Input FileList"))
	{
		//static int selectIdx = -1;
		int i = 0;
		bool isOpenPopup = false;

		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode((void*)1, "*.ply, *.pcd files"))
		{
			ImGui::Columns(1, "modelcolumns5", false);
			int cnt = 0;
			for (auto &fileName : m_files2)
			{
				++cnt;
				//if (cnt > 1)
				//	break;

				const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
					| ((i == m_selFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

				ImGui::TreeNodeEx((void*)(intptr_t)i, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
					fileName.c_str());

				if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
				{
					m_selFileIdx = i;
					common::StrPath ansifileName = m_files[i].ansi();// change UTF8 -> UTF16
					m_selectPath = ansifileName;

					OpenFile(ansifileName);

					// Popup Menu
					if (ImGui::IsItemClicked(1))
					{
						isOpenPopup = true;
					}
				}

				ImGui::NextColumn();

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

	ImGui::EndChild();
	ImGui::PopStyleVar();
}


// 데이타 변화율이 적은 정보를 저장한다.
// 1초가 지난 후, 계산된다.
void cInputView::StoreMinimumDifferenceSensorBuffer()
{
	if (m_minDifference > g_root.m_sensorBuff.m_diffAvrs.GetCurValue())
	{
		if (g_root.m_sensorBuff.m_vertices.size() != m_vertices.size())
		{
			m_vertices.resize(g_root.m_sensorBuff.m_vertices.size());
			m_depthBuff.resize(g_root.m_sensorBuff.m_depthBuff.size());
			m_depthBuff2.resize(g_root.m_sensorBuff.m_depthBuff2.size());
		}

		memcpy(&m_vertices[0], &g_root.m_sensorBuff.m_vertices[0], m_vertices.size() * sizeof(m_vertices[0]));
		memcpy(&m_depthBuff[0], &g_root.m_sensorBuff.m_depthBuff[0], m_depthBuff.size() * sizeof(m_depthBuff[0]));
		memcpy(&m_depthBuff2[0], &g_root.m_sensorBuff.m_depthBuff2[0], m_depthBuff2.size() * sizeof(m_depthBuff2[0]));

		m_minDifference = g_root.m_sensorBuff.m_diffAvrs.GetCurValue();
	}
}


void cInputView::DelayMeasure()
{
	RET(!m_isCaptureContinuos && !m_isFileAnimation);

	m_state = eState::DELAY_MEASURE;
	m_measureTime = 0;
	m_minDifference = FLT_MAX;
	g_root.m_boxesStored.clear();
}


// 1초간 정보를 받아서, 가장 적은 오차가 있는 정보로 계산한다.
void cInputView::CalcDelayMeasure()
{
	memcpy(&g_root.m_sensorBuff.m_vertices[0], &m_vertices[0], m_vertices.size() * sizeof(m_vertices[0]));
	memcpy(&g_root.m_sensorBuff.m_depthBuff[0], &m_depthBuff[0], m_depthBuff.size() * sizeof(m_depthBuff[0]));
	memcpy(&g_root.m_sensorBuff.m_depthBuff2[0], &m_depthBuff2[0], m_depthBuff2.size() * sizeof(m_depthBuff2[0]));

	g_root.m_sensorBuff.MeasureVolume(GetRenderer());

	// Update FilterView, DepthView, DepthView2
	((cViewer*)g_application)->m_3dView->Capture3D();
	((cViewer*)g_application)->m_filterView->ProcessDepth();
	((cViewer*)g_application)->m_depthView->ProcessDepth();
	((cViewer*)g_application)->m_depthView2->ProcessDepth();
	
	// 측정된 정보를 따로 저장한다.
	g_root.m_boxesStored = g_root.m_boxes;

	m_state = eState::NORMAL;
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
		//common::CollectFiles(exts, L"../media/Depth", out); // test
		//common::CollectFiles(exts, L"../media/Depth6", out); // sun day
		//common::CollectFiles(exts, L"../media/Depth4", out); // satur day
		//common::CollectFiles(exts, L"../media/Depth2", out); // thurs day
		//common::CollectFiles(exts, L"../media/Depth8", out); // 2018-03-26 data
		common::CollectFiles(exts, g_root.m_inputFilePath.wstr().c_str(), out); // 2018-03-26 data		

		m_files.reserve(256);
		for (auto &str : out)
			m_files.push_back(str.utf8());

		m_files2.clear();
		m_files2.reserve(256);
		for (auto &str : out)
			m_files2.push_back(str.utf8().GetFileName());
	}
}


bool cInputView::OpenFile(const StrPath &ansifileName)
{
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

	return true;
}


void cInputView::OnEventProc(const sf::Event &evt)
{
	if (evt.type == sf::Event::KeyReleased)
	{
		switch (evt.key.code)
		{
		case sf::Keyboard::Key::Down:
			++m_selFileIdx;
			if ((int)m_files.size() <= m_selFileIdx)
				m_selFileIdx = (int)m_files.size() - 1;

			if (m_selFileIdx >= 0)
				OpenFile(m_files[m_selFileIdx].ansi());
			break;

		case sf::Keyboard::Key::Up:
			if (m_selFileIdx < 0)
				break;

			--m_selFileIdx;
			if (m_selFileIdx < 0)
				m_selFileIdx = m_files.empty()? -1 : 0;

			if (m_selFileIdx >= 0)
				OpenFile(m_files[m_selFileIdx].ansi());
			break;
		}
	}
}

