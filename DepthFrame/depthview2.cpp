
#include "stdafx.h"
#include "depthview2.h"

using namespace graphic;
using namespace framework;

cDepthView2::cDepthView2(const string &name)
	: framework::cDockWindow(name)
	, m_thresholdMin(0)
	, m_thresholdMax(50000)
{
}

cDepthView2::~cDepthView2()
{
}


bool cDepthView2::Init(graphic::cRenderer &renderer)
{
	m_depthTexture.Create(renderer, g_baslerDepthWidth, g_baslerDepthHeight, DXGI_FORMAT_R32_FLOAT);

	return true;
}


void cDepthView2::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image(m_depthTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

	// HUD
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(std::min(m_rect.Width()-15.f, 500.f), 250));
	if (ImGui::Begin("DepthView2 Info", &isOpen, ImVec2(std::min(m_rect.Width()-15.f, 500.f), 250.f), windowAlpha, flags))
	{
		//ImGui::DragInt("Depth Threshold Min", &g_root.m_depthThresholdMin, 10, 0, USHORT_MAX);
		//ImGui::DragInt("Depth Threshold Max", &g_root.m_depthThresholdMax, 10, 1, USHORT_MAX);
		bool isUpdate = false;
		if (ImGui::DragInt("Threshold Min", &m_thresholdMin, 100, 0, USHORT_MAX))
			isUpdate = true;
		if (ImGui::DragInt("Threshold Max", &m_thresholdMax, 100, 1, USHORT_MAX))
			isUpdate = true;

		m_thresholdMin = std::min(m_thresholdMin, m_thresholdMax);
		m_thresholdMax = std::max(m_thresholdMin, m_thresholdMax);

		if (isUpdate)
		{
			ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_depthBuff2[0]
				, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
				, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
		}

		ImGui::End();
	}
}


void cDepthView2::Process()
{
	ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_depthBuff2[0]
		, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
		, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
}


void cDepthView2::ProcessDepth(INT64 nTime
	, const UINT16* pBuffer
	, int nWidth
	, int nHeight
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	// Make sure we've received valid data
	if ((nWidth != m_depthTexture.Width()) || (nHeight != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), nWidth, nHeight, DXGI_FORMAT_R32_FLOAT);
	}

	// Update Texture
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_depthTexture.Lock(renderer, map))
	{
		for (int i = 0; i < nHeight; ++i)
		{
			for (int k = 0; k < nWidth; ++k)
			{
				USHORT depth = pBuffer[i * nWidth + k];
				BYTE *p = dst + (i * map.RowPitch) + (k * 4);
				*(float*)p = std::max(0.f, (float)(depth - m_thresholdMin) / (float)(m_thresholdMax - m_thresholdMin));
			}
		}

		m_depthTexture.Unlock(renderer);
	}
}
