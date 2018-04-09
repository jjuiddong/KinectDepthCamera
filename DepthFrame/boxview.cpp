#include "stdafx.h"
#include "boxview.h"

using namespace graphic;
using namespace framework;


cBoxView::cBoxView(const string &name)
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_showGround(true)
	, m_showPointCloud(true)
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
	m_boxLine.Create(renderer);

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

		RenderBoxVolume3D(renderer);

		if (m_showPointCloud)
		{
			CommonStates states(renderer.GetDevice());
			renderer.GetDevContext()->OMSetBlendState(states.NonPremultiplied(), 0, 0xffffffff);

			if (g_root.m_baslerCameraIdx == 0)
			{
				g_root.m_sensorBuff[0].Render(renderer, "Unlit", true);
			}
			else if (g_root.m_baslerCameraIdx == 1)
			{
				if (g_root.m_sensorBuff[1].m_isLoaded)
					g_root.m_sensorBuff[1].Render(renderer, "Unlit", true, g_root.m_cameraOffset2.GetMatrixXM());
			}
			else
			{
				g_root.m_sensorBuff[0].Render(renderer, "Unlit", true);
				if (g_root.m_sensorBuff[1].m_isLoaded)
					g_root.m_sensorBuff[1].Render(renderer, "Unlit", true, g_root.m_cameraOffset2.GetMatrixXM());
			}

			//if (g_root.m_sensorBuff[1].m_isLoaded)
			//	g_root.m_sensorBuff[1].Render(renderer, "Unlit", true);

			renderer.GetDevContext()->OMSetBlendState(states.Opaque(), 0, 0xffffffff);
		}
	}
	m_renderTarget.End(renderer);
}


void cBoxView::RenderBoxVolume3D(graphic::cRenderer &renderer)
{
	for (auto &box : g_root.m_boxes)
	{
		m_boxLine.SetColor(box.color);

		for (u_int i = 0; i < box.pointCnt; ++i)
		{
			m_boxLine.SetLine(box.box3d[i], box.box3d[(i+1)% box.pointCnt], 0.1f);
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
	ImGui::SetNextWindowSize(ImVec2(std::min(m_viewRect.Width(), 500.f), std::max(m_viewRect.Height(), 800.f)));
	if (ImGui::Begin("Box Volume", &isOpen, ImVec2(std::min(m_viewRect.Width(), 500.f), std::max(m_viewRect.Height(), 800.f)), windowAlpha, flags))
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

		ImGui::Spacing();
		ImGui::Separator();
		for (u_int i = 0; i < g_root.m_boxes.size(); ++i)
		{
			auto &box = g_root.m_boxes[i];
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

		ImGui::End();
	}
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

