
#include "stdafx.h"
#include "inputview.h"
#include "3dview.h"
#include "depthview.h"
#include "depthview2.h"


using namespace graphic;
using namespace framework;

cInputView::cInputView(const string &name)
	: framework::cDockWindow(name)
	, m_isCaptureContinuos(false)
	, m_captureTime(0)
	, m_isFileAnimation(false)
	, m_aniIndex1(-1)
	, m_aniIndex2(-1)
	, m_aniIndex3(-1)
	, m_selFileIdx(-1)
	, m_state(eState::NORMAL)
	, m_measureTime(0)
	, m_comboFileIdx(0)
	, m_aniCameraCount(2)
	, m_explorerFolderIndex(2)
	, m_isAutoSelectFileIndex(false)
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
		//ImGui::Text(g_root.m_kinect.IsConnect() ? "Kinect Camera - Connect" : "Kinect Camera - Off");
		//ImGui::Checkbox("Kinect Connect Auto", &g_root.m_isConnectKinect);

		//if (ImGui::Button("Capture Kinect Camera"))
		//{
		//	if (g_root.m_kinect.IsConnect())
		//		g_root.KinectCapture();
		//}

		//if (m_isCaptureContinuos)
		//{
		//	if (ImGui::Button("Capture Continuous Kinect Camera - Off"))
		//		m_isCaptureContinuos = false;
		//}
		//else
		//{
		//	if (ImGui::Button("Capture Continuous Kinect Camera - On"))
		//		m_isCaptureContinuos = true;
		//}
	}
	break;

	case cRoot::eInputType::BASLER:
	{
		Str64 stateText;
		switch (g_root.m_baslerCam.m_state)
		{
		case cBaslerCameraSync::eThreadState::CONNECT_TRY: stateText = "Basler Camera - Try Connect"; break;
		case cBaslerCameraSync::eThreadState::CONNECT_CONFIG: stateText = "Basler Camera - Configuration"; break;
		case cBaslerCameraSync::eThreadState::CAPTURE: stateText = "Basler Camera - Connect"; break;
		default: stateText = "Basler Camera - Off"; break;
		}

		ImGui::Text(stateText.c_str());// g_root.m_baslerCam.IsConnect() ? "Basler Camera - Connect" : "Basler Camera - Off");
		ImGui::Checkbox("Basler Connect Auto", &g_root.m_isTryConnectBasler);

		if (ImGui::Button("Capture Balser Camera"))
			if (g_root.m_baslerCam.IsReadyCapture())
				g_root.m_baslerCam.CopyCaptureBuffer(g_root.m_3dView->GetRenderer());

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

		if (ImGui::Button("Basler Trigger"))
			g_root.m_baslerCam.m_isTrySyncTrigger = true;
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
	ImGui::Checkbox("Grab-Log", &g_root.m_isGrabLog);
	ImGui::SameLine();
	ImGui::Checkbox("Grab-ErrLog", &g_root.m_isGrabErrLog);

	if (m_isCaptureContinuos && g_root.m_baslerCam.IsReadyCapture())
	{
		m_captureTime += deltaSeconds;
		if (m_captureTime > 0.25f)
		{
			if (g_root.m_baslerCam.CopyCaptureBuffer(g_root.m_3dView->GetRenderer()))
				UpdateDelayMeasure(m_captureTime);
			else
				m_measureTime += m_captureTime;

			m_captureTime = 0;
		}
	}
	else if (m_isFileAnimation)
	{
		m_aniTime += deltaSeconds;
		if ((m_aniTime > 0.1f) && !m_files.empty() && g_root.m_baslerCam.IsReadyCapture())
		{
			sFileInfo &finfo1 = m_files[0];
			sFileInfo &finfo2 = m_files[1];
			sFileInfo &finfo3 = m_files[2]; // master
			cSensor *sensor1 = g_root.m_baslerCam.m_sensors[0];
			cSensor *sensor2 = g_root.m_baslerCam.m_sensors[1];
			cSensor *sensor3 = g_root.m_baslerCam.m_sensors[2]; // master

			if (finfo3.fileNames.size() > (u_int)m_aniIndex3)
			{
				auto ret1 = GetOnlyTwoCameraAnimationIndex(finfo3, m_aniIndex3, finfo2, m_aniIndex2);
				m_aniIndex3 = ret1.first;
				m_aniIndex2 = ret1.second;

				auto ret2 = GetOnlyTwoCameraAnimationIndex(finfo3, m_aniIndex3, finfo1, m_aniIndex1, true);
				m_aniIndex1 = ret2.second;

				bool r1 = false;
				if ((m_aniIndex1 >= 0) && sensor1->m_isShow)
				{
					r1 = sensor1->m_buffer.ReadDatFile(g_root.m_3dView->GetRenderer()
						, finfo1.fullFileNames[m_aniIndex1].ansi().c_str());
					++m_aniIndex1;
				}
				else
				{
					sensor1->m_buffer.m_isLoaded = false;
				}

				bool r2 = false;
				if ((m_aniIndex2 >= 0) && sensor2->m_isShow)
				{
					r2 = sensor2->m_buffer.ReadDatFile(g_root.m_3dView->GetRenderer()
					, finfo2.fullFileNames[m_aniIndex2].ansi().c_str());
				}
				else
				{
					sensor2->m_buffer.m_isLoaded = false;
				}

				bool r3 = false;
				if ((m_aniIndex3 >= 0) && sensor3->m_isShow)
				{
					r3 = sensor3->m_buffer.ReadDatFile(g_root.m_3dView->GetRenderer()
						, finfo3.fullFileNames[m_aniIndex3].ansi().c_str());
				}
				else
				{
					sensor3->m_buffer.m_isLoaded = false;
				}


				if (r1 || r2 || r3)
					UpdateDelayMeasure(m_aniTime);
				else
					m_measureTime += m_captureTime;

			}
		
			m_aniIndex3++;
			m_aniTime = 0.f;
			if ((u_int)m_aniIndex3 >= finfo3.fileNames.size())
			{
				m_aniIndex1 = 0;
				m_aniIndex2 = 0;
				m_aniIndex3 = 0;
			}

			if (m_aniIndex2 < 0)
			{
				m_aniIndex1 = 0;
				m_aniIndex2 = 0;
				m_aniIndex3 = 0;
			}

			if (m_aniIndex1 < 0)
			{
				m_aniIndex1 = 0;
			}

		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	
	ImGui::Text("Camera Count ");
	ImGui::SameLine(); ImGui::RadioButton("Cam1", &m_aniCameraCount, 0);
	ImGui::SameLine(); ImGui::RadioButton("Cam2", &m_aniCameraCount, 1);
	ImGui::SameLine(); ImGui::RadioButton("Cam3", &m_aniCameraCount, 2);
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

	//ImGui::SameLine();
	//ImGui::Checkbox("Ani Only 2 Camera", &m_isOnlyTwoCameraFile);

	//if ((u_int)(m_aniIndex - 1) < m_files.size())
	//	ImGui::Text("%s", m_files[m_aniIndex - 1].c_str());
	//if (m_isReadTwoCamera && ((u_int)m_aniIndex2 < m_secondFiles.size()))
	//	ImGui::Text("%s", m_secondFiles[m_aniIndex2].c_str());

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderFileList();

	ImGui::EndChild();
	ImGui::PopStyleVar();
}


void cInputView::RenderFileList()
{
	if (g_root.m_baslerCam.IsReadyCapture())
	{
		ImGui::Text("Folder Select ");

		for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
		{
			Str32 text;
			text.Format("./%d/", sensor->m_id);
			ImGui::SameLine();
			ImGui::RadioButton(text.c_str(), &m_explorerFolderIndex, sensor->m_id);
		}
	}

	ImGui::Checkbox("AutoSelect File", &m_isAutoSelectFileIndex);

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
		
		if (!m_files.empty())
		{
			sFileInfo &finfo = m_files[m_explorerFolderIndex];

			const u_int fileSize = finfo.fileNames.size();
			const u_int maxSize = (m_comboFileIdx == (m_filePages-1))? fileSize : MAX_FILEPAGE;

			for (u_int i = 0; i < maxSize; ++i)
			{
				const u_int idx = MAX_FILEPAGE * m_comboFileIdx + i;
				if (fileSize <= idx)
					break;

				auto &fileName = finfo.fileNames[idx];

				const ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
					| ((idx == m_selFileIdx) ? ImGuiTreeNodeFlags_Selected : 0);

				ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
					fileName.c_str());

				if (ImGui::IsItemClicked() || ImGui::IsItemClicked(1))
				{
					m_selFileIdx = idx;
					common::StrPath ansifileName = finfo.fullFileNames[idx].ansi();// change UTF8 -> UTF16
					m_selectPath = ansifileName;

					OpenFile(ansifileName, m_explorerFolderIndex);

					if (m_isAutoSelectFileIndex)
					{
						const int fidx = (m_explorerFolderIndex == 2) ? 1 : 2;
						sFileInfo &finfo2 = m_files[fidx];
						if (finfo2.fullFileNames.size() > idx)
						{
							common::StrPath ansifileName2 = finfo2.fullFileNames[idx].ansi();// change UTF8 -> UTF16
							OpenFile(ansifileName2, fidx);
						}
					}

					// Popup Menu
					if (ImGui::IsItemClicked(1))
					{
						isOpenPopup = true;
					}
				}

				ImGui::NextColumn();
			}
		}

		ImGui::TreePop();
	}
}


// 1초간 지연 후, 가장 작은 오차를 가진 정보로 볼륨을 측정한다.
void cInputView::UpdateDelayMeasure(const float deltaSeconds)
{
	bool isUpdateVolumeCalc = true;

	if (eState::DELAY_MEASURE1 == m_state)
	{
		//m_measureTime += deltaSeconds;
		//if (m_measureTime >= 3.f)
		if (m_measureCount > 200)
		{
			CalcDelayMeasure();
			isUpdateVolumeCalc = false; // already show
		}
	}
	else if (eState::DELAY_MEASURE2 == m_state)
	{
		if (m_measureCount > 20)
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
		//if (g_root.m_measure.m_boxes.size() == measure.m_contours.size())
		{
			for (u_int i=0; i < measure.m_boxes.size(); ++i)
			{
				//auto &contour = measure.m_contours[i];
				auto &box = g_root.m_measure.m_boxes[i];

				const float l1 = std::max(box.volume.x, box.volume.z);
				const float l2 = std::min(box.volume.x, box.volume.z);
				const float l3 = box.volume.y;

				sMeasureVolume info;
				info.integral = box.integral;
				info.id = id++;
				info.horz = l1;
				info.vert = l2;
				info.height = l3;
				info.pos = box.pos;
				info.volume = box.minVolume;
				info.vw = box.minVolume / 6000.f;
				info.pointCount = box.pointCnt;
				//info.contour = contour;
				result.volumes.push_back(info);
			}
		}

		if (!result.volumes.empty())
		{
			m_measureCount++;
			g_root.m_dbClient.Insert(result);
			g_root.m_measure.m_results.push_back(result);
		}
	}
}


void cInputView::DelayMeasure()
{
	RET(!m_isCaptureContinuos && !m_isFileAnimation);

	m_state = eState::DELAY_MEASURE1;
	m_measureTime = 0;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


void cInputView::DelayMeasure10()
{
	RET(!m_isCaptureContinuos && !m_isFileAnimation);

	m_state = eState::DELAY_MEASURE2;
	m_measureTime = 0;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


void cInputView::CancelDelayMeasure()
{
	RET(!m_isCaptureContinuos && !m_isFileAnimation);

	m_state = eState::NORMAL;
	m_measureTime = 0;
	m_measureCount = 0;
	g_root.m_measure.m_boxesStored.clear();
}


// 일정한 횟수만큼 정보를 저장한 후, 많이 인식된 정보를 저장한다.
void cInputView::CalcDelayMeasure()
{
	// 누적된 평균값을 저장한다.
	g_root.m_measure.m_boxesStored.clear();
	vector<sAvrContour> &avrContours = g_root.m_measure.m_avrContours;
	for (u_int i = 0; i < avrContours.size(); ++i)
	{
		// 인식된 횟수가 적으면, 무시한다.
		const float distribCnt = (float)avrContours[i].count / (float)g_root.m_measure.m_calcAverageCount;
		if (distribCnt < 0.5f)
			continue;

		auto &box = avrContours[i].result;
		g_root.m_measure.m_boxesStored.push_back(box);
	}
	//

	//CalcDelayMeasureRefine();

	m_state = eState::NORMAL;

	if (cMeasure::INTEGRAL == g_root.m_measureType)
	{
		g_root.m_dbClient.WriteExcel(true);
	}
}


// 전체 저장된 측정치에서 평균, 표준편차를 구한 후,
// 표준편차에서 멀어진 측정치는 제외한 평균값을 구해서 결과를 낸다.
void cInputView::CalcDelayMeasureRefine()
{
	// Calc Average
	vector<sMeasureVolume> avrVolumes;
	for (auto &result : g_root.m_measure.m_results)
	{
		for (u_int i = 0; i < result.volumes.size(); ++i)
		{
			if (avrVolumes.size() <= i)
			{
				sMeasureVolume vol;
				vol.id = i;
				vol.horz = 0.f;
				vol.vert = 0.f;
				vol.height = 0.f;
				vol.volume = 0.f;
				vol.vw = 0.f;
				vol.pointCount = 0;
				avrVolumes.push_back(vol);
			}

			sMeasureVolume &vol = result.volumes[i];
			sMeasureVolume &avr = avrVolumes[i];
			avr.horz += vol.horz;
			avr.vert += vol.vert;
			avr.height += vol.height;
			avr.vw += vol.vw;
			avr.pointCount++;
		}
	}

	for (auto &avr : avrVolumes)
	{
		avr.horz /= (float)avr.pointCount;
		avr.vert /= (float)avr.pointCount;
		avr.height /= (float)avr.pointCount;
		avr.vw /= (float)avr.pointCount;
	}

	//------------------------------------------------------
	// Calc Standard Deviation
	vector<sMeasureVolume> sdVolumes;
	for (auto &result : g_root.m_measure.m_results)
	{
		for (u_int i = 0; i < result.volumes.size(); ++i)
		{
			if (sdVolumes.size() <= i)
			{
				sMeasureVolume vol;
				vol.id = i;
				vol.horz = 0.f;
				vol.vert = 0.f;
				vol.height = 0.f;
				vol.volume = 0.f;
				vol.vw = 0.f;
				vol.pointCount = 0;
				sdVolumes.push_back(vol);
			}

			const sMeasureVolume &vol = result.volumes[i];
			const sMeasureVolume &avr = avrVolumes[i];
			sMeasureVolume &sd = sdVolumes[i];

			sd.horz += (vol.horz - avr.horz) * (vol.horz - avr.horz);
			sd.vert += (vol.vert - avr.vert) * (vol.vert - avr.vert);
			sd.height += (vol.height - avr.height) * (vol.height - avr.height);
			sd.vw += (vol.vw - avr.vw) * (vol.vw - avr.vw);
			sd.pointCount++;
		}
	}

	for (auto &sd : sdVolumes)
	{
		if (sd.pointCount > 1)
		{
			sd.horz = sqrt(sd.horz / (float)(sd.pointCount - 1));
			sd.vert = sqrt(sd.vert / (float)(sd.pointCount - 1));
			sd.height = sqrt(sd.height / (float)(sd.pointCount - 1));
			sd.vw = sqrt(sd.vw / (float)(sd.pointCount - 1));
		}
	}


	//-------------------------------------------------------------
	// 표준편차에서 벗어난 데이타를 제외하고 다시 구한다.
	vector<sMeasureVolume> finalVolumes;
	for (auto &result : g_root.m_measure.m_results)
	{
		for (u_int i = 0; i < result.volumes.size(); ++i)
		{
			if (finalVolumes.size() <= i)
			{
				sMeasureVolume vol;
				vol.id = i;
				vol.horz = 0.f;
				vol.vert = 0.f;
				vol.height = 0.f;
				vol.volume = 0.f;
				vol.vw = 0.f;
				vol.pointCount = 0;
				finalVolumes.push_back(vol);
			}

			const sMeasureVolume &vol = result.volumes[i];
			const sMeasureVolume &avr = avrVolumes[i];
			const sMeasureVolume &sd = sdVolumes[i];
			sMeasureVolume &fin = finalVolumes[i];

			if ((abs(vol.horz - avr.horz) < sd.horz)
				&& (abs(vol.vert - avr.vert) < sd.vert)
				&& (abs(vol.height - avr.height) < sd.height)
				)
			{
				fin.horz += vol.horz;
				fin.vert += vol.vert;
				fin.height += vol.height;
				fin.vw += vol.vw;
				fin.pointCount++;
			}
		}
	}

	for (auto &fin : finalVolumes)
	{
		fin.horz /= (float)fin.pointCount;
		fin.vert /= (float)fin.pointCount;
		fin.height /= (float)fin.pointCount;
		fin.vw /= (float)fin.pointCount;
	}

	vector<sAvrContour> &avrContours = g_root.m_measure.m_avrContours;
	for (u_int i = 0; i < finalVolumes.size(); ++i)
	{
		// 인식된 횟수가 적으면, 무시한다.
		const float distribCnt = (float)finalVolumes[i].pointCount / (float)g_root.m_measure.m_calcAverageCount;
		if (distribCnt < 0.5f)
			continue;

		if (g_root.m_measure.m_boxesStored.size() <= i)
			continue;

		const auto &fin = finalVolumes[i];
		auto &box = g_root.m_measure.m_boxesStored[i];
		box.minVolume = fin.vw * 6000.f;
		box.maxVolume = fin.vw * 6000.f;
		box.volume = Vector3(fin.vert, fin.horz, fin.height);
	}

}


void cInputView::UpdateFileList()
{
	m_files.clear();

	vector<WStr32> exts;
	exts.reserve(16);
	exts.push_back(L"ply"); exts.push_back(L"PLY");
	exts.push_back(L"pcd"); exts.push_back(L"PCD");
	//exts.push_back(L"pcd2"); exts.push_back(L"PCD2");
	//exts.push_back(L"pcdz"); exts.push_back(L"PCDZ");

	if (g_root.m_baslerCam.IsReadyCapture())
	{
		g_root.m_baslerCam.CreateSensor(m_aniCameraCount + 1);

		m_files.reserve( std::max(g_root.m_baslerCam.m_sensors.size(), (size_t)3) );

		for (auto *sensor : g_root.m_baslerCam.m_sensors)
		{
			vector<WStrPath> out;
			out.reserve(512);
			
			StrPath path;
			path.Format("%s/%d", g_root.m_inputFilePath.c_str(), sensor->m_id);
			common::CollectFiles(exts, path.wstr().c_str(), out);

			// 데이타 중복 복사를 피하기위한 꽁수
			m_files.push_back({});
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

	//if (m_isReadTwoCamera)
	//{
	//	// two camera는 0,1 경로에 따로 로딩한다.
	//	common::CollectFiles(exts, (g_root.m_inputFilePath + "/0").wstr().c_str(), out);

	//	vector<WStrPath> out2;
	//	out2.reserve(512);
	//	common::CollectFiles(exts, (g_root.m_inputFilePath + "/1").wstr().c_str(), out2);

	//	m_secondFiles.clear();
	//	m_secondFiles.reserve(512);
	//	for (auto &str : out2)
	//		m_secondFiles.push_back(str.utf8());
	//}
	//else
	//{
	//	common::CollectFiles(exts, g_root.m_inputFilePath.wstr().c_str(), out);
	//}

	//m_files.reserve(out.size());
	//for (auto &str : out)
	//	m_files.push_back(str.utf8());

	//m_files2.clear();
	//m_files2.reserve(out.size());
	//for (auto &str : out)
	//	m_files2.push_back(str.utf8().GetFileName());

	// Page Combo Box를 생성한다.
	int cnt = 0;
	int pages = 0;
	if (!m_files.empty())
	{
		m_comboFileStr.clear();
		sFileInfo &finfo = m_files[2];

		for (u_int i = 0; i < finfo.fullFileNames.size() / MAX_FILEPAGE; ++i)
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
	//if (m_isReadTwoCamera)
	//{
	//	m_fileIds.clear();
	//	m_fileIds.reserve(m_files2.size());
	//	for (auto &fileName : m_files2)
	//	{
	//		const __int64 num = ConvertFileNameToInt64(fileName);
	//		if (num > 0)
	//			m_fileIds.push_back(num);
	//	}

	//	m_secondFileIds.clear();
	//	m_secondFileIds.reserve(m_secondFiles.size());
	//	for (auto &fileName : m_secondFiles)
	//	{
	//		const __int64 num = ConvertFileNameToInt64(fileName.GetFileName());
	//		if (num > 0)
	//			m_secondFileIds.push_back(num);
	//	}
	//}

	m_filePages = pages + 1;
	m_comboFileIdx = 0;
	m_aniIndex1 = -1;
	m_aniIndex2 = -1;
	m_aniIndex3 = -1;
	//m_aniIndex2 = 8000;
	//m_aniIndex3 = 8000;
}


// 파일명을 숫자로 리턴한다.
// ex = 2018-04-08-15-53-28-211.pcdz
// int64 = 20180408155328211
__int64 cInputView::ConvertFileNameToInt64(const common::StrPath &fileName)
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
//int cInputView::GetTwoCameraAnimationIndex(int aniIdx1, int aniIdx2)
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


// 2개의 카메라 정보가 있는 파일만 애니메이션 한다.
std::pair<int, int> cInputView::GetOnlyTwoCameraAnimationIndex(const sFileInfo &finfo1, int aniIdx1
	, const sFileInfo &finfo2, int aniIdx2
	, const bool isSkip //= false
)
{
	RETV(aniIdx1 < 0, std::make_pair(-1, -1) );
	RETV(aniIdx2 < 0, std::make_pair(-1, -1));
	RETV((int)finfo1.ids.size() <= aniIdx1, std::make_pair(-1, -1));
	RETV((int)finfo2.ids.size() <= aniIdx2, std::make_pair(-1, -1));

	while (abs(finfo2.ids[aniIdx2] - finfo1.ids[aniIdx1]) > 60)
	{
		if (isSkip)
		{
			// 시간차가 크면, 스킵한다.
			aniIdx2 = -1;
			break;
		}

		if (finfo1.ids[aniIdx1] > finfo2.ids[aniIdx2])
		{
			++aniIdx2;
		}
		else
		{
			++aniIdx1;
		}

		if ((int)finfo1.ids.size() <= aniIdx1)
			return { -1,-1 };
		if ((int)finfo2.ids.size() <= aniIdx2)
			return { -1,-1 };
	}

	return { aniIdx1, aniIdx2 };
}


bool cInputView::OpenFile(const StrPath &ansifileName
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

	return true;
}


void cInputView::OnEventProc(const sf::Event &evt)
{
	if (evt.type == sf::Event::KeyReleased)
	{
		switch (evt.key.code)
		{
		case sf::Keyboard::Key::Down:
		{
			if ((int)m_files.size() <= m_explorerFolderIndex)
				break;

			sFileInfo &finfo = m_files[m_explorerFolderIndex];

			++m_selFileIdx;
			if ((int)finfo.fullFileNames.size() <= m_selFileIdx)
				m_selFileIdx = (int)finfo.fullFileNames.size() - 1;

			if (m_selFileIdx >= 0)
				OpenFile(finfo.fullFileNames[m_selFileIdx].ansi(), m_explorerFolderIndex);

			if (m_isAutoSelectFileIndex)
			{
				const int fidx = (m_explorerFolderIndex == 2) ? 1 : 2;
				sFileInfo &finfo2 = m_files[fidx];
				if ((int)finfo2.fullFileNames.size() > m_selFileIdx)
				{
					common::StrPath ansifileName2 = finfo2.fullFileNames[m_selFileIdx].ansi();// change UTF8 -> UTF16
					OpenFile(ansifileName2, fidx);
				}
			}
		}
		break;

		case sf::Keyboard::Key::Up:
		{
			if ((int)m_files.size() <= m_explorerFolderIndex)
				break;

			sFileInfo &finfo = m_files[m_explorerFolderIndex];

			if (m_selFileIdx < 0)
				break;

			--m_selFileIdx;
			if (m_selFileIdx < 0)
				m_selFileIdx = finfo.fullFileNames.empty() ? -1 : 0;

			if (m_selFileIdx >= 0)
				OpenFile(finfo.fullFileNames[m_selFileIdx].ansi(), m_explorerFolderIndex);

			if (m_isAutoSelectFileIndex)
			{
				const int fidx = (m_explorerFolderIndex == 2) ? 1 : 2;
				sFileInfo &finfo2 = m_files[fidx];
				if ((int)finfo2.fullFileNames.size() > m_selFileIdx)
				{
					common::StrPath ansifileName2 = finfo2.fullFileNames[m_selFileIdx].ansi();// change UTF8 -> UTF16
					OpenFile(ansifileName2, fidx);
				}
			}
		}
		break;
		}
	}
}

