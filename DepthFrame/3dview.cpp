
#include "stdafx.h"
#include "3dview.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_showGround(true)
	, m_showWireframe(false)
	, m_genPlane(-1)
	, m_showSensorPlane(true)
	, m_isGenPlane(false)
	, m_isGenVolumeCenter(false)
	, m_state(eState::NORMAL)
	, m_showPointCloud(true)
	, m_showBoxAreaPointCloud(true)
	, m_planeStandardDeviation(0)
	, m_showBoxVolume(false)
	, m_isUpdateOrthogonalProjection(true)
	, m_rangeMinMax(50,50)
{
}

c3DView::~c3DView()
{
}


bool c3DView::Init(cRenderer &renderer)
{
	const Vector3 eyePos = g_root.m_3dEyePos;// (0.f, 380.f, -300.f);
	const Vector3 lookAt = g_root.m_3dLookAt;// (0, 0, 0);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, m_rect.Width() / m_rect.Height(), 1, 1000000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());

	sf::Vector2u size((u_int)m_rect.Width() - 15, (u_int)m_rect.Height() - 50);
	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = (float)size.x;
	vp.m_vp.Height = (float)size.y;
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	cViewport vp2 = renderer.m_viewPort;
	vp2.m_vp.Width = g_capture3DWidth;
	vp2.m_vp.Height = g_capture3DHeight;
	m_captureTarget.Create(renderer, vp2
		//, DXGI_FORMAT_R8G8B8A8_UNORM
		, DXGI_FORMAT_R32_FLOAT
		, true, true);

	m_ground.Create(renderer, 100, 100, 10, 10);
	m_planeGrid.Create(renderer, 100, 100, 10, 10);
	m_sphere.Create(renderer, 1, 10, 10, cColor::WHITE);
	m_volumeCenterLine.Create(renderer);
	m_boxLine.Create(renderer);

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	m_camera.SetCamera(g_root.m_3dEyePos, g_root.m_3dLookAt, Vector3(0, 1, 0));
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);

	renderer.UnbindTextureAll();

	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	if (m_renderTarget.Begin(renderer, common::Vector4(20.f/255.f, 20.f / 255.f, 20.f / 255.f, 1)))
	{
		CommonStates states(renderer.GetDevice());

		if (m_showWireframe)
		{
			renderer.GetDevContext()->RSSetState(states.Wireframe());
		}
		else
		{
			renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());
		}

		if (m_showGround)
			m_ground.Render(renderer);

		if (m_isGenPlane && m_showSensorPlane)
			m_planeGrid.Render(renderer);

		if (m_isGenVolumeCenter)
			m_volumeCenterLine.Render(renderer);

		m_boxLine.SetColor(cColor::BLUE);

		if (m_showBoxVolume)
			RenderBoxVolume3D(renderer);

		m_sphere.Render(renderer);

		// Render Point Cloud
		if (m_showPointCloud)
		{
			for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
				if (sensor->m_isShow)
					if (sensor->m_buffer.m_isLoaded)
						sensor->m_buffer.Render(renderer, "Unlit", false);

			//renderer.GetDevContext()->RSSetState(states.Wireframe());
			//g_root.m_sensorBuff.RenderTessellation(renderer);
		}

		if (m_showBoxAreaPointCloud)
		{
			cShader11 *shader = renderer.m_shaderMgr.FindShader(eVertexType::POSITION);
			assert(shader);
			shader->SetTechnique("Unlit");
			shader->Begin();
			shader->BeginPass(renderer, 0);

			renderer.m_cbPerFrame.m_v->mWorld = XMMatrixIdentity();
			renderer.m_cbPerFrame.Update(renderer);
			renderer.m_cbMaterial.Update(renderer, 2);

			for (int i=0; i < g_root.m_areaFloorCnt; ++i)		
			{
				auto &areaFloor = g_root.m_areaBuff[i];

				XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&areaFloor->color.GetColor());
				renderer.m_cbMaterial.m_v->diffuse = diffuse;
				renderer.m_cbMaterial.Update(renderer, 2);
				areaFloor->vtxBuff->Bind(renderer);
				renderer.GetDevContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
				renderer.GetDevContext()->DrawInstanced(areaFloor->areaCnt, 1, 0, 0);
			}
		}

		if (eState::RANGE2 == m_state)
		{
			// left top
			const Vector3 offset[4] = {
				Vector3(-m_rangeMinMax.x, -m_rangeCenter.y, -m_rangeMinMax.y) // left-top
				, Vector3(m_rangeMinMax.x, -m_rangeCenter.y, -m_rangeMinMax.y) // right-top
				, Vector3(m_rangeMinMax.x, -m_rangeCenter.y, m_rangeMinMax.y) // right-bottom
				, Vector3(-m_rangeMinMax.x, -m_rangeCenter.y, m_rangeMinMax.y) // left-bottom
			};

			renderer.m_dbgLine.SetColor(cColor::RED);
			for (int i=0; i < 4; ++i)
			{
				const Vector3 p0 = m_rangeCenter + offset[i];
				const Vector3 p1 = p0 + Vector3(0, m_rangeCenter.y, 0);
				renderer.m_dbgLine.SetLine(p0, p1, 1.f);
				renderer.m_dbgLine.Render(renderer);
			}

			renderer.m_dbgLine.SetColor(cColor::GREEN);
			for (int i = 0; i < 4; ++i)
			{
				const int next = (i + 1) % 4;
				const Vector3 p0 = m_rangeCenter + offset[i] + Vector3(0, m_rangeCenter.y,0);
				const Vector3 p1 = m_rangeCenter + offset[next] + Vector3(0, m_rangeCenter.y, 0);
				renderer.m_dbgLine.SetLine(p0, p1, 1.f);
				renderer.m_dbgLine.Render(renderer);
			}


			renderer.m_dbgLine.SetColor(cColor::WHITE); // recovery
		}
	}
	m_renderTarget.End(renderer);
}


// 직교투영으로 -Y축으로 바라보면서 장면을 그린다.
// 그린 장면을 로컬메모리에 저장한다.
void c3DView::Capture3D()
{
	RET(!m_isUpdateOrthogonalProjection);

	const Vector3 eyePos(0.f, 380.f, 00.f);
	const Vector3 lookAt(0, 0, 0);
	const float width = g_capture3DWidth;
	const float height = g_capture3DHeight;

	cCamera3D camera("parallel camera");
	camera.SetCamera(eyePos, lookAt, Vector3(0, 0, 1));
	camera.SetProjectionOrthogonal(width, height, 1, 10000.f); // 직교투영
	
	const sRectf curViewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	const sRectf viewRect = { 0, 0, width, height};
	camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&camera);

	renderer.UnbindTextureAll();

	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	if (m_captureTarget.Begin(renderer, common::Vector4(0,0,0,1)))
	{
		CommonStates states(renderer.GetDevice());
		//renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());
		renderer.GetDevContext()->RSSetState(states.CullNone());

		Transform tfm;
		tfm.scale = Vector3(1.5f, 1, 1.5f);
		//tfm.scale = Vector3(3.f, 1, 3.f);
		//tfm.scale = Vector3(2.f, 1, 2.f);
		//g_root.m_sensorBuff.RenderTessellation(renderer, tfm.GetMatrixXM());

		for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
			if (sensor->m_isShow)
				if (sensor->m_buffer.m_isLoaded)
					sensor->m_buffer.Render(renderer, "Heightmap", false
						, tfm.GetMatrixXM());
	}
	m_captureTarget.End(renderer);

	// Save Png File (Debugging)
	//DirectX::SaveWICTextureToFile(renderer.GetDevContext(), m_captureTarget.m_texture
	//	, GUID_ContainerFormatPng, L"3dcapture.png");

	// Copy RenderTarget to cv::Mat
	{
		using Microsoft::WRL::ComPtr;
		D3D11_TEXTURE2D_DESC desc = {};
		ComPtr<ID3D11Texture2D> pStaging;
		HRESULT hr = DirectX::CaptureTexture(renderer.GetDevContext(), m_captureTarget.m_texture, desc, pStaging);
		if (FAILED(hr))
			return;

		float *dst = (float*)g_root.m_projMap.data;
		D3D11_MAPPED_SUBRESOURCE map;
		hr = renderer.GetDevContext()->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &map);
		if (FAILED(hr))
			return;

		if (float *src = (float*)map.pData)
		{
			for (int i = 0; i < width * height; ++i)
				*dst++ = *src++;
			renderer.GetDevContext()->Unmap(pStaging.Get(), 0);
		}
	}
}


void c3DView::RenderBoxVolume3D(graphic::cRenderer &renderer)
{
	for (auto &box : g_root.m_boxes)
	{
		m_boxLine.SetColor(box.color);

		for (u_int i = 0; i < box.pointCnt; ++i)
		{
			m_boxLine.SetLine(box.box3d[i], box.box3d[(i + 1) % box.pointCnt], 0.1f);
			m_boxLine.Render(renderer);
		}

		for (u_int i = 0; i < box.pointCnt; ++i)
		{
			m_boxLine.SetLine(box.box3d[i + box.pointCnt]
				, box.box3d[((i + 1) % box.pointCnt) + box.pointCnt], 0.1f);
			m_boxLine.Render(renderer);
		}

		for (u_int i = 0; i < box.pointCnt; ++i)
		{
			m_boxLine.SetLine(box.box3d[i], box.box3d[i + box.pointCnt], 0.1f);
			m_boxLine.Render(renderer);
		}
	}
}


void c3DView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	m_viewRect = { pos.x + 5, pos.y, pos.x + m_rect.Width() - 30, pos.y + m_rect.Height() - 50 };
	ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

	// HUD
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(std::min(m_viewRect.Width(), 800.f), std::min(m_viewRect.Height(), 500.f)));
	if (ImGui::Begin("Information", &isOpen, ImVec2(m_viewRect.Width(), 800.f), windowAlpha, flags))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// Show Visible Camera Flag
		if (g_root.m_baslerCam.IsReadyCapture())
		{
			bool firstCheckBox = true;
			for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
			{
				if (!sensor->IsEnable())
				{
					sensor->m_isShow = false;
					continue;
				}

				if (!firstCheckBox)
					ImGui::SameLine();

				firstCheckBox = false;
				if (ImGui::Checkbox(sensor->m_info.strDisplayName.c_str(), &sensor->m_isShow))
				{
					g_root.MeasureVolume();
				}
			}
		}
		else
		{
			ImGui::Text("Try Connect Camera...");
		}

		if (ImGui::Button("Camera Origin"))
		{
			const Vector3 eyePos(0.f, 380.f, -300.f);
			const Vector3 lookAt(0, 0, 0);
			m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));

			g_root.m_3dEyePos = m_camera.GetEyePos();
			g_root.m_3dLookAt = m_camera.GetLookAt();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Ground", &m_showGround);
		ImGui::SameLine();
		ImGui::Checkbox("Sensor Ground", &m_showSensorPlane);		

		ImGui::Checkbox("Point Cloud", &m_showPointCloud);
		ImGui::SameLine();
		ImGui::Checkbox("Box Aread Point Cloud", &m_showBoxAreaPointCloud);
		ImGui::SameLine();
		ImGui::Checkbox("Box Volume", &m_showBoxVolume);

		if (ImGui::Button("Gen Plane"))
		{
			m_genPlane = 0;
		}

		ImGui::SameLine();
		if (ImGui::Button("Save Plane"))
		{
			cSensor *sensor1 = g_root.m_baslerCam.m_sensors.empty()? NULL : g_root.m_baslerCam.m_sensors[0];

			if (m_isGenPlane && sensor1)
			{
				g_root.m_config.SetValue("plane-x", sensor1->m_buffer.m_plane.N.x);
				g_root.m_config.SetValue("plane-y", sensor1->m_buffer.m_plane.N.y);
				g_root.m_config.SetValue("plane-z", sensor1->m_buffer.m_plane.N.z);
				g_root.m_config.SetValue("plane-d", sensor1->m_buffer.m_plane.D);
			}

			if (m_isGenVolumeCenter && sensor1)
			{
				g_root.m_config.SetValue("center-x", sensor1->m_buffer.m_volumeCenter.x);
				g_root.m_config.SetValue("center-y", sensor1->m_buffer.m_volumeCenter.y);
				g_root.m_config.SetValue("center-z", sensor1->m_buffer.m_volumeCenter.z);
			}

			if (g_root.m_config.Write("config_depthframe.txt"))
			{
				// ok
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Load Plane"))
		{
			common::cConfig config;
			if (config.Read("config_depthframe.txt"))
			{
				if (config.GetString("plane-x", "none") != "none")
				{
					m_isGenPlane = true;
					for (cSensor *sensor : g_root.m_baslerCam.m_sensors)

					{
						sensor->m_buffer.m_plane.N.x = config.GetFloat("plane-x");
						sensor->m_buffer.m_plane.N.y = config.GetFloat("plane-y");
						sensor->m_buffer.m_plane.N.z = config.GetFloat("plane-z");
						sensor->m_buffer.m_plane.D = config.GetFloat("plane-d");
					}
				}

				if (config.GetString("center-x", "none") != "none")
				{
					m_isGenVolumeCenter = true;
					for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
					{
						sensor->m_buffer.m_volumeCenter.x = config.GetFloat("center-x");
						sensor->m_buffer.m_volumeCenter.y = config.GetFloat("center-y");
						sensor->m_buffer.m_volumeCenter.z = config.GetFloat("center-z");
					}
				}
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Pick Pos"))
		{
			m_state = eState::PICKPOS;
		}
		ImGui::Text("pick pos = %f, %f, %f", m_pickPos.x, m_pickPos.y, m_pickPos.z);

		if (ImGui::Button("Volume Center"))
		{
			m_state = eState::VCENTER;
		}

		cSensor *sensor1 = g_root.m_baslerCam.m_sensors.empty() ? NULL : g_root.m_baslerCam.m_sensors[0];
		if (sensor1)
		{
			const Vector3 &volumeCenter = sensor1->m_buffer.m_volumeCenter;
			ImGui::Text("volume center = %f, %f, %f", volumeCenter.x, volumeCenter.y, volumeCenter.z);

			if (ImGui::Button("Change Space"))
			{
				sensor1->m_buffer.ChangeSpace(GetRenderer());
			}
		}


		if (ImGui::Button("Standard Deviation"))
		{
			m_planeStandardDeviation = CalcBasePlaneStandardDeviation();
		}

		if (m_planeStandardDeviation != 0)
		{
			ImGui::Text("Standard Deviation = %f", m_planeStandardDeviation);
		}

		if (ImGui::Button("Pick Range"))
		{
			m_state = eState::RANGE;
		}

		if (eState::RANGE2 == m_state)
		{
			ImGui::DragFloat2("MinMax", (float*)&m_rangeMinMax, 0.1f);

			if (ImGui::Button("Calibration"))
			{
				cSensor *selSensor = NULL;
				for (auto sensor : g_root.m_baslerCam.m_sensors)
				{
					if (sensor->m_isEnable && sensor->m_isShow 
						&& sensor->m_buffer.m_isLoaded )
					{
						selSensor = sensor;
						break;
					}
				}
				if (selSensor)
					CalcBasePlaneCalibration(selSensor);
			}
		}

		ImGui::End();
	}
}


// 바닥 평면 컬리브레이션
// 영역이 정해진 후, 
//        top
//  * ----- * ----- * 
//  |   0   |   1   | right
//  |       |       |
//  * ----- * ----- *
//  |   3   |   2   |
//  |       |       |
//  * ----- * ----- *
//        bottom
//
//   /\ Z axis
//    |
//    |
//    |
//   -------------> X Axis
//
// 4등분 한 후, 랜덤으로 설정된 3 영역에서, 랜덤포인트를 찍는다.
// 3포인트로 평면을 생성하고, 그 평면에서 영역의 높이 편차를 구한 후
// 가장 작게되는 편차가 될 때까지 반복한다.
void c3DView::CalcBasePlaneCalibration(const cSensor *sensor)
{
	// 위치값을 기본 좌표계로 복원한다.
	const Vector3 center = m_rangeCenter * sensor->m_buffer.m_offset.Inverse();
	const sRectf range = sRectf(center.x - m_rangeMinMax.x, center.z + m_rangeMinMax.y,
		center.x + m_rangeMinMax.x, center.z - m_rangeMinMax.y);

	float offset = 5.f; // 5cm
	sRectf rects[4] = {
		sRectf(center.x - m_rangeMinMax.x, center.z + m_rangeMinMax.y
			, center.x, center.z)
		, sRectf(center.x, center.z + m_rangeMinMax.y
			, center.x + m_rangeMinMax.x, center.z)
		, sRectf(center.x, center.z
			, center.x + m_rangeMinMax.x, center.z - m_rangeMinMax.y)
		, sRectf(center.x - m_rangeMinMax.x, center.z
			, center.x, center.z - m_rangeMinMax.y)
	};

	// 오프셋 적용
	for (int i = 0; i < 4; ++i)
	{
		rects[i].left += offset;
		rects[i].top -= offset;
		rects[i].right -= offset;
		rects[i].bottom += offset;
	}

	vector<Vector3> pts[4];
	for (int i = 0; i < 4; ++i)
		pts[i].reserve(1024*5);
	
	// 4분할 영역에 속하는 포인트 클라우드를 걸러낸다.
	for (auto &vtx : sensor->m_buffer.m_vertices)
	{
		if (vtx.IsEmpty())
			continue;

		for (int i = 0; i < 4; ++i)
		{
			if ((rects[i].left < vtx.x)
				&& (rects[i].right > vtx.x)
				&& (rects[i].top > vtx.z)
				&& (rects[i].bottom < vtx.z))
			{
				if (abs(center.y - vtx.y) < offset)
					pts[i].push_back(vtx);
				break;
			}
		}
	}

	srand(timeGetTime());

	const double curSD = CalcHeightStandardDeviation(sensor, center, range, Matrix44::Identity);
	double minSD = FLT_MAX;
	Plane minPlane;
	int cnt = 0;
	while (cnt++ < 100)
	{
		// 4분할된 영역을 랜덤하게 3 영역을 선택한다.
		int rectIdAr[] = { 0,1,2,3 };
		int ids[3];
		for (int i = 0; i < 3; ++i)
		{
			const int k = rand() % (4 - i);
			ids[i] = rectIdAr[k];
			rectIdAr[k] = rectIdAr[3 - i];
		}

		// 선택한 영역에서 랜덤하게 포인트를 선택한다.
		const Vector3 p1 = pts[ids[0]][common::randint(0, pts[ids[0]].size() - 1)];
		const Vector3 p2 = pts[ids[1]][common::randint(0, pts[ids[1]].size() - 1)];
		const Vector3 p3 = pts[ids[2]][common::randint(0, pts[ids[2]].size() - 1)];

		// 선택한 포인트로 평면을 만든다.
		Plane plane(p1, p2, p3);
		Matrix44 tm;
		if (!plane.N.IsEmpty())
		{
			Quaternion q;
			q.SetRotationArc(plane.N, Vector3(0, 1, 0));
			tm *= q.GetMatrix();
			//Vector3 center = m_volumeCenter * q.GetMatrix();
			//center.y = 0;
			//Matrix44 T;
			//T.SetPosition(Vector3(-center.x, m_plane.D, -center.z));
			//tm *= T;
		}

		// 편차를 구하고, 가장 낮은 편차가 될 때까지 반복한다.
		const double sd = CalcHeightStandardDeviation(sensor, center, range, tm);
		if (minSD > sd)
		{
			minSD = sd;
			minPlane = plane;
		}
	}

	int a = 0;
}


// rect영역의 높이 표준편차를 구한다.
// rect : x-z plane rect
//		  left-right, x axis
//		  top-bottom, z axis
// tm : transform matrix
//
double c3DView::CalcHeightStandardDeviation(const cSensor *sensor, const Vector3 &center, const sRectf &rect, const Matrix44 &tm)
{
	// 높이 평균 구하기
	int k = 0;
	double avr = 0;
	for (auto &vtx : sensor->m_buffer.m_vertices)
	{
		if (vtx.IsEmpty())
			continue;

		if ((rect.left < vtx.x)
			&& (rect.right > vtx.x)
			&& (rect.top > vtx.z)
			&& (rect.bottom < vtx.z))
		{
			if (abs(vtx.y - center.y) > 10) // 10 cm
				continue;

			const Vector3 p = vtx * tm;
			avr = CalcAverage(++k, avr, p.y);
		}
	}

	// 표준 편차 구하기
	k = 0;
	double sd = 0;
	for (auto &vtx : sensor->m_buffer.m_vertices)
	{
		if (vtx.IsEmpty())
			continue;

		if ((rect.left < vtx.x)
			&& (rect.right > vtx.x)
			&& (rect.top > vtx.z)
			&& (rect.bottom < vtx.z))
		{
			if (abs(vtx.y - center.y) > 10) // 10 cm
				continue;

			const Vector3 p = vtx * tm;
			sd = CalcAverage(++k, sd, (p.y - avr) * (p.y - avr));
		}
	}
	sd = sqrt(sd);

	return sd;
}


// 바닥의 표준편차를 구한다.
double c3DView::CalcBasePlaneStandardDeviation(const size_t camIdx //=0
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


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


void c3DView::UpdateLookAt()
{
	GetMainCamera().MoveCancel();

	const float centerX = GetMainCamera().m_width / 2;
	const float centerY = GetMainCamera().m_height / 2;
	const Ray ray = GetMainCamera().GetRay((int)centerX, (int)centerY);
	const float distance = m_groundPlane1.Collision(ray.dir);
	if (distance < -0.2f)
	{
		GetMainCamera().m_lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	}
	else
	{ // horizontal viewing
		const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 50.f;
		GetMainCamera().m_lookAt = lookAt;
	}

	GetMainCamera().UpdateViewMatrix();
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void c3DView::OnWheelMove(const float delta, const POINT mousePt)
{
	UpdateLookAt();

	float len = 0;
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	Vector3 lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	len = (ray.orig - lookAt).Length();

	// zoom in/out
	float zoomLen = 0;
	if (len > 100)
		zoomLen = len * 0.1f;
	else if (len > 50)
		zoomLen = std::max(1.f, (len / 2.f));
	else
		zoomLen = (len / 3.f);

	GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);

	g_root.m_3dEyePos = GetMainCamera().GetEyePos();
	g_root.m_3dLookAt = GetMainCamera().GetLookAt();
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	bool isUpdateCam = false;

	if (m_mouseDown[0])
	{
		if (ImGui::IsAnyItemHovered())
			return;

		Vector3 dir = GetMainCamera().GetDirection();
		Vector3 right = GetMainCamera().GetRight();
		dir.y = 0;
		dir.Normalize();
		right.y = 0;
		right.Normalize();

		GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.001f);
		GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.001f);
		isUpdateCam = true;
	}
	else if (m_mouseDown[1])
	{
		m_camera.Yaw2(delta.x * 0.005f, Vector3(0, 1, 0));
		m_camera.Pitch2(delta.y * 0.005f, Vector3(0, 1, 0));
		isUpdateCam = true;
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
		isUpdateCam = true;
	}

	if (isUpdateCam)
	{
		g_root.m_3dEyePos = GetMainCamera().GetEyePos();
		g_root.m_3dLookAt = GetMainCamera().GetLookAt();
	}
}


// Handling Mouse Button Down Event
void c3DView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	UpdateLookAt();
	SetCapture();

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;
		const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		m_rotateLen = (p1 - ray.orig).Length();// min(500.f, (p1 - ray.orig).Length());

		// 화면에 보이는 센서 중, 첫번째 센서를 선택한다.
		cSensor *curSensor = NULL;
		for (auto sensor : g_root.m_baslerCam.m_sensors)
		{
			if (sensor->m_buffer.m_isLoaded && sensor->m_isShow)
			{
				curSensor = sensor;
				break;
			}
		}
		if (!curSensor)
			break;

		// Generate Plane
		if ((m_genPlane >= 0) && (m_genPlane < 3))
		{
			const Vector3 vtxPos = curSensor->m_buffer.PickVertex(ray);
			m_sphere.SetPos(vtxPos);
			m_planePos[m_genPlane++] = vtxPos;

			if (m_genPlane >= 3)
			{
				m_isGenPlane = true;
				m_showSensorPlane = true;

				Plane plane(m_planePos[0], m_planePos[1], m_planePos[2]);
				
				Quaternion q;
				q.SetRotationArc(Vector3(0, 1, 0), plane.N);
				m_planeGrid.m_transform.rot = q;
				m_planeGrid.m_transform.pos = Vector3(0, 0, 0);
				m_planeGrid.m_transform.pos = plane.N * -plane.D;

				curSensor->m_buffer.GeneratePlane(m_planePos);
			}
		}

		// Picking Vertex Pos
		if (eState::PICKPOS == m_state)
		{
			const Vector3 vtxPos = curSensor->m_buffer.PickVertex(ray);
			m_pickPos = vtxPos;
			m_sphere.SetPos(vtxPos);
			m_state = eState::NORMAL;
		}

		// Picking Volume Center
		if (eState::VCENTER == m_state)
		{
			const Vector3 vtxPos = curSensor->m_buffer.PickVertex(ray);
			curSensor->m_buffer.m_volumeCenter = vtxPos;
			m_isGenVolumeCenter = true;

			const Plane &plane = curSensor->m_buffer.m_plane;
			const float d = plane.Distance(vtxPos) * 10.f;
			m_volumeCenterLine.SetLine(vtxPos + plane.N*d, vtxPos - plane.N*d, 0.1f);
			
			m_state = eState::NORMAL;
		}

		// Picking Range Center
		if (eState::RANGE == m_state)
		{
			const Vector3 vtxPos = curSensor->m_buffer.PickVertex(ray);
			m_rangeCenter = vtxPos;
			m_sphere.SetPos(vtxPos);
			m_state = eState::RANGE2;
		}

	}
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;

		const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		Vector3 target = m_groundPlane1.Pick(ray.orig, ray.dir);
		const float len = (GetMainCamera().GetEyePos() - target).Length();
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void c3DView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	switch (button)
	{
	case sf::Mouse::Left:
		m_mouseDown[0] = false;
		break;
	case sf::Mouse::Right:
		m_mouseDown[1] = false;
		break;
	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void c3DView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.code)
		{
		case sf::Keyboard::Return:
			break;

		case sf::Keyboard::Space:
			break;

		//case sf::Keyboard::Left: g_root.m_cameraOffset2.pos.x -= 0.1f; break;
		//case sf::Keyboard::Right: g_root.m_cameraOffset2.pos.x += 0.1f; break;
		//case sf::Keyboard::Up: g_root.m_cameraOffset2.pos.z += 0.1f; break;
		//case sf::Keyboard::Down: g_root.m_cameraOffset2.pos.z -= 0.1f; break;
		}
		break;

	case sf::Event::MouseMoved:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnMouseMove(pos);
	}
	break;

	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		if (sf::Event::MouseButtonPressed == evt.type)
		{
			if (m_viewRect.IsIn((float)curPos.x, (float)curPos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			OnMouseUp(evt.mouseButton.button, pos);
		}
	}
	break;

	case sf::Event::MouseWheelScrolled:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnWheelMove(evt.mouseWheelScroll.delta, pos);
	}
	break;
	}
}


void c3DView::OnResetDevice()
{
	cRenderer &renderer = GetRenderer();

	// update viewport
	sRectf viewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cViewport vp = GetRenderer().m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, DXGI_FORMAT_D24_UNORM_S8_UINT);
}

