
#include "stdafx.h"
#include "filterview.h"

using namespace graphic;
using namespace framework;
using namespace cv;
using namespace common;


cFilterView::cFilterView(const string &name)
	: framework::cDockWindow(name)
{
}

cFilterView::~cFilterView()
{
}


bool cFilterView::Init(graphic::cRenderer &renderer)
{
	m_depthTexture.Create(renderer, (int)g_capture3DWidth, (int)g_capture3DHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

	return true;
}


void cFilterView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::Image(m_depthTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

	// HUD
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(std::min(m_rect.Width() - 15.f, 800.f), m_rect.Height()));
	ImGui::SetNextWindowBgAlpha(windowAlpha);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	if (ImGui::Begin("FilterView Info", &isOpen, flags))
	{
		ImGui::Checkbox("Box", &g_root.m_isShowBox);
		ImGui::SameLine();
		ImGui::Checkbox("Box Candidate", &g_root.m_isShowBoxCandidate);
		ImGui::SameLine();
		ImGui::Checkbox("Vertex", &g_root.m_isShowBoxVertex);

		ImGui::Spacing();
		ImGui::Separator();
		for (u_int i = 0; i < g_root.m_measure.m_boxes.size(); ++i)
		{
			auto &box = g_root.m_measure.m_boxes[i];
			ImGui::Text("Box%d", i + 1);
			ImGui::Text("\t X = %f", box.volume.x);
			ImGui::Text("\t Y = %f", box.volume.z);
			ImGui::Text("\t H = %f", box.volume.y);
			ImGui::Text("\t V/W = %f", box.minVolume / 6000.f);
			ImGui::Spacing();
			ImGui::Separator();
		}
		ImGui::End();
	}
	ImGui::PopStyleColor();
}


void cFilterView::Process()
{
	ProcessDepth();
}


void cFilterView::ProcessDepth()
{
	cv::Mat &srcImg = g_root.m_measure.m_projMap;
	if ((srcImg.cols != m_depthTexture.Width()) || (srcImg.rows != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), srcImg.cols, srcImg.rows, DXGI_FORMAT_R32G32B32_FLOAT);
	}

	UpdateTexture();
}


// Mat -> DX11 Texture
void cFilterView::UpdateTexture()
{
	const int nWidth = m_depthTexture.Width();
	const int nHeight = m_depthTexture.Height();

	Mat &dstImg = g_root.m_measure.m_dstImg;
	BYTE *src = (BYTE*)dstImg.data;
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_depthTexture.Lock(renderer, map))
	{
		for (int i = 0; i < nHeight; ++i)
		{
			for (int k = 0; k < nWidth; ++k)
			{
				const BYTE v1 = src[i * dstImg.step[0] + k * 3];
				const BYTE v2 = src[i * dstImg.step[0] + k * 3 + 1];
				const BYTE v3 = src[i * dstImg.step[0] + k * 3 + 2];
				BYTE *p = dst + (i * map.RowPitch) + (k * sizeof(float) * 4);

				*(float*)p = v1 / 255.f;
				*(float*)(p + sizeof(float) * 1) = v2 / 255.f;
				*(float*)(p + sizeof(float) * 2) = v3 / 255.f;
				*(float*)(p + sizeof(float) * 3) = 1.f;
			}
		}

		m_depthTexture.Unlock(renderer);
	}
}
