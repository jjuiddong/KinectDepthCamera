
#include "stdafx.h"
#include "animationview.h"
#include "3dview.h"
#include "depthview.h"
#include "depthview2.h"


using namespace graphic;
using namespace framework;

cAnimationView::cAnimationView(const string &name)
	: framework::cDockWindow(name)
	, m_isFileAnimation(false)
	, m_selectFileList(0)
	, m_state(eState::NORMAL)
	, m_aniCameraCount(2)
	, m_isAutoSelectFileIndex(false)
{
}

cAnimationView::~cAnimationView()
{
}


bool cAnimationView::Init(graphic::cRenderer &renderer)
{
	//UpdateFileList();
	return true;
}


void cAnimationView::OnRender(const float deltaSeconds)
{
	ImGui::Checkbox("AutoMeasure", &g_root.m_isAutoMeasure);
	ImGui::SameLine();
	ImGui::Checkbox("Palete", &g_root.m_isPalete);
	ImGui::SameLine();
	ImGui::Checkbox("Grab-Log", &g_root.m_isGrabLog);

	if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if ((m_aniTime > 0.1f) && !m_files.empty() && g_root.m_baslerCam.IsReadyCapture())
		{
			const int masterIdx = g_root.m_masterSensor;
			bool result = false;

			for (u_int idx = 0; idx < m_files.size(); ++idx)
			{
				if (masterIdx == idx)
					continue;

				sFileInfo &finfo1 = m_files[masterIdx];
				sFileInfo &finfo2 = m_files[idx];
				if (finfo2.fileNames.empty())
					continue;

				cSensor *sensor1 = g_root.m_baslerCam.m_sensors[masterIdx];
				cSensor *sensor2 = g_root.m_baslerCam.m_sensors[idx];

				auto ret1 = GetOnlyTwoCameraAnimationIndex(finfo1, finfo1.aniIdx, finfo2, finfo2.aniIdx);
				finfo1.aniIdx = ret1.first;
				finfo2.aniIdx = ret1.second;

				if ((finfo2.aniIdx >= 0) && sensor2->m_isShow)
				{
					result |= sensor2->m_buffer.ReadDatFile(g_root.m_3dView->GetRenderer()
						, finfo2.fullFileNames[finfo2.aniIdx].ansi().c_str());
					++finfo2.aniIdx;
				}
				else
				{
					sensor2->m_buffer.m_isLoaded = false;
				}

				if (finfo2.fileNames.size() > (unsigned int)finfo2.aniIdx)
				{
					m_selFileIdx[idx] = finfo2.aniIdx;
					m_selectPath[idx] = finfo2.fileNames[finfo2.aniIdx];
				}
			}

			// master load pcd file
			sFileInfo &finfoMaster = m_files[masterIdx];
			cSensor *masterSensor = g_root.m_baslerCam.m_sensors[masterIdx];
			if ((finfoMaster.aniIdx >= 0) && masterSensor->m_isShow)
			{
				result |= masterSensor->m_buffer.ReadDatFile(g_root.m_3dView->GetRenderer()
					, finfoMaster.fullFileNames[finfoMaster.aniIdx].ansi().c_str());
				++finfoMaster.aniIdx;
			}
			else
			{
				masterSensor->m_buffer.m_isLoaded = false;
			}

			if (finfoMaster.fileNames.size() > (unsigned int)finfoMaster.aniIdx)
			{
				m_selFileIdx[masterIdx] = finfoMaster.aniIdx;
				m_selectPath[masterIdx] = finfoMaster.fileNames[finfoMaster.aniIdx];
			}

			if (result)
				UpdateDelayMeasure(m_aniTime);

			finfoMaster.aniIdx++; // master, select next file 
			m_aniTime = 0.f;
			for (auto &finfo : m_files)
			{
				if (finfo.fileNames.empty())
					continue;

				if ((u_int)finfo.aniIdx >= finfo.fileNames.size())
				{
					for (auto &f : m_files)
						f.aniIdx = 0;
					break;
				}
			}
		}
	}

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Text("Camera Count ");
	ImGui::SameLine(); ImGui::RadioButton("1", &m_aniCameraCount, 0);
	ImGui::SameLine(); ImGui::RadioButton("2", &m_aniCameraCount, 1);
	ImGui::SameLine(); ImGui::RadioButton("3", &m_aniCameraCount, 2);
	ImGui::SameLine(); ImGui::RadioButton("4", &m_aniCameraCount, 3);
	ImGui::SameLine(); ImGui::RadioButton("5", &m_aniCameraCount, 4);
	//ImGui::Spacing();

	ImGui::Text("Master Camera");
	ImGui::SameLine(); ImGui::RadioButton("Cam1", &g_root.m_masterSensor, 0);
	ImGui::SameLine(); ImGui::RadioButton("Cam2", &g_root.m_masterSensor, 1);
	ImGui::SameLine(); ImGui::RadioButton("Cam3", &g_root.m_masterSensor, 2);
	ImGui::SameLine(); ImGui::RadioButton("Cam4", &g_root.m_masterSensor, 3);
	ImGui::SameLine(); ImGui::RadioButton("Cam5", &g_root.m_masterSensor, 4);
	ImGui::Spacing();

	// File Animation
	if (m_isFileAnimation)
	{
		if (ImGui::Button("File Animation - Stop"))
		{
			m_isFileAnimation = false;
		}
	}
	else
	{
		if (ImGui::Button("File Animation - Play"))
		{
			m_isFileAnimation = true;
			g_root.m_baslerCam.CreateSensor(m_aniCameraCount + 1);
		}
	}

	ImGui::Separator();

	// Select Files
	for (u_int i=0; i < m_selectPath.size(); ++i)
	{
		auto &fileName = m_selectPath[i];
		ImGui::Text("Cam%d [%d] = %s", i, m_selFileIdx[i], fileName.c_str());
	}

	// Animation Index Slider
	if (m_files.size() > (u_int)g_root.m_masterSensor)
	{
		sFileInfo &finfo = m_files[g_root.m_masterSensor];

		if (ImGui::SliderInt("Master Ani Idx"
			, &finfo.aniIdx, 0, finfo.fileNames.size(), NULL))
		{
			if (!m_isFileAnimation)
			{
				OpenFileFromIndex(g_root.m_masterSensor, finfo.aniIdx, true);
			}
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

	if (!m_isFileAnimation)
		RenderFileList();
}


void cAnimationView::RenderFileList()
{
	if (g_root.m_baslerCam.IsReadyCapture())
	{
		ImGui::Text("File Select ");
	}

	ImGui::Checkbox("AutoSelect File", &m_isAutoSelectFileIndex);

	ImGui::PushID(10);
	ImGui::InputText("", g_root.m_inputFilePath.m_str, g_root.m_inputFilePath.SIZE);
	ImGui::PopID();
	ImGui::SameLine();
	if (ImGui::SmallButton("Read"))
		UpdateFileList();

	if (m_files.empty())
		return;

	const float x = ImGui::GetCursorPosX();
	const float y = ImGui::GetCursorPosY();
	const float w = (m_rect.Width() / m_files.size()) - x;

	for (u_int i=0; i < m_files.size(); ++i)
	{
		auto &files = m_files[i];
		const int selIdx = m_selFileIdx[i];

		ImGui::SetCursorPosX(x + (i*w));
		ImGui::SetCursorPosY(y);

		Str32 windowId;
		windowId.Format("Input File Window %d", i);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
		ImGui::BeginChild(windowId.c_str(), ImVec2(w, m_rect.Height() - ImGui::GetCursorPos().y - 40), true);

		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_FirstUseEver);
		if (ImGui::TreeNode((void*)i, "FileList"))
		{
			sFileInfo &finfo = files;
			for (u_int idx = 0; idx < finfo.fileNames.size(); ++idx)
			{
				auto &fileName = finfo.fileNames[idx];

				const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
					| ((idx == selIdx) ? ImGuiTreeNodeFlags_Selected : 0);

				ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
					fileName.c_str());

				if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
				{
					OpenFileFromIndex(i, idx, true);
				}

				ImGui::NextColumn();
			}

			ImGui::TreePop();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}
}


// 1초간 지연 후, 가장 작은 오차를 가진 정보로 볼륨을 측정한다.
void cAnimationView::UpdateDelayMeasure(const float deltaSeconds)
{
	bool isUpdateVolumeCalc = true;

	if (eState::DELAY_MEASURE1 == m_state)
	{
		if (m_measureCount > 1000)
		{
			CalcDelayMeasure();
			isUpdateVolumeCalc = false; // already show
		}
	}
	else if (eState::DELAY_MEASURE2 == m_state)
	{
		if (m_measureCount > 10)
		{
			CalcDelayMeasure();
			isUpdateVolumeCalc = false; // already show
		}
	}

	if (isUpdateVolumeCalc)
	{
		g_root.MeasureVolume();

		if ((eState::DELAY_MEASURE1 != m_state) && (eState::DELAY_MEASURE2 != m_state))
			return;

		// 측정값을 db에 저장한다.
		sMeasureResult result;
		result.id = g_root.m_measure.m_measureId;
		result.type = 2; // snap measure
		int id = 0;

		cMeasure &measure = g_root.m_measure;
		if (g_root.m_measure.m_boxes.size() == measure.m_contours.size())
		{
			for (u_int i = 0; i < measure.m_contours.size(); ++i)
			{
				auto &contour = measure.m_contours[i];
				auto &box = g_root.m_measure.m_boxes[i];

				const float l1 = std::max(box.volume.x, box.volume.z);
				const float l2 = std::min(box.volume.x, box.volume.z);
				const float l3 = box.volume.y;

				sMeasureVolume info;
				info.id = id++;
				info.horz = l1;
				info.vert = l2;
				info.height = l3;
				info.pos = box.pos;
				info.volume = box.minVolume;
				info.vw = box.minVolume / 6000.f;
				info.pointCount = box.pointCnt;
				info.contour = contour;
				result.volumes.push_back(info);
			}
		}

		if (!result.volumes.empty())
		{
			m_measureCount++;
			g_root.m_dbClient.Insert(result);
		}
	}
}


void cAnimationView::DelayMeasure()
{
	RET(!m_isFileAnimation);

	m_state = eState::DELAY_MEASURE1;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


void cAnimationView::DelayMeasure10()
{
	RET(!m_isFileAnimation);

	m_state = eState::DELAY_MEASURE2;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


void cAnimationView::CancelDelayMeasure()
{
	RET(!m_isFileAnimation);

	m_state = eState::NORMAL;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


// 1초간 정보를 받아서, 가장 적은 오차가 있는 정보로 계산한다.
void cAnimationView::CalcDelayMeasure()
{
	// 누적된 평균값을 저장한다.
	g_root.m_measure.m_boxesStored.clear();
	vector<sAvrContour> &avrContours = g_root.m_measure.m_avrContours;
	for (u_int i = 0; i < avrContours.size(); ++i)
	{
		const float distribCnt = (float)avrContours[i].count / (float)g_root.m_measure.m_calcAverageCount;
		if (distribCnt < 0.5f)
			continue;

		auto &box = avrContours[i].result;
		g_root.m_measure.m_boxesStored.push_back(box);
	}

	m_state = eState::NORMAL;
}


void cAnimationView::UpdateFileList()
{
	m_files.clear();
	m_selFileIdx.clear();
	m_selectPath.clear();

	vector<WStr32> exts;
	exts.reserve(16);
	exts.push_back(L"ply"); exts.push_back(L"PLY");
	exts.push_back(L"pcd"); exts.push_back(L"PCD");
	exts.push_back(L"ptx"); exts.push_back(L"PTX");

	//if (!g_root.m_baslerCam.IsReadyCapture())
	//	return;

	g_root.m_baslerCam.CreateSensor(m_aniCameraCount + 1);

	m_files.clear();
	m_files.reserve(std::max(g_root.m_baslerCam.m_sensors.size(), (size_t)3));

	for (auto *sensor : g_root.m_baslerCam.m_sensors)
	{
		vector<WStrPath> out;
		out.reserve(512);

		StrPath path;
		path.Format("%s/%d", g_root.m_inputFilePath.c_str(), sensor->m_id);
		common::CollectFiles(exts, path.wstr().c_str(), out);

		// 데이타 중복 복사를 피하기위한 꽁수
		m_files.push_back({});
		m_selFileIdx.push_back(0);
		m_selectPath.push_back("");
		sFileInfo &finfos = m_files.back();

		finfos.fullFileNames.reserve(out.size());
		for (auto &str : out)
			finfos.fullFileNames.push_back(str.utf8());

		for (auto &str : out)
			finfos.fileNames.push_back(str.utf8().GetFileName());

		// 각 0/1/2/... 폴더의 파일을 __int64 값으로 수치화해서 저장한다.
		// 애니메이션시 값을 비교해 가장 가까운 파일을 서로 연결해 애니메이션 한다.
		finfos.ids.reserve(out.size());
		for (auto &fileName : finfos.fileNames)
		{
			const __int64 num = ConvertFileNameToInt64(fileName);
			if (num > 0)
				finfos.ids.push_back(num);
		}
	}
}


// 파일명을 숫자로 리턴한다.
// ex = 2018-04-08-15-53-28-211.pcdz
// int64 = 20180408155328211
__int64 cAnimationView::ConvertFileNameToInt64(const common::StrPath &fileName)
{
	if ((fileName.size() != 27) && (fileName.size() != 28))
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
//int cAnimationView::GetTwoCameraAnimationIndex(int aniIdx1, int aniIdx2)
//{
//	RETV(aniIdx1 < 0, -1);
//	RETV(aniIdx2 < 0, -1);
//	RETV((int)m_fileIds.size() <= aniIdx1, -1);
//	RETV((int)m_secondFileIds.size() <= aniIdx2, -1);
//
//	while (m_fileIds[aniIdx1] > m_secondFileIds[aniIdx2])
//	{
//		++aniIdx2;
//
//		if ((int)m_secondFileIds.size() <= aniIdx2)
//			return -1;
//	}
//
//	return aniIdx2;
//}


// finfo1, aniIdx1에 해당하는 파일과 매칭되는 파일을 검색한다.
// 같은 시간대에 저장된 파일을 검색한다. 시간차가 크면 제외
// finfo1 을 기준으로 검색한다.
// aniIdx2 : -1 이면, aniIdx1을 중심으로 검색한다. 그렇지 않으면, aniIdx2를 중심으로 검색한다.
int cAnimationView::SearchMatchFile(const sFileInfo &finfo1, int aniIdx1
	, const sFileInfo &finfo2, int aniIdx2 )
{
	RETV(aniIdx1 < 0, -1);
	if (aniIdx2 < 0)
		aniIdx2 = 0;
	RETV((int)finfo1.ids.size() <= aniIdx1, -1);
	RETV((int)finfo2.ids.size() <= aniIdx2, -1);

	int oldIdx = -1;
	while (abs(finfo2.ids[aniIdx2] - finfo1.ids[aniIdx1]) > 60)
	{
		int nextIdx = 0;
		if (finfo1.ids[aniIdx1] > finfo2.ids[aniIdx2])
		{
			nextIdx = aniIdx2 + 1;
		}
		else
		{
			nextIdx = aniIdx2 - 1;
		}

		if ((int)finfo2.ids.size() <= (unsigned int)aniIdx2)
			return -1;

		// already check file, Not Exist matching file
		if (oldIdx == nextIdx)
			return -1;

		oldIdx = aniIdx2;
		aniIdx2 = nextIdx;
	}

	return aniIdx2;
}


// 2개의 카메라 정보가 있는 파일만 애니메이션 한다.
// 같은 시간대에 저장된 파일을 매칭한다. 시간차가 크면 제외
// finfo1 : master (기준 인덱스)
std::pair<int, int> cAnimationView::GetOnlyTwoCameraAnimationIndex(const sFileInfo &finfo1, int aniIdx1
	, const sFileInfo &finfo2, int aniIdx2
	, const bool isSkip //= false
)
{
	RETV(aniIdx1 < 0, std::make_pair(-1, -1));
	RETV(aniIdx2 < 0, std::make_pair(-1, -1));
	RETV((int)finfo1.ids.size() <= aniIdx1, std::make_pair(-1, -1));
	RETV((int)finfo2.ids.size() <= aniIdx2, std::make_pair(-1, -1));

	int oldIdx = -1;
	while (abs(finfo2.ids[aniIdx2] - finfo1.ids[aniIdx1]) > 60)
	{
		if (isSkip)
		{
			// 시간차가 크면, 스킵한다.
			aniIdx2 = -1;
			break;
		}

		int nextIdx = 0;
		if (finfo1.ids[aniIdx1] > finfo2.ids[aniIdx2])
		{
			nextIdx = aniIdx2 + 1;
		}
		else
		{
			nextIdx = aniIdx2 - 1;
		}

		if ((int)finfo1.ids.size() <= aniIdx1)
			return { -1,-1 };
		if ((int)finfo2.ids.size() <= aniIdx2)
			return { -1,-1 };

		// already check file, Not Exist matching file
		if (oldIdx == nextIdx)
		{
			// move master file to next
			oldIdx = -1;
			++aniIdx1;
		}
		else
		{
			oldIdx = aniIdx2;
			aniIdx2 = nextIdx;
		}
	}

	return { aniIdx1, aniIdx2 };
}


bool cAnimationView::OpenFile(const StrPath &ansifileName
	, const size_t camIdx //=0
)
{
	if (camIdx >= g_root.m_baslerCam.m_sensors.size())
		return false;

	cSensor *sensor1 = g_root.m_baslerCam.m_sensors[camIdx];

	if (string(".ply") == ansifileName.GetFileExt())
	{
		sensor1->m_buffer.ReadPlyFile(
			g_root.m_3dView->GetRenderer(), ansifileName.c_str());

		g_root.MeasureVolume();
	}
	else if ((string(".pcd") == ansifileName.GetFileExt())
		|| (string(".pcdz") == ansifileName.GetFileExt()))
	{
		sensor1->m_buffer.ReadDatFile(
			g_root.m_3dView->GetRenderer(), ansifileName.c_str());
		g_root.MeasureVolume();
	}
	else if (string(".ptx") == ansifileName.GetFileExt())
	{
		cPtxReader ptxReader;
		ptxReader.Read(ansifileName.c_str());

		cDatReader datReader;
		datReader.m_vertices = ptxReader.m_vertices;
		sensor1->m_buffer.ReadDatFile(
			g_root.m_3dView->GetRenderer(), datReader);
		g_root.MeasureVolume();
	}

	return true;
}


// Open pcd file, from camera, file index
bool cAnimationView::OpenFileFromIndex(const size_t camIdx, const int fileIdx
	, const bool isUpdateIternalValue //= false
)
{
	if (isUpdateIternalValue)
		m_selectFileList = camIdx;

	if (m_files.size() <= camIdx)
		return false;

	const sFileInfo &finfo = m_files[camIdx];
	if (finfo.fullFileNames.size() <= (unsigned int)fileIdx)
		return false;

	const common::StrPath ansifileName = finfo.fullFileNames[fileIdx].ansi();// change UTF8 -> UTF16
	OpenFile(ansifileName, camIdx);

	if (isUpdateIternalValue)
	{
		m_selFileIdx[camIdx] = fileIdx;
		m_selectPath[camIdx] = ansifileName;
	}

	if (m_isAutoSelectFileIndex)
	{
		const int camIdx2 = (camIdx == 2) ? 1 : 2;
		const sFileInfo &finfo2 = m_files[camIdx2];
		const int fileIdx2 = SearchMatchFile(finfo, fileIdx, finfo2, m_selFileIdx[camIdx2]);
		if (isUpdateIternalValue)
			m_selFileIdx[camIdx2] = fileIdx2;

		if (fileIdx2 >= 0)
		{
			common::StrPath ansifileName2 = finfo2.fullFileNames[m_selFileIdx[camIdx2]].ansi();// change UTF8 -> UTF16
			OpenFile(ansifileName2, camIdx2);

			if (isUpdateIternalValue)
				m_selectPath[camIdx2] = ansifileName2;
		}
		else
		{
			cSensor *sensor = g_root.m_baslerCam.m_sensors[camIdx2];
			if (sensor)
				sensor->m_buffer.m_isLoaded = false;

			if (isUpdateIternalValue)
				m_selectPath[camIdx2] = "";
		}
	}

	return true;
}


// Select Next File from keyboard event
void cAnimationView::NextFile(const int add)
{
	const int i = m_selectFileList;
	if ((int)m_files.size() <= i)
		return;

	OpenFileFromIndex(i, m_selFileIdx[i] + add, true);
}


void cAnimationView::OnEventProc(const sf::Event &evt)
{
	if (evt.type == sf::Event::KeyReleased)
	{
		switch (evt.key.code)
		{
		case sf::Keyboard::Key::Down:
			NextFile(1);
			break;

		case sf::Keyboard::Key::Up:
			NextFile(-1);
			break;
		}
	}
}

