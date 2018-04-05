
#include "stdafx.h"
#include "inputview.h"
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
	, m_minDifference(0)
	, m_comboFileIdx(0)
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

	switch (g_root.m_input)
	{
	case cRoot::eInputType::FILE:
		break;

	case cRoot::eInputType::KINECT:
	{
		ImGui::Text(g_root.m_kinectSetupSuccess ? "Kinect Camera - Connect" : "Kinect Camera - Off");
		ImGui::Checkbox("Kinect Connect Auto", &g_root.m_isConnectKinect);

		if (ImGui::Button("Capture Kinect Camera"))
		{
			if (g_root.m_kinectSetupSuccess)
				g_root.KinectCapture();
		}

		if (m_isCaptureContinuos)
		{
			if (ImGui::Button("Capture Continuous Kinect Camera - Off"))
				m_isCaptureContinuos = false;
		}
		else
		{
			if (ImGui::Button("Capture Continuous Kinect Camera - On"))
				m_isCaptureContinuos = true;
		}
	}
	break;

	case cRoot::eInputType::BASLER:
	{
		ImGui::Text(g_root.m_baslerSetupSuccess ? "Basler Camera - Connect" : "Basler Camera - Off");
		ImGui::Checkbox("Basler Connect Auto", &g_root.m_isConnectBasler);

		if (ImGui::Button("Capture Balser Camera"))
			if (g_root.m_baslerSetupSuccess)
				g_root.BaslerCapture();

		if (m_isCaptureContinuos)
		{
			if (ImGui::Button("Capture Continuous Basler Camera - Off"))
				m_isCaptureContinuos = false;
		}
		else
		{
			if (ImGui::Button("Capture Continuous Basler Camera - On"))
				m_isCaptureContinuos = true;
		}
	}
	break;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Checkbox("AutoSave", &g_root.m_isAutoSaveCapture);
	ImGui::SameLine();
	ImGui::Checkbox("AutoMeasure", &g_root.m_isAutoMeasure);
	ImGui::SameLine();
	ImGui::Checkbox("Palete", &g_root.m_isPalete);

	if (m_isCaptureContinuos && (g_root.m_baslerSetupSuccess || g_root.m_kinectSetupSuccess))
	{
		m_captureTime += deltaSeconds;
		if (m_captureTime > 0.1f)
		{
			if (g_root.BaslerCapture())
				UpdateDelayMeasure(m_captureTime);
			else
				m_measureTime += m_captureTime;

			m_captureTime = 0;
		}
	}
	else if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if (m_aniTime > 0.1f)
		{
			if (m_files.size() > (u_int)m_aniIndex)
			{
				if (g_root.m_sensorBuff.ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
					, m_files[m_aniIndex].ansi().c_str()))
					UpdateDelayMeasure(m_aniTime);
				else
					m_measureTime += m_captureTime;
			}
		
			m_aniIndex++;
			m_aniTime = 0.f;
			if ((u_int)m_aniIndex >= m_files.size())
				m_aniIndex = 0;
		}
	}

	// File Animation
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

	RenderFileList();

	ImGui::EndChild();
	ImGui::PopStyleVar();
}


void cInputView::RenderFileList()
{
	ImGui::PushID(10);
	ImGui::InputText("", g_root.m_inputFilePath.m_str, g_root.m_inputFilePath.SIZE);
	ImGui::PopID();
	ImGui::SameLine();
	if (ImGui::SmallButton("Read"))
		UpdateFileList();

	ImGui::Combo("Page", &m_comboFileIdx, m_comboFileStr.c_str());

	ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
	ImGui::BeginChild("Input File Window", ImVec2(0, m_rect.Height() - ImGui::GetCursorPos().y - 40), true);

	ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
	if (ImGui::TreeNode((void*)0, "FileList"))
	{
		int i = 0;
		bool isOpenPopup = false;

		//ImGui::Columns(5, "modelcolumns5", false);
		//for (auto &fileName : m_files2)
		//for (u_int i = MAX_FILEPAGE*m_comboFileIdx; )

		const u_int fileSize = m_files.size();
		const u_int maxSize = (m_comboFileIdx == (m_filePages-1))? fileSize : MAX_FILEPAGE;

		for (u_int i = 0; i < maxSize; ++i)
		{
			const u_int idx = MAX_FILEPAGE * m_comboFileIdx + i;
			if (fileSize <= idx)
				break;

			auto &fileName = m_files2[idx];

			const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
				| ((idx == m_selFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

			ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
				fileName.c_str());

			if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
			{
				m_selFileIdx = idx;
				common::StrPath ansifileName = m_files[idx].ansi();// change UTF8 -> UTF16
				m_selectPath = ansifileName;

				OpenFile(ansifileName);

				// Popup Menu
				if (ImGui::IsItemClicked(1))
				{
					isOpenPopup = true;
				}
			}

			ImGui::NextColumn();
		}

		ImGui::TreePop();
	}
}


// 1초간 지연 후, 가장 작은 오차를 가진 정보로 볼륨을 측정한다.
void cInputView::UpdateDelayMeasure(const float deltaSeconds)
{
	bool isUpdateVolumeCalc = true;

	if (eState::DELAY_MEASURE == m_state)
	{
		m_measureTime += deltaSeconds;
		if (m_measureTime >= 1.f)
		{
			CalcDelayMeasure();
			isUpdateVolumeCalc = false; // already show
		}
		else
		{
			StoreMinimumDifferenceSensorBuffer();
		}
	}

	if (isUpdateVolumeCalc)
	{
		g_root.MeasureVolume();
	}
}


// 데이타 변화율이 적은 정보를 저장한다.
// 1 초가 지난 후, 계산된다.
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

	g_root.MeasureVolume();
	
	// 측정된 정보를 따로 저장한다.
	g_root.m_boxesStored = g_root.m_boxes;

	m_state = eState::NORMAL;
}


void cInputView::UpdateFileList()
{
	m_files.clear();

	vector<WStr32> exts;
	exts.reserve(16);
	exts.push_back(L"ply"); exts.push_back(L"PLY");
	exts.push_back(L"pcd"); exts.push_back(L"PCD");
	exts.push_back(L"pcd2"); exts.push_back(L"PCD2");

	vector<WStrPath> out;
	out.reserve(256);
	common::CollectFiles(exts, g_root.m_inputFilePath.wstr().c_str(), out);

	m_files.reserve(256);
	for (auto &str : out)
		m_files.push_back(str.utf8());

	m_files2.clear();
	m_files2.reserve(256);
	for (auto &str : out)
		m_files2.push_back(str.utf8().GetFileName());

	// Page Combo Box를 생성한다.
	int cnt = 0;
	int pages = 0;
	m_comboFileStr.clear();
	for (u_int i = 0; i < m_files.size() / MAX_FILEPAGE; ++i)
	{
		char buff[4] = { NULL, };
		sprintf(buff, "%3d", i + 1);

		if (((u_int)cnt + 10) > m_comboFileStr.SIZE)
		{
			m_comboFileStr.m_str[cnt++] = ' ';
			m_comboFileStr.m_str[cnt++] = '.';
			m_comboFileStr.m_str[cnt++] = '.';
			m_comboFileStr.m_str[cnt++] = '.';
			break;
		}
		else
		{
			++pages;
			for (u_int k = 0; k < strlen(buff); ++k)
				m_comboFileStr.m_str[cnt++] = buff[k];
			m_comboFileStr.m_str[cnt++] = '\0';
		}
	}

	if (pages <= 0)
	{
		m_comboFileStr.m_str[cnt++] = ' ';
		m_comboFileStr.m_str[cnt++] = '.';
		m_comboFileStr.m_str[cnt++] = '.';
		m_comboFileStr.m_str[cnt++] = '.';
	}

	m_filePages = pages + 1;
	m_comboFileIdx = 0;
}


bool cInputView::OpenFile(const StrPath &ansifileName)
{
	if (string(".ply") == ansifileName.GetFileExt())
	{
		g_root.m_sensorBuff.ReadPlyFile(
			((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());

		g_root.MeasureVolume();
	}
	else if (string(".pcd") == ansifileName.GetFileExt())
	{
		g_root.m_sensorBuff.ReadDatFile(
			((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());

		g_root.MeasureVolume();
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

