
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

	CalibrationGroundPlane();
	SingleSensorSubPlaneCalibration();
	MultiSensorGroundCalibration();
	HeightDistrubution();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	//--------------------------------------------------------------------------
	// offset edit
	ImGui::PushID(100);
	ImGui::Text("Config File Path");
	ImGui::SameLine();
	ImGui::InputText("", g_root.m_configFileName.m_str, g_root.m_configFileName.SIZE);
	ImGui::PopID();
	ImGui::SameLine();
	if (ImGui::Button("..."))
	{
		g_root.m_configFileName = OpenFileDialog();
	}

	if (ImGui::Button("Load Calibration Variable"))
	{
		g_root.LoadPlane();

		for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
		{
			sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
			sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Save Calibration Variable"))
	{
		g_root.SavePlane();
	}

	{
		ImGui::Text("Ground Plane");
		const bool update1 = ImGui::DragFloat4("Ground Plane", (float*)&g_root.m_groundPlane, 0.01f, -1000, 1000, "%.5f");
		if (update1)
			g_root.m_groundPlane.N.Normalize();
		const bool update2 = ImGui::DragFloat3("Ground Center", (float*)&g_root.m_volumeCenter, 0.01f);
		ImGui::Separator();
		ImGui::Spacing();

		if (update1 || update2)
		{
			for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
			{
				sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
				sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
			}
		}
	}

	ImGui::Text("Camera Offset - SubPlane");
	for (u_int i = 0; i < g_root.m_baslerCam.m_sensors.size(); ++i)
	{
		cSensor *sensor = g_root.m_baslerCam.m_sensors[i];
		Str128 text;
		text.Format("%d-Position Offset", i);
		const bool update1 = ImGui::DragFloat3(text.c_str(), (float*)&sensor->m_buffer.m_offset.pos, 0.01f);
		text.Format("%d-RotateY", i);
		const bool update2 = ImGui::DragFloat(text.c_str(), &g_root.m_cameraOffsetYAngle[i], 0.01f);
		text.Format("%d-SubPlane", i);
		const bool update3 = ImGui::DragFloat4(text.c_str(), (float*)&g_root.m_planeSub[i], 0.01f, -1000, 1000, "%.5f");
		if (update1)
		{
			g_root.m_cameraOffset[i].pos = sensor->m_buffer.m_offset.pos;
		}
		if (update3)
		{
			g_root.m_planeSub[i].N.Normalize();
			sensor->m_buffer.m_planeSub = g_root.m_planeSub[i];
		}

		if (update1 || update2 || update3)
		{
			Quaternion q;
			q.SetRotationY(g_root.m_cameraOffsetYAngle[i]);
			sensor->m_buffer.m_offset.rot = q;
			sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
			sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
		}

		ImGui::Separator();
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
				sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
			g_root.MeasureVolume();
		}
	}

	ImGui::DragFloat3("range min", (float*)&g_root.m_cullRangeMin, 1);
	ImGui::DragFloat3("range max", (float*)&g_root.m_cullRangeMax, 1);
	ImGui::Spacing();
	ImGui::Spacing();
}


void cCalibrationView::CalibrationGroundPlane()
{
	if (ImGui::CollapsingHeader("Ground Plane Calibration"))
	{
		if (ImGui::Button("Pick 3-Point"))
		{
			g_root.m_3dView->m_genPlane = 0;
		}

		ImGui::SameLine();
		if (ImGui::Button("Pick Center Point"))
		{
			g_root.m_3dView->m_state = c3DView::eState::VCENTER;
		}

		ImGui::SameLine();
		if (ImGui::Button("Change Space"))
		{
			for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
			{
				sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
				sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
			}
		}

		ImGui::Spacing();
		const bool update1 = ImGui::DragFloat4("Ground Plane", (float*)&g_root.m_groundPlane, 0.01f, -1000, 1000, "%.5f");
		if (update1)
			g_root.m_groundPlane.N.Normalize();
		const bool update2 = ImGui::DragFloat3("Ground Center", (float*)&g_root.m_volumeCenter, 0.01f);
		ImGui::Spacing();

		if (update1 || update2)
		{
			for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
			{
				sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
				sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
			}
		}
	}
}


void cCalibrationView::SingleSensorSubPlaneCalibration()
{
	if (ImGui::CollapsingHeader("Single Sensor SubPlane Calibration"))
	{
		static int sensorIdx = 0;
		for (u_int i = 0; i < g_root.m_baslerCam.m_sensors.size(); ++i)
		{
			auto &sensor = g_root.m_baslerCam.m_sensors[i];
			Str32 text;
			text.Format("cam%d", i);
			if (i != 0)
				ImGui::SameLine();
			ImGui::RadioButton(text.c_str(), &sensorIdx, i);
		}

		if (!g_root.m_baslerCam.m_sensors.empty())
		{
			ImGui::SameLine();
			if (ImGui::Button("Pick Calibration Pos"))
			{
				g_root.m_3dView->m_state = c3DView::eState::REGION;
			}
		}
		
		ImGui::Spacing();

		ImGui::DragFloat3("Region Center", (float*)&g_root.m_regionCenter, 0.01f);
		ImGui::DragFloat2("Region Size", (float*)&g_root.m_regionSize, 0.1f);
		ImGui::Checkbox("Continuous Calibration", &g_root.m_isContinuousCalibrationPlane);

		static bool isCalcCalibration = false;
		cCalibration &calib = g_root.m_calib;

		// SubPlane Calibration
		if (ImGui::Button("Ground Calibration"))
		{
			if (CalibrationSubPlane(sensorIdx, g_root.m_regionCenter, g_root.m_regionSize))
			{
				isCalcCalibration = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Calc Ground Standard Deviation"))
		{
			g_root.m_planeStandardDeviation = CalcBasePlaneStandardDeviation();
		}

		if (g_root.m_planeStandardDeviation != 0)
		{
			ImGui::Text("Standard Deviation = %f", g_root.m_planeStandardDeviation);
		}

		if (isCalcCalibration)
		{
			ImGui::Text("current sd = %f", calib.m_result.curSD);
			ImGui::Text("calibration sd = %f", calib.m_result.minSD);
			ImGui::Text("plane x=%f, y=%f, z=%f, d=%f"
				, calib.m_result.plane.N.x, calib.m_result.plane.N.y, calib.m_result.plane.N.z, calib.m_result.plane.D);

			if (g_root.m_isContinuousCalibrationPlane)
				ImGui::Text("avr plane x=%f, y=%f, z=%f, d=%f"
					, calib.m_avrPlane.N.x, calib.m_avrPlane.N.y, calib.m_avrPlane.N.z, calib.m_avrPlane.D);

			if (ImGui::Button("Clear Base Plane Calibration"))
			{
				dbg::Logp("Clear Base Plane Calibration \n");
				calib.Clear();
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0, 0, 255));
			ImGui::SameLine();
			if (ImGui::Button("Update Ground Plane"))
			{
				// 기존 SubPlane 초기화
				g_root.m_baslerCam.m_sensors[sensorIdx]->m_buffer.m_planeSub = Plane(Vector3(0,1,0),0);
				g_root.m_baslerCam.m_sensors[sensorIdx]->m_buffer.m_offset.pos.y = 0;
				g_root.m_cameraOffset[sensorIdx].pos.y = 0;
				g_root.m_planeSub[sensorIdx] = Plane(Vector3(0, 1, 0), 0);

				g_root.GeneratePlane(calib.m_result.pos);

				for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
				{
					sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
					sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
				}
			}

			ImGui::PopStyleColor(3);
		}
	}
}


bool cCalibrationView::CalibrationSubPlane(const int sensorIdx
	, const Vector3 &regionCenter, const Vector2 &regionSize)
{
	if (g_root.m_baslerCam.m_sensors.size() <= (u_int)sensorIdx)
		return false;

	cCalibration &calib = g_root.m_calib;
	auto &sensor = g_root.m_baslerCam.m_sensors[sensorIdx];

	// sub plane을 초기화 한 상태에서 컬리브레이션 해야한다.
	sensor->m_buffer.m_planeSub = Plane(Vector3(0, 1, 0), 0);
	sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
	calib.CalibrationBasePlane(regionCenter, regionSize, sensor);

	// update sub plane and reload point cloud
	sensor->m_buffer.m_planeSub = calib.m_result.plane;
	sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
	sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());

	dbg::Logp("calib plane xyzd, %f, %f, %f, %f\n"
		, calib.m_result.plane.N.x, calib.m_result.plane.N.y, calib.m_result.plane.N.z, calib.m_result.plane.D);

	// update config variable
	g_root.m_config.SetValue("calib-center-x", regionCenter.x);
	g_root.m_config.SetValue("calib-center-y", regionCenter.y);
	g_root.m_config.SetValue("calib-center-z", regionCenter.z);
	g_root.m_config.SetValue("calib-minmax-x", regionSize.x);
	g_root.m_config.SetValue("calib-minmax-y", regionSize.y);

	// save calibration variable
	g_root.m_planeSub[sensorIdx] = calib.m_result.plane;
	g_root.SavePlane();

	return true;
}


void cCalibrationView::MultiSensorGroundCalibration()
{
	if (ImGui::CollapsingHeader("Multi Sensor Ground Calibration"))
	{
		cCalibration &calib = g_root.m_calib;

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
			cCalibration::sResult result;
			m_isCalc = calib.CalibrationBasePlane(fileName.c_str(), result);
		}

		if (m_isCalc)
		{
			ImGui::Spacing();

			cCalibration::sResult &result = calib.m_result;
			ImGui::Text("current sd = %f", result.curSD);
			ImGui::Text("calibration sd = %f", result.minSD);
			ImGui::Text("plane x=%f, y=%f, z=%f, d=%f"
				, result.plane.N.x, result.plane.N.y, result.plane.N.z, result.plane.D);
		}
	}
}


// 높이 분포맵을 구한다.
void cCalibrationView::HeightDistrubution()
{
	static float calcOffsetY = 0.f;

	if (ImGui::CollapsingHeader("Height Distribution"))
	{
		if (ImGui::Button("Set Region"))
		{
			g_root.m_3dView->m_state = c3DView::eState::HDISTRIB;
		}

		if (cSensor *sensor = g_root.GetFirstVisibleSensor())
		{
			Str64 text;
			text.Format("Calc Height Distribution - Cam%d", sensor->m_id);
			ImGui::SameLine();
			if (ImGui::Button(text.c_str()))
			{
				cCalibration &calib = g_root.m_calib;
				calcOffsetY = calib.CalcHeightDistribute(g_root.m_hdistribCenter
					, g_root.m_hdistribSize, sensor);
			}
		}

		ImGui::DragFloat3("Region Center", (float*)&g_root.m_hdistribCenter, 0.1f);
		ImGui::DragFloat2("Region Size", (float*)&g_root.m_hdistribSize, 0.1f);

		if (cSensor *sensor = g_root.GetFirstVisibleSensor())
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0, 0, 255));

			Str64 text;
			text.Format("Update Ground Zero Y (%.3f) - Cam%d", calcOffsetY, sensor->m_id);
			if (ImGui::Button(text.c_str()))
			{
				sensor->m_buffer.m_offset.pos.y -= calcOffsetY;
				g_root.m_cameraOffset[sensor->m_id].pos.y -= calcOffsetY;

				sensor->m_buffer.UpdatePointCloudAllConfig(g_root.m_3dView->GetRenderer());
				sensor->m_buffer.UpdatePointCloudBySelf(g_root.m_3dView->GetRenderer());
			}

			ImGui::PopStyleColor(3);
		}
	}
}


// 바닥 높이의 표준편차를 구한다.
double cCalibrationView::CalcBasePlaneStandardDeviation(
	const size_t camIdx //=0
)
{
	if (camIdx >= g_root.m_baslerCam.m_sensors.size())
		return 0.f;

	cSensor *sensor1 = g_root.m_baslerCam.m_sensors[camIdx];

	const float xLimitLower = -40.f;
	const float xLimitUpper = 45.f;
	const float zLimitLower = -42.5f;
	const float zLimitUpper = 42.5f;

	// 높이 평균 구하기
	int k = 0;
	double avr = 0;
	for (auto &vtx : sensor1->m_buffer.m_vertices)
	{
		if ((xLimitLower < vtx.x)
			&& (xLimitUpper > vtx.x)
			&& (zLimitLower < vtx.z)
			&& (zLimitUpper > vtx.z))
		{
			avr = CalcAverage(++k, avr, vtx.y);
		}
	}

	// 표준 편차 구하기
	k = 0;
	double sd = 0;
	for (auto &vtx : sensor1->m_buffer.m_vertices)
	{
		if ((xLimitLower < vtx.x)
			&& (xLimitUpper > vtx.x)
			&& (zLimitLower < vtx.z)
			&& (zLimitUpper > vtx.z))
		{
			sd = CalcAverage(++k, sd, (vtx.y - avr) * (vtx.y - avr));
		}
	}
	sd = sqrt(sd);

	return sd;
}


// 파일열기 다이얼로그를 띄운다.
StrPath cCalibrationView::OpenFileDialog()
{
	IFileOpenDialog *pFileOpen;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr))
	{
		COMDLG_FILTERSPEC filter[] = {
			{ L"Configuration File (*.txt)", L"*.txt" }
			,{ L"All File (*.*)", L"*.*" }
		};
		pFileOpen->SetFileTypes(ARRAYSIZE(filter), filter);

		hr = pFileOpen->Show(NULL);
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr))
				{
					WStrPath path = pszFilePath;
					CoTaskMemFree(pszFilePath);
					return path.str();
				}
				pItem->Release();
			}
		}
		pFileOpen->Release();
	}

	return "";
}
