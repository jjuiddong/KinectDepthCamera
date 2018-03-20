
#include "stdafx.h"
#include "3dview.h"
#include "plyreader.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_showGround(true)
	, m_showWireframe(false)
	//, m_pointCloudCount(0)
	, m_genPlane(-1)
	, m_showSensorPlane(true)
	, m_isGenPlane(false)
	, m_isGenVolumeCenter(false)
	, m_state(eState::NORMAL)
	, m_offset(Vector3(0,10,0))
	, m_isAutoProcess(false)
	, m_showPointCloud(true)
	, m_showBoxAreaPointCloud(true)
{
}

c3DView::~c3DView()
{
}


bool c3DView::Init(cRenderer &renderer)
{
	const Vector3 eyePos(0.f, 1.f, -30.f);
	const Vector3 lookAt(0, 0, 0);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, m_rect.Width() / m_rect.Height(), 1, 1000000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());

	sf::Vector2u size((u_int)m_rect.Width() - 15, (u_int)m_rect.Height() - 50);
	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = (float)size.x;
	vp.m_vp.Height = (float)size.y;
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	m_ground.Create(renderer, 100, 100, 10, 10);
	m_planeGrid.Create(renderer, 100, 100, 10, 10);

	//m_vtxBuff.Create(renderer, g_kinectDepthHeight*g_kinectDepthWidth, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	//m_vertices.resize(g_kinectDepthHeight*g_kinectDepthWidth);

	m_sphere.Create(renderer, 1, 10, 10, cColor::WHITE);

	m_volumeCenterLine.Create(renderer);

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	if (g_root.m_isUpdate)
	{
		//ProcessDepth(g_root.m_nTime, g_root.m_pDepthBuff
		//	, g_kinectDepthWidth, g_kinectDepthHeight
		//	, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);

		if (m_isAutoProcess)
		{
			//ChangeSpace();
			//MeasureVolume();
		}
	}

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

		m_sphere.Render(renderer);

		// Render Point Cloud
		if (m_showPointCloud)
		{
			g_root.m_sensorBuff.Render(renderer);
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

			cColor colors[] = {
				cColor::YELLOW, cColor::RED, cColor::GREEN, cColor::BLUE
			};

			for (int i=0; i < g_root.m_areaFloorCnt; ++i)			
			{
				auto &areaFloor = g_root.m_areaBuff[i];

				common::Vector4 color;
				if (i < ARRAYSIZE(colors))
					color = colors[i].GetColor();
				else
					color = common::Vector4(common::randfloatpositive(), common::randfloatpositive(), common::randfloatpositive(), 1);

				XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&color);
				renderer.m_cbMaterial.m_v->diffuse = diffuse;
				renderer.m_cbMaterial.Update(renderer, 2);
				areaFloor->vtxBuff->Bind(renderer);
				renderer.GetDevContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
				renderer.GetDevContext()->DrawInstanced(areaFloor->areaCnt, 1, 0, 0);
			}
		}
	}
	m_renderTarget.End(renderer);
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
	ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 400), min(m_viewRect.Height(), 500)));
	if (ImGui::Begin("Information", &isOpen, ImVec2(400.f, 500.f), windowAlpha, flags))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		if (ImGui::Button("Camera Origin"))
		{
			const Vector3 eyePos(0.f, 1.f, -30.f);
			const Vector3 lookAt(0, 0, 0);
			m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
		}
		ImGui::SameLine();
		ImGui::Checkbox("Update", &g_root.m_isUpdate);
		ImGui::SameLine();
		ImGui::Checkbox("Ground", &m_showGround);
		ImGui::SameLine();
		ImGui::Checkbox("Sensor Ground", &m_showSensorPlane);		

		ImGui::Checkbox("Point Cloud", &m_showPointCloud);
		ImGui::SameLine();
		ImGui::Checkbox("Box Aread Point Cloud", &m_showBoxAreaPointCloud);

		ImGui::DragFloat("Density", &g_root.m_depthDensity, 0.1f, 0.f, 10.f);

		if (ImGui::Button("Process"))
		{
			//ProcessDepth(g_root.m_nTime, g_root.m_pDepthBuff
			//	, g_kinectDepthWidth, g_kinectDepthHeight
			//	, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);

			if (m_isAutoProcess)
			{
				//ChangeSpace();
				//MeasureVolume();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Gen Plane"))
		{
			m_genPlane = 0;
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
		const Vector3 &volumeCenter = g_root.m_sensorBuff.m_volumeCenter;
		ImGui::Text("volume center = %f, %f, %f", volumeCenter.x, volumeCenter.y, volumeCenter.z);

		if (ImGui::Button("Change Space"))
		{
			//ChangeSpace();
		}

		if (ImGui::Button("Volume Measure"))
		{
			//MeasureVolume();
		}

		ImGui::Checkbox("Auto Process", &m_isAutoProcess);

		ImGui::End();
	}
}


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


bool c3DView::ReadPlyFile(const string &fileName)
{
	cPlyReader reader;
	if (!reader.Read(fileName))
		return false;


	
	return true;
}

//
//bool c3DView::ProcessDepth(INT64 nTime
//	, const UINT16* pBuffer
//	, int nWidth
//	, int nHeight
//	, USHORT nMinDepth
//	, USHORT nMaxDepth)
//{
//	// Make sure we've received valid data
//	if (!pBuffer
//		|| (nWidth != g_kinectDepthWidth)
//		|| (nHeight != g_kinectDepthHeight))
//		return false;
//
//	for (int i = 0; i < g_kinectDepthHeight; ++i)
//	{
//		for (int k = 0; k < g_kinectDepthWidth; ++k)
//		{
//			const Vector3 pos = Get3DPos(k, i, nMinDepth, nMaxDepth);
//			m_vertices[i*g_kinectDepthWidth + k] = pos + m_offset;
//		}
//	}
//
//	// Update Texture
//	cRenderer &renderer = GetRenderer();
//
//	// Update Point Cloud
//	m_pointCloudCount = 0;
//	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
//	{
//		int cnt = 0;
//
//		for (int i = 0; i < g_kinectDepthHeight; ++i)
//		{
//			for (int k = 0; k < g_kinectDepthWidth; ++k)
//			{
//				const Vector3 p1 = GetVertex(k, i);
//				const Vector3 p2 = GetVertex(k-1, i);
//				const Vector3 p3 = GetVertex(k, i-1);
//				const Vector3 p4 = GetVertex(k+1, i);
//				const Vector3 p5 = GetVertex(k, i+1);
//
//				const float l1 = p1.Distance(p2);
//				const float l2 = p1.Distance(p3);
//				const float l3 = p1.Distance(p4);
//				const float l4 = p1.Distance(p5);
//
//				const float maxDist = g_root.m_depthDensity;
//				if ((l1 > maxDist)
//					|| (l2 > maxDist)
//					|| (l3 > maxDist)
//					|| (l4 > maxDist)
//					)
//					continue;					
//
//
//				dst->p = p1;
//				m_vertices[cnt++] = p1;
//
//				++dst;
//				++m_pointCloudCount;
//			}
//		}
//
//		m_vtxBuff.Unlock(renderer);
//
//		for (u_int i = (u_int)cnt; i < m_vertices.size(); ++i)
//			m_vertices[i] = Vector3(0, 0, 0);
//	}
//
//	return true;
//}
//
//
//inline Vector3 c3DView::Get3DPos(const int x, const int y, USHORT nMinDepth, USHORT nMaxDepth)
//{
//	if ((x < 0) || (x >= g_kinectDepthWidth)
//		|| (y < 0) || (y >= g_kinectDepthHeight))
//		return Vector3(0, 0, 0);
//
//	const USHORT depth = g_root.m_pDepthBuff[y * g_kinectDepthWidth + x];
//	if (depth < nMinDepth)
//		return Vector3();
//
//	const float w = 10.f;
//	const float h = ((float)g_kinectDepthHeight / (float)g_kinectDepthWidth) * w;
//	const float d = (float)depth / (float)nMaxDepth;
//
//	// x = -w ~ +w
//	// y = -h ~ +h
//	const float x0 = ((float)x - (g_kinectDepthWidth / 2.f)) / (g_kinectDepthWidth / 2.f) * w;
//	const float y0 = -((float)y - (g_kinectDepthHeight / 2.f)) / (g_kinectDepthHeight / 2.f) * h;
//	const Vector3 p0(x0, y0, 10);
//	const Vector3 p1 = p0 * 50.f;
//	const float len = p0.Distance(p1);
//
//	const Vector3 p2 = p0.Normal() * (d * len);
//
//	return p2;
//}
//
//
//inline common::Vector3 c3DView::GetVertex(const int x, const int y)
//{
//	if ((x < 0) || (x >= g_kinectDepthWidth)
//		|| (y < 0) || (y >= g_kinectDepthHeight))
//		return Vector3(0, 0, 0);
//
//	return m_vertices[y*g_kinectDepthWidth + x];
//}
//
//
//Vector3 c3DView::PickVertex(const Ray &ray)
//{
//	Vector3 mostNearVertex;
//	float maxDot = 0;
//
//	for (auto &vtx : g_root.m_sensorBuff.m_vertices)
//	{
//		const Vector3 p = vtx;
//		const Vector3 v = (p - ray.orig).Normal();
//		const float d = abs(ray.dir.DotProduct(v));
//		if (maxDot < d)
//		{
//			maxDot = d;
//			mostNearVertex = vtx;
//		}
//	}
//
//	return mostNearVertex;
//}
//
//
//// sensor plane, volume center space change
//void c3DView::ChangeSpace()
//{
//	Quaternion q;
//	q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
//	const Matrix44 tm = q.GetMatrix();
//
//	Vector3 center = m_volumeCenter * tm;
//	center.y = 0;
//
//	for (u_int i = 0; i < g_root.m_sensorBuff.m_vertices.size(); ++i)
//	{
//		Vector3 pos = g_root.m_sensorBuff.m_vertices[i];
//		g_root.m_sensorBuff.m_vertices[i] = pos * tm;
//		g_root.m_sensorBuff.m_vertices[i].y += m_plane.D;
//		g_root.m_sensorBuff.m_vertices[i].x -= center.x;
//		g_root.m_sensorBuff.m_vertices[i].z -= center.z;
//	}
//
//	cRenderer &renderer = GetRenderer();
//	if (sVertex *dst = (sVertex*)g_root.m_sensorBuff.m_vtxBuff.Lock(renderer))
//	{
//		for (u_int i = 0; i < g_root.m_sensorBuff.m_vertices.size(); ++i)
//		{
//			dst->p = g_root.m_sensorBuff.m_vertices[i];
//			++dst;
//		}
//		g_root.m_sensorBuff.m_vtxBuff.Unlock(renderer);
//	}
//}
//
//
//void c3DView::MeasureVolume()
//{
//	m_isAutoProcess = true;
//
//	// Calculate Height Distribution
//	g_root.m_distribCount = 0;
//	ZeroMemory(g_root.m_hDistrib, sizeof(g_root.m_hDistrib));
//
//	for (auto &vtx : g_root.m_sensorBuff.m_vertices)
//	{
//		if (abs(vtx.x) > 15.f)
//			continue;
//		if (abs(vtx.z) > 15.f)
//			continue;
//
//		const int h = (int)(vtx.y * 10.f);
//		if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
//		{
//			g_root.m_distribCount++;
//			g_root.m_hDistrib[h] += 1.f;
//		}
//	}
//
//	// height distribute differential - 2
//	ZeroMemory(&g_root.m_hDistribDifferential, sizeof(g_root.m_hDistribDifferential));
//
//	float oldD = 0;
//	float oldArea = 0;
//	for (int i = 0; i < ARRAYSIZE(g_root.m_hDistrib); ++i)
//	{
//		if (i == 0)
//		{
//			g_root.m_hDistribDifferential.AddValue(0);
//			continue;
//		}
//
//		const float a = g_root.m_hDistrib[i];
//		const float d = g_root.m_hDistrib[i] - g_root.m_hDistrib[i - 1];
//		if ((d * oldD <= 0) && ((oldArea > 100) || (a > 100)))
//			g_root.m_hDistribDifferential.AddValue( a );
//		else
//			g_root.m_hDistribDifferential.AddValue(0);
//
//		oldD = d;
//		oldArea = a;
//	}
//
//	// Generate Area Floor
//	cRenderer &renderer = GetRenderer();
//	u_int floor = 0;
//	for (int i = 10; i < g_root.m_hDistribDifferential.size; ++i)
//	{
//		if (g_root.m_hDistribDifferential.values[i] <= 0)
//			continue;
//
//		int maxAreaIdx = i;
//		for (int k = i+1; k < (i + 4); ++k)
//		{
//			if (g_root.m_hDistribDifferential.values[i] > g_root.m_hDistribDifferential.values[maxAreaIdx])
//				maxAreaIdx = k;			
//		}
//		i += 3;
//
//		if (g_root.m_areaBuff.size() <= floor)
//		{
//			cVertexBuffer *vtxBuff = new cVertexBuffer();
//			vtxBuff->Create(renderer, g_kinectDepthHeight*g_kinectDepthWidth, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
//			
//			cRoot::sAreaFloor *areaFloor = new cRoot::sAreaFloor;
//			areaFloor->vtxBuff = vtxBuff;
//			g_root.m_areaBuff.push_back(areaFloor);
//		}
//
//		cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[floor++];
//		areaFloor->areaCnt = 0;
//		areaFloor->areaMin = INT_MAX;
//		areaFloor->areaMax = 0;
//		ZeroMemory(&areaFloor->areaGraph, sizeof(areaFloor->areaGraph));
//
//		// Generate AreaFloor Vertex
//		if (sVertex *dst = (sVertex*)areaFloor->vtxBuff->Lock(renderer))
//		{
//			for (auto &vtx : g_root.m_sensorBuff.m_vertices)
//			{
//				if (abs(vtx.x) > 15.f)
//					continue;
//				if (abs(vtx.z) > 15.f)
//					continue;
//
//				const int mostHighIdx = maxAreaIdx;
//				const int h = (int)(vtx.y * 10.f);
//				if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
//				{
//					const bool ok = ((h - mostHighIdx) == 0)
//						|| (((h - mostHighIdx) > 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[0]))
//						|| (((h - mostHighIdx) < 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[1]));
//
//					if (ok)
//					{
//						++areaFloor->areaCnt;
//						dst->p = vtx + Vector3(0, 0.01f, 0);
//						++dst;
//					}
//				}
//			}
//
//			areaFloor->vtxBuff->Unlock(renderer);
//		}
//
//		if (areaFloor->areaCnt > areaFloor->areaMax)
//			areaFloor->areaMax = areaFloor->areaCnt;
//		if (areaFloor->areaCnt < areaFloor->areaMin)
//			areaFloor->areaMin = areaFloor->areaCnt;
//
//		areaFloor->areaGraph.AddValue((float)areaFloor->areaCnt);
//	}
//	g_root.m_areaFloorCnt = floor;
//
//	// minimum height 10
//	//int mostHighIdx = 10;
//	//for (int i = 10; i < ARRAYSIZE(g_root.m_hDistrib); ++i)
//	//{
//	//	if (g_root.m_hDistrib[i] > g_root.m_hDistrib[mostHighIdx])
//	//	{
//	//		mostHighIdx = i;
//	//	}
//	//}
//
//	//g_root.m_areaCount = 0;
//	//cRenderer &renderer = GetRenderer();
//	//if (sVertex *dst = (sVertex*)m_vtxBuff2.Lock(renderer))
//	//{
//	//	for (auto &vtx : m_vertices)
//	//	{
//	//		if (abs(vtx.x) > 15.f)
//	//			continue;
//	//		if (abs(vtx.z) > 15.f)
//	//			continue;
//
//	//		const int h = (int)(vtx.y * 10.f);
//	//		if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
//	//		{
//	//			const bool ok = ((h - mostHighIdx) == 0)
//	//				|| (((h - mostHighIdx) > 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[0]))
//	//				|| (((h - mostHighIdx) < 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[1]));
//	//			
//	//			if (ok)
//	//			{
//	//				++g_root.m_areaCount;
//	//				dst->p = vtx + Vector3(0,0.01f,0);
//	//				++dst;
//	//			}
//	//		}
//	//	}
//
//	//	m_vtxBuff2.Unlock(renderer);
//	//}
//
//	//if (g_root.m_areaCount > g_root.m_areaMax)
//	//	g_root.m_areaMax = g_root.m_areaCount;
//	//if (g_root.m_areaCount < g_root.m_areaMin)
//	//	g_root.m_areaMin = g_root.m_areaCount;
//
//	//g_root.m_areaGraph.AddValue((float)g_root.m_areaCount);
//}


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
		zoomLen = max(1.f, (len / 2.f));
	else
		zoomLen = (len / 3.f);

	GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;

	if (m_mouseDown[0])
	{
		Vector3 dir = GetMainCamera().GetDirection();
		Vector3 right = GetMainCamera().GetRight();
		dir.y = 0;
		dir.Normalize();
		right.y = 0;
		right.Normalize();

		GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.001f);
		GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.001f);
	}
	else if (m_mouseDown[1])
	{
		m_camera.Yaw2(delta.x * 0.005f, Vector3(0, 1, 0));
		m_camera.Pitch2(delta.y * 0.005f, Vector3(0, 1, 0));
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
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

		// Generate Plane
		if ((m_genPlane >= 0) && (m_genPlane < 3))
		{
			const Vector3 vtxPos = g_root.m_sensorBuff.PickVertex(ray);
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

				//m_plane = plane;
				g_root.m_sensorBuff.GeneratePlane(m_planePos);
			}
		}

		// Picking Vertex Pos
		if (eState::PICKPOS == m_state)
		{
			const Vector3 vtxPos = g_root.m_sensorBuff.PickVertex(ray);
			m_pickPos = vtxPos;
			m_sphere.SetPos(vtxPos);
			m_state = eState::NORMAL;
		}

		// Picking Volume Center
		if (eState::VCENTER == m_state)
		{
			const Vector3 vtxPos = g_root.m_sensorBuff.PickVertex(ray);
			g_root.m_sensorBuff.m_volumeCenter = vtxPos;
			m_isGenVolumeCenter = true;

			const Plane &plane = g_root.m_sensorBuff.m_plane;
			const float d = plane.Distance(vtxPos) * 10.f;
			m_volumeCenterLine.SetLine(vtxPos + plane.N*d, vtxPos - plane.N*d, 0.1f);
			
			m_state = eState::NORMAL;
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

		case sf::Keyboard::Left: m_camera.MoveRight(-0.5f); break;
		case sf::Keyboard::Right: m_camera.MoveRight(0.5f); break;
		case sf::Keyboard::Up: m_camera.MoveUp(0.5f); break;
		case sf::Keyboard::Down: m_camera.MoveUp(-0.5f); break;
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

