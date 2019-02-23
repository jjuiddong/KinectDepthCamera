//
// Volume Viewer
//
#include "stdafx.h"
#include "volumeviewer.h"
#include "../view/3dview.h"
#include "../view/colorview.h"
#include "../view/depthview.h"
#include "../view/depthview2.h"
#include "../view/infraredview.h"
#include "../view/analysisview.h"
#include "../view/inputview.h"
#include "../view/filterview.h"
#include "../view/logview.h"
#include "../view/resultview.h"
#include "../view/boxview.h"
#include "../view/cameraview.h"
#include "../view/calibrationview.h"
#include "../view/animationview.h"


using namespace graphic;
using namespace framework;

INIT_FRAMEWORK3(cViewer);

cRoot g_root;

cViewer::cViewer()
{
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");
	m_windowName = L"Volume Viewer";
	//m_isLazyMode = true;
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 1280, 960 };
	//const RECT r = { 0, 0, (int)(1280*1.5f)	, (int)(960*1.5f) };
	m_windowRect = r;
}

cViewer::~cViewer()
{
}


bool cViewer::OnInit()
{
	dbg::RemoveLog();
	dbg::RemoveErrLog();

	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 0.1f, 10000.0f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		common::Vector4(0.2f, 0.2f, 0.2f, 1)
		, common::Vector4(0.9f, 0.9f, 0.9f, 1)
		, common::Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	m_gui.SetContext();

	g_root.Create(); // ���� ���� ȣ��
	g_root.m_input = cRoot::eInputType::FILE;

	m_3dView = new c3DView("3DView");
	m_3dView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL);
	bool result = m_3dView->Init(m_renderer);
	assert(result);

	m_analysisView = new cAnalysisView("Analysis View");
	m_analysisView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, m_3dView, 0.5f);

	m_inputView = new cInputView("Input View");
	m_inputView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_analysisView);
	result = m_inputView->Init(m_renderer);
	assert(result);

	m_aniView = new cAnimationView("Animation View");
	m_aniView->Create(eDockState::DOCKWINDOW, eDockSlot::BOTTOM, this, m_3dView, 0.5f);
	result = m_aniView->Init(m_renderer);
	assert(result);

	m_calibView = new cCalibrationView("Calibration");
	m_calibView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_aniView);

	m_infraredView = new cInfraredView("Infrared View");
	m_infraredView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_aniView, 0.5f);
	result = m_infraredView->Init(m_renderer);
	assert(result);

	m_depthView = new cDepthView("Depth View");
	m_depthView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_aniView);
	result = m_depthView->Init(m_renderer);
	assert(result);

	m_depthView2 = new cDepthView2("Depth2 View");
	m_depthView2->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_aniView);
	result = m_depthView2->Init(m_renderer);
	assert(result);

	m_colorView = new cColorView("Color View");
	m_colorView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_aniView);
	result = m_colorView->Init(m_renderer);
	assert(result);

	m_boxView = new cBoxView("Box View");
	m_boxView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_3dView);
	result = m_boxView->Init(m_renderer);
	assert(result);

	m_filterView = new cFilterView("Filter View");
	m_filterView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_3dView);
	result = m_filterView->Init(m_renderer);
	assert(result);

	m_resultView = new cResultView("Result");
	m_resultView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_analysisView, 0.5f);
	result = m_resultView->Init(m_renderer);
	assert(result);

	m_logView = new cLogView("Output Log");
	m_logView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_resultView);

	m_camView = new cCameraView("Camera");
	m_camView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, m_analysisView);


	g_root.m_3dView = m_3dView;
	g_root.m_colorView = m_colorView;
	g_root.m_depthView = m_depthView;
	g_root.m_depthView2 = m_depthView2;
	g_root.m_infraredView = m_infraredView;
	g_root.m_analysisView = m_analysisView;
	g_root.m_inputView = m_inputView;
	g_root.m_filterView = m_filterView;
	g_root.m_logView = m_logView;
	g_root.m_resultView = m_resultView;
	g_root.m_boxView = m_boxView;
	g_root.m_camView = m_camView;
	g_root.m_calibView = m_calibView;
	g_root.m_aniView = m_aniView;

	g_root.DisconnectSensor();

	m_gui.SetContext();

	float col_main_hue = 0.0f / 255.0f;
	float col_main_sat = 0.0f / 255.0f;
	float col_main_val = 80.0f / 255.0f;

	float col_area_hue = 0.0f / 255.0f;
	float col_area_sat = 0.0f / 255.0f;
	float col_area_val = 50.0f / 255.0f;

	float col_back_hue = 0.0f / 255.0f;
	float col_back_sat = 0.0f / 255.0f;
	float col_back_val = 35.0f / 255.0f;

	float col_text_hue = 0.0f / 255.0f;
	float col_text_sat = 0.0f / 255.0f;
	float col_text_val = 255.0f / 255.0f;

	float col_btn_hue = 40.0f / 255.0f;
	float col_btn_sat = 240.0f / 255.0f;
	float col_btn_val = 120.0f / 255.0f;

	float frameRounding = 0.0f;

	ImVec4 col_text = ImColor::HSV(col_text_hue, col_text_sat, col_text_val);
	ImVec4 col_main = ImColor::HSV(col_main_hue, col_main_sat, col_main_val);
	ImVec4 col_area = ImColor::HSV(col_area_hue, col_area_sat, col_area_val);
	ImVec4 col_back = ImColor::HSV(col_back_hue, col_back_sat, col_back_val);
	ImVec4 col_btn = ImColor::HSV(col_btn_hue, col_btn_sat, col_btn_val);
	float rounding = frameRounding;

	ImGuiStyle& style = ImGui::GetStyle();
	//style.FrameBorderSize = 0.f;
	//style.ChildBorderSize = 0.f;
	style.FrameRounding = rounding;
	style.WindowRounding = rounding;
	style.Colors[ImGuiCol_Text] = ImVec4(col_text.x, col_text.y, col_text.z, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(col_text.x, col_text.y, col_text.z, 0.58f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(col_back.x, col_back.y, col_back.z, 0.80f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	//style.Colors[ImGuiCol_Border] = ImVec4(col_text.x, col_text.y, col_text.z, 0.30f);
	style.Colors[ImGuiCol_Border] = ImVec4(col_text.x, col_text.y, col_text.z, 0.30f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(col_back.x, col_back.y, col_back.z, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.68f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(col_main.x, col_main.y, col_main.z, 0.31f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.78f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	//style.Colors[ImGuiCol_ComboBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(col_text.x, col_text.y, col_text.z, 0.80f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(col_main.x, col_main.y, col_main.z, 0.54f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(col_btn.x, col_btn.y, col_btn.z, 0.44f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(col_btn.x, col_btn.y, col_btn.z, 0.86f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(col_btn.x, col_btn.y, col_btn.z, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(col_main.x, col_main.y, col_main.z, 0.76f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.86f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(col_text.x, col_text.y, col_text.z, 0.32f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(col_text.x, col_text.y, col_text.z, 0.78f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(col_text.x, col_text.y, col_text.z, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(col_main.x, col_main.y, col_main.z, 0.20f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.78f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	//style.Colors[ImGuiCol_CloseButton] = ImVec4(col_text.x, col_text.y, col_text.z, 0.16f);
	//style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(col_text.x, col_text.y, col_text.z, 0.39f);
	//style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(col_text.x, col_text.y, col_text.z, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(col_text.x, col_text.y, col_text.z, 0.63f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(col_text.x, col_text.y, col_text.z, 0.63f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(col_main.x, col_main.y, col_main.z, 0.43f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.10f, 0.10f, 0.10f, 0.55f);

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
	//g_root.Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.code)
		{
		case sf::Keyboard::Escape: close(); break;
		}
		break;
	}
}


void cViewer::OnShutdown()
{
	g_root.Clear();
}