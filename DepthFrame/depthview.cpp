
#include "stdafx.h"
#include "depthview.h"

using namespace graphic;
using namespace framework;

cDepthView::cDepthView(const string &name)
	: framework::cDockWindow(name)
{
}

cDepthView::~cDepthView()
{
}


bool cDepthView::Init(graphic::cRenderer &renderer)
{
	//m_depthTexture.Create(renderer, g_baslerDepthWidth, g_baslerDepthHeight);
	m_depthTexture.Create(renderer, g_baslerDepthWidth, g_baslerDepthHeight, DXGI_FORMAT_R32_FLOAT);

	return true;
}


void cDepthView::OnRender(const float deltaSeconds)
{
	if (g_root.m_isUpdate)
	{
		ProcessDepth(g_root.m_nTime, &g_root.m_sensorBuff.m_depthBuff[0]
			, g_root.m_sensorBuff.m_width, g_root.m_sensorBuff.m_height
			, g_root.m_nDepthMinReliableDistance, g_root.m_nDepthMaxDistance);
	}

	ImGui::Image(m_depthTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));
}


void cDepthView::ProcessDepth(INT64 nTime
	, const UINT16* pBuffer
	, int nWidth
	, int nHeight
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	if ((nWidth != m_depthTexture.Width()) || (nHeight != m_depthTexture.Height()))
	{
		m_depthTexture.Create(GetRenderer(), nWidth, nHeight, DXGI_FORMAT_R32_FLOAT);
	}

	// Make sure we've received valid data
	//if (pBuffer && (nWidth == g_kinectDepthWidth) && (nHeight == g_kinectDepthHeight))
	{
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
					//*(float*)p = max(0, (float)(depth - g_root.m_depthThresholdMin) / (g_root.m_depthThresholdMax - g_root.m_depthThresholdMin));
					*(float*)p = max(0, abs((float)(depth - 4000.f) / 4000.f));


					//USHORT depth = pBuffer[i * nWidth + k];
					//BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);

					//BYTE *p = dst + (i * map.RowPitch) + (k * 4);
					//*p = intensity;
					//*(p + 1) = intensity;
					//*(p + 2) = intensity;
					//*(p + 3) = 0xFF;
				}
			}

			m_depthTexture.Unlock(renderer);
		}
	}
}

