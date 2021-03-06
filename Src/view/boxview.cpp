#include "stdafx.h"
#include "boxview.h"
#include "filterview.h"

using namespace graphic;
using namespace framework;


cBoxView::cBoxView(const string &name)
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_showGround(true)
	, m_showPointCloud(true)
	, m_showBoxVertex(true)
	, m_showBoxAverageShape(true)
	, m_showBoxMeasureShape(true)
	, m_showProjectionMap(true)
{
}

cBoxView::~cBoxView()
{
}


bool cBoxView::Init(cRenderer &renderer)
{
	const Vector3 eyePos(0.f, 380.f, -300.f);
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
	m_boxLine.Create(renderer, Vector3(0,0,0), Vector3(1,1,1), 1, eVertexType::POSITION);

	return true;
}


void cBoxView::OnUpdate(const float deltaSeconds)
{
	m_camera.SetCamera(g_root.m_3dEyePos, g_root.m_3dLookAt, Vector3(0, 1, 0));
}


void cBoxView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);

	renderer.UnbindTextureAll();

	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	if (m_renderTarget.Begin(renderer, common::Vector4(20.f / 255.f, 20.f / 255.f, 20.f / 255.f, 1)))
	{
		CommonStates states(renderer.GetDevice());
		renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());

		if (m_showGround)
			m_ground.Render(renderer);

		if (m_showPointCloud)
		{
			if (m_showProjectionMap)
			{
				RenderProjectionMap(renderer);
			}
			else
			{
				CommonStates states(renderer.GetDevice());
				renderer.GetDevContext()->OMSetBlendState(states.NonPremultiplied(), 0, 0xffffffff);

				for (cSensor *sensor : g_root.m_baslerCam.m_sensors)
					if (sensor->m_isShow)
						if (sensor->m_buffer.m_isLoaded)
							sensor->m_buffer.Render(renderer, "Unlit", true);

				renderer.GetDevContext()->OMSetBlendState(states.Opaque(), 0, 0xffffffff);
			}
		}

		if (!m_showProjectionMap)
			RenderBoxVolume3D(renderer);
	}
	m_renderTarget.End(renderer);
}


void cBoxView::RenderProjectionMap(graphic::cRenderer &renderer)
{
	// Render Max Boundary
	renderer.m_dbgLine.SetColor(cColor::GREEN);
	for (int i = 0; i < 4; ++i)
	{
		const Vector3 p0 = g_root.m_projRoi[i];
		const Vector3 p1 = g_root.m_projRoi[(i + 1) % 4];
		renderer.m_dbgLine.SetLine(p0 + Vector3(0, 5, 0), p1 + Vector3(0, 5, 0), 0.2f);
		renderer.m_dbgLine.Render(renderer);
	}

	CommonStates states(renderer.GetDevice());
	renderer.GetDevContext()->RSSetState(states.Wireframe());
	renderer.GetDevContext()->OMSetBlendState(states.NonPremultiplied(), 0, 0xffffffff);

	g_root.m_tessPos.SetTechnique("Unlit");
	g_root.m_tessPos.Begin();
	g_root.m_tessPos.BeginPass(renderer, 0);

	// 원본 버텍스가 1.5배 되어 있는 상태.
	const float scale = 1.f / 1.5f;
	Transform tfm;
	tfm.scale = Vector3(scale, 1, scale);

	XMMATRIX parentTm = XMMatrixIdentity();
	renderer.m_cbPerFrame.m_v->mWorld = tfm.GetMatrixXM();
	renderer.m_cbPerFrame.Update(renderer);
	XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&common::Vector4(1.f, 1.f, 1.f, 0.9f));
	renderer.m_cbMaterial.m_v->diffuse = diffuse;
	renderer.m_cbMaterial.Update(renderer, 2);
	renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * scale;
	renderer.m_cbTessellation.Update(renderer, 6);

	g_root.m_projVtxBuff.Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	renderer.GetDevContext()->Draw((int)(g_capture3DWidth * g_capture3DHeight), 0);

	renderer.GetDevContext()->OMSetBlendState(states.Opaque(), 0, 0xffffffff);
	renderer.UnbindShaderAll();
}


void cBoxView::RenderBoxVolume3D(graphic::cRenderer &renderer)
{
	// Render Current Measure Box
	if (m_showBoxMeasureShape)
	{
		for (auto &box : g_root.m_measure.m_boxes)
		{
			const float width = 0.2f;
			common::Vector4 color = box.color.GetColor();
			color = color * 0.4f;
			color.w = 0.5f;
			const cColor newColor(color);
			m_boxLine.SetColor(newColor);
			//m_boxLine.SetColor(cColor(0.f,1.f,1.f));

			for (u_int i = 0; i < box.pointCnt; ++i)
			{
				m_boxLine.SetLine(box.box3d[i], box.box3d[(i + 1) % box.pointCnt], width);
				m_boxLine.Render(renderer);
			}

			for (u_int i = 0; i < box.pointCnt; ++i)
			{
				m_boxLine.SetLine(box.box3d[i + box.pointCnt]
					, box.box3d[((i + 1) % box.pointCnt) + box.pointCnt], width);
				m_boxLine.Render(renderer);
			}

			for (u_int i = 0; i < box.pointCnt; ++i)
			{
				m_boxLine.SetLine(box.box3d[i], box.box3d[i + box.pointCnt], width);
				m_boxLine.Render(renderer);
			}

			if (m_showBoxVertex)
			{
				renderer.m_dbgSphere.SetColor(cColor::GRAY);
				renderer.m_dbgSphere.SetRadius(1.f);

				for (u_int i = 0; i < box.pointCnt; ++i)
				{
					renderer.m_dbgSphere.SetPos(box.box3d[i]);
					renderer.m_dbgSphere.Render(renderer);
					renderer.m_dbgSphere.SetPos(box.box3d[i + box.pointCnt]);
					renderer.m_dbgSphere.Render(renderer);
				}
			}
		}
	}

	// Render Average Box 
	cMeasure &measure = g_root.m_measure;

	if (m_showBoxAverageShape)
	{
		for (auto &box : measure.m_avrContours)
		{
			const float distribCnt = (float)box.count / (float)measure.m_calcAverageCount;
			if (distribCnt < 0.5f)
				continue;

			const float width = 0.2f;
			m_boxLine.SetColor(box.box.color);

			const u_int pointCnt = box.box.contour.Size();
			for (u_int i = 0; i < pointCnt; ++i)
			{
				m_boxLine.SetLine(box.vertices3d[i], box.vertices3d[(i + 1) % pointCnt], width);
				m_boxLine.Render(renderer);
			}

			for (u_int i = 0; i < pointCnt; ++i)
			{
				m_boxLine.SetLine(box.vertices3d[i + pointCnt]
					, box.vertices3d[((i + 1) % pointCnt) + pointCnt], width);
				m_boxLine.Render(renderer);
			}

			for (u_int i = 0; i < pointCnt; ++i)
			{
				m_boxLine.SetLine(box.vertices3d[i], box.vertices3d[i + pointCnt], width);
				m_boxLine.Render(renderer);
			}

			if (m_showBoxVertex)
			{
				renderer.m_dbgSphere.SetColor(cColor::WHITE);
				renderer.m_dbgSphere.SetRadius(1.f);

				for (u_int i = 0; i < pointCnt; ++i)
				{
					renderer.m_dbgSphere.SetPos(box.vertices3d[i]);
					renderer.m_dbgSphere.Render(renderer);
					renderer.m_dbgSphere.SetPos(box.vertices3d[i + pointCnt]);
					renderer.m_dbgSphere.Render(renderer);
				}
			}
		}
	}
}


void cBoxView::OnRender(const float deltaSeconds)
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
	ImGui::SetNextWindowSize(ImVec2(std::min(m_viewRect.Width(), 300.f), m_viewRect.Height()-50.f));
	ImGui::SetNextWindowBgAlpha(windowAlpha);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	if (ImGui::Begin("Box Volume", &isOpen, flags))
	{
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
		ImGui::Checkbox("Point Cloud", &m_showPointCloud);
		//ImGui::SameLine();
		ImGui::Checkbox("Box Measure", &m_showBoxMeasureShape);
		ImGui::SameLine();
		ImGui::Checkbox("Box Vertex", &m_showBoxVertex);
		ImGui::SameLine();
		ImGui::Checkbox("Box Average", &m_showBoxAverageShape);
		ImGui::Checkbox("ProjectionMap", &m_showProjectionMap);

		ImGui::Separator();
		for (u_int i = 0; i < g_root.m_measure.m_boxes.size(); ++i)
		{
			auto &box = g_root.m_measure.m_boxes[i];
			 
			if (box.integral)
				ImGui::Text("Integral Calc");
			else
				ImGui::Text("Box%d", i + 1);

			ImGui::Text("\t X = %f", box.volume.x);
			ImGui::Text("\t Y = %f", box.volume.z);
			ImGui::Text("\t H = %f", box.volume.y);
			ImGui::Text("\t V/W = %f", box.minVolume / 6000.f);
			ImGui::Text("\t V/W = %f", box.maxVolume / 6000.f);
			ImGui::Text("\t Loop = %d", box.loopCnt);
			ImGui::Spacing();
			ImGui::Separator();
		}

		// Display Total V/W
		if (!g_root.m_measure.m_boxes.empty())
		{
			float totalVW = 0;
			for (u_int i = 0; i < g_root.m_measure.m_boxes.size(); ++i)
				if (!g_root.m_measure.m_boxes[i].integral)
					totalVW += g_root.m_measure.m_boxes[i].minVolume / 6000.f;

			ImGui::Text("Total V/W = %f", totalVW);
		}

		if ((cMeasure::INTEGRAL == g_root.m_measureType)
			|| (cMeasure::BOTH == g_root.m_measureType))
		{
			ImGui::Text("Integral V/W = %f", g_root.m_measure.m_integralVW);
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();
}


void cBoxView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


void cBoxView::UpdateLookAt()
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
void cBoxView::OnWheelMove(const float delta, const POINT mousePt)
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
void cBoxView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	bool isUpdateCam = false;

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
void cBoxView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
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


void cBoxView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
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


void cBoxView::OnEventProc(const sf::Event &evt)
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

			//case sf::Keyboard::Left: m_camera.MoveRight(-0.5f); break;
			//case sf::Keyboard::Right: m_camera.MoveRight(0.5f); break;
			//case sf::Keyboard::Up: m_camera.MoveUp(0.5f); break;
			//case sf::Keyboard::Down: m_camera.MoveUp(-0.5f); break;
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


void cBoxView::OnResetDevice()
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

