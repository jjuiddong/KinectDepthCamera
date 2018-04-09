
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
	, m_captureTime(0)
	, m_triggerDelayTime(0)
	, m_isFileAnimation(false)
	, m_aniIndex(-1)
	, m_aniIndex2(-1)
	, m_selFileIdx(-1)
	, m_state(eState::NORMAL)
	, m_measureTime(0)
	, m_minDifference(0)
	, m_comboFileIdx(0)
	, m_isReadTwoCamera(false)
	, m_isReadCamera2(false)
{
}

cInputView::~cInputView()
{
}


bool cInputView::Init(graphic::cRenderer &renderer)
{
	//UpdateFileList();
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
		ImGui::Text(g_root.m_kinect.IsConnect() ? "Kinect Camera - Connect" : "Kinect Camera - Off");
		ImGui::Checkbox("Kinect Connect Auto", &g_root.m_isConnectKinect);

		if (ImGui::Button("Capture Kinect Camera"))
		{
			if (g_root.m_kinect.IsConnect())
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
		ImGui::Text(g_root.m_balserCam.IsConnect() ? "Basler Camera - Connect" : "Basler Camera - Off");
		ImGui::Checkbox("Basler Connect Auto", &g_root.m_isTryConnectBasler);

		if (ImGui::Button("Capture Balser Camera"))
			if (g_root.m_balserCam.IsConnect())
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

		if (ImGui::Button("Trigger Delay"))
		{
			g_root.m_balserCam.setTriggerDelays();
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

	if (m_isCaptureContinuos && (g_root.m_balserCam.IsConnect() || g_root.m_kinect.IsConnect()))
	{
		m_captureTime += deltaSeconds;
		m_triggerDelayTime += deltaSeconds;
		if (m_captureTime > 0.1f)
		{
			if (g_root.BaslerCapture())
				UpdateDelayMeasure(m_captureTime);
			else
				m_measureTime += m_captureTime;

			m_captureTime = 0;
		}
		if (m_triggerDelayTime > 1.f)
		{
			g_root.m_balserCam.setTriggerDelays();
			m_triggerDelayTime = 0;
		}

	}
	else if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if (m_aniTime > 0.1f)
		{
			if (m_files.size() > (u_int)m_aniIndex)
			{
				if (m_isReadTwoCamera)
				{
					m_aniIndex2 = GetTwoCameraAnimationIndex(m_aniIndex, m_aniIndex2);

					if (m_aniIndex2 >= 0)
					{
						const bool r1 = g_root.m_sensorBuff[0].ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
							, m_files[m_aniIndex].ansi().c_str());

						
						bool r2 = false;
						if (m_secondFileIds[m_aniIndex2] - m_fileIds[m_aniIndex] < 60)
						{
							r2 = g_root.m_sensorBuff[1].ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
								, m_secondFiles[m_aniIndex2].ansi().c_str());
						}
						else
						{
							g_root.m_sensorBuff[1].m_isLoaded = false;
						}

						if (r1 || r2)
							UpdateDelayMeasure(m_aniTime);
						else
							m_measureTime += m_captureTime;
					}
				}
				else
				{
					if (g_root.m_sensorBuff[0].ReadDatFile(((cViewer*)g_application)->m_3dView->GetRenderer()
						, m_files[m_aniIndex].ansi().c_str()))
						UpdateDelayMeasure(m_aniTime);
					else
						m_measureTime += m_captureTime;
				}

			}
		
			m_aniIndex++;
			m_aniTime = 0.f;
			if ((u_int)m_aniIndex >= m_files.size())
			{
				m_aniIndex = 0;
				m_aniIndex2 = 0;
			}

			if (m_isReadTwoCamera)
			{
				if (m_aniIndex2 < 0)
				{
					m_aniIndex = 0;
					m_aniIndex2 = 0;
				}
			}
		}
	}

	// File Animation
	if (m_isFileAnimation)
	{
		if (ImGui::Button("File Animation - Stop"))
		{
			m_isFileAnimation = false;
		}
		if ((u_int)(m_aniIndex-1) < m_files.size())
			ImGui::Text("%s", m_files[m_aniIndex-1].c_str());
		if (m_isReadTwoCamera && ((u_int)m_aniIndex2 < m_secondFiles.size()))
			ImGui::Text("%s", m_secondFiles[m_aniIndex2].c_str());
	}
	else
	{
		if (ImGui::Button("File Animation - Play"))
		{
			m_isFileAnimation = true;
			//m_aniIndex = 0;
			//m_aniIndex2 = 0;
		}

		if ((u_int)(m_aniIndex - 1) < m_files.size())
			ImGui::Text("%s", m_files[m_aniIndex - 1].c_str());
		if (m_isReadTwoCamera && ((u_int)m_aniIndex2 < m_secondFiles.size()))
			ImGui::Text("%s", m_secondFiles[m_aniIndex2].c_str());
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
	ImGui::Checkbox("Read Two Camera", &m_isReadTwoCamera);
	ImGui::SameLine();
	ImGui::Checkbox("Read Camera2", &m_isReadCamera2);

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

				OpenFile(ansifileName, m_isReadCamera2? 1 : 0);

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
void cInputView::StoreMinimumDifferenceSensorBuffer(
	const size_t camIdx //=0 
	)
{
	if (m_minDifference > g_root.m_sensorBuff[camIdx].m_diffAvrs.GetCurValue())
	{
		if (g_root.m_sensorBuff[camIdx].m_vertices.size() != m_vertices.size())
		{
			m_vertices.resize(g_root.m_sensorBuff[camIdx].m_vertices.size());
			m_depthBuff.resize(g_root.m_sensorBuff[camIdx].m_depthBuff.size());
			m_depthBuff2.resize(g_root.m_sensorBuff[camIdx].m_depthBuff2.size());
		}

		memcpy(&m_vertices[0], &g_root.m_sensorBuff[camIdx].m_vertices[0], m_vertices.size() * sizeof(m_vertices[0]));
		memcpy(&m_depthBuff[0], &g_root.m_sensorBuff[camIdx].m_depthBuff[0], m_depthBuff.size() * sizeof(m_depthBuff[0]));
		memcpy(&m_depthBuff2[0], &g_root.m_sensorBuff[camIdx].m_depthBuff2[0], m_depthBuff2.size() * sizeof(m_depthBuff2[0]));

		m_minDifference = g_root.m_sensorBuff[camIdx].m_diffAvrs.GetCurValue();
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
void cInputView::CalcDelayMeasure(const size_t camIdx // =0
)
{
	memcpy(&g_root.m_sensorBuff[camIdx].m_vertices[0], &m_vertices[0], m_vertices.size() * sizeof(m_vertices[0]));
	memcpy(&g_root.m_sensorBuff[camIdx].m_depthBuff[0], &m_depthBuff[0], m_depthBuff.size() * sizeof(m_depthBuff[0]));
	memcpy(&g_root.m_sensorBuff[camIdx].m_depthBuff2[0], &m_depthBuff2[0], m_depthBuff2.size() * sizeof(m_depthBuff2[0]));

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
	out.reserve(512);

	if (m_isReadTwoCamera)
	{
		// two camera는 0,1 경로에 따로 로딩한다.
		common::CollectFiles(exts, (g_root.m_inputFilePath + "/0").wstr().c_str(), out);

		vector<WStrPath> out2;
		out2.reserve(512);
		common::CollectFiles(exts, (g_root.m_inputFilePath + "/1").wstr().c_str(), out2);

		m_secondFiles.clear();
		m_secondFiles.reserve(512);
		for (auto &str : out2)
			m_secondFiles.push_back(str.utf8());
	}
	else
	{
		common::CollectFiles(exts, g_root.m_inputFilePath.wstr().c_str(), out);
	}

	m_files.reserve(out.size());
	for (auto &str : out)
		m_files.push_back(str.utf8());

	m_files2.clear();
	m_files2.reserve(out.size());
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


	// 2대의 카메라를 애니메이션 한다면, 각 0/1 폴더의 파일을 __int64 값으로 수치화해서 저장한다.
	// 애니메이션시 값을 비교해 가장 가까운 파일을 서로 연결해 애니메이션 한다.
	if (m_isReadTwoCamera)
	{
		m_fileIds.clear();
		m_fileIds.reserve(m_files2.size());
		for (auto &fileName : m_files2)
		{
			const __int64 num = ConvertFileNameToInt64(fileName);
			if (num > 0)
				m_fileIds.push_back(num);
		}

		m_secondFileIds.clear();
		m_secondFileIds.reserve(m_secondFiles.size());
		for (auto &fileName : m_secondFiles)
		{
			const __int64 num = ConvertFileNameToInt64(fileName.GetFileName());
			if (num > 0)
				m_secondFileIds.push_back(num);
		}
	}

	m_filePages = pages + 1;
	m_comboFileIdx = 0;
	m_aniIndex = -1;
	m_aniIndex2 = -1;
}


// 파일명을 숫자로 리턴한다.
// ex = 2018-04-08-15-53-28-211.pcd
// int64 = 20180408155328211
__int64 cInputView::ConvertFileNameToInt64(const common::StrPath &fileName)
{
	if (fileName.size() != 27)
		return 0;

	char buff[32];
	ZeroMemory(buff, sizeof(buff));

	// ex = 2018-04-08-15-53-28-211.pcd
	buff[0] = fileName.m_str[0]; // 2
	buff[1] = fileName.m_str[1]; // 0
	buff[2] = fileName.m_str[2]; // 1
	buff[3] = fileName.m_str[3]; // 8
	buff[4] = fileName.m_str[5]; // 0
	buff[5] = fileName.m_str[6]; // 4
	buff[6] = fileName.m_str[8]; // 0
	buff[7] = fileName.m_str[9]; // 8
	buff[8] = fileName.m_str[11]; // 1
	buff[9] = fileName.m_str[12]; // 5
	buff[10] = fileName.m_str[14]; // 5
	buff[11] = fileName.m_str[15]; // 3
	buff[12] = fileName.m_str[17]; // 2
	buff[13] = fileName.m_str[18]; // 8
	buff[14] = fileName.m_str[20]; // 2
	buff[15] = fileName.m_str[21]; // 1
	buff[16] = fileName.m_str[22]; // 1

	const __int64 val = _atoi64(buff);
	return val;
}


// 2개의 카메라 애니메이션을 할 때, 첫 번째 카메라 애니메이션 파일을 기준으로, 
// 두 번째 카메라 애니메이션 파일을 선택한다.
// 이 때, 파일명이 시간을 나타내므로, 가장 가까운 시간에 있는 파일을 선택해
// 인덱스값을 리턴한다.
int cInputView::GetTwoCameraAnimationIndex(int aniIdx1, int aniIdx2)
{
	RETV(aniIdx1 < 0, -1);
	RETV(aniIdx2 < 0, -1);
	RETV((int)m_fileIds.size() <= aniIdx1, -1);
	RETV((int)m_secondFileIds.size() <= aniIdx1, -1);
	RETV((int)m_secondFileIds.size() <= aniIdx2, -1);
	RETV((int)m_fileIds.size() <= aniIdx2, -1);

	while (m_fileIds[aniIdx1] > m_secondFileIds[aniIdx2])
	{
		++aniIdx2;

		if ((int)m_fileIds.size() <= aniIdx2)
			return -1;
		if ((int)m_secondFileIds.size() <= aniIdx2)
			return -1;
	}

	return aniIdx2;
}


bool cInputView::OpenFile(const StrPath &ansifileName
	, const size_t camIdx //=0
)
{
	if (string(".ply") == ansifileName.GetFileExt())
	{
		g_root.m_sensorBuff[camIdx].ReadPlyFile(
			((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());

		g_root.MeasureVolume();
	}
	else if (string(".pcd") == ansifileName.GetFileExt())
	{
		g_root.m_sensorBuff[camIdx].ReadDatFile(
			((cViewer*)g_application)->m_3dView->GetRenderer(), ansifileName.c_str());

		g_root.MeasureVolume();
	}

	return true;
}


void cInputView::OnEventProc(const sf::Event &evt)
{
	if (evt.type == sf::Event::KeyReleased)
	{
		return;

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

