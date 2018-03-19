
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
	m_depthTexture.Create(renderer, cDepthWidth, cDepthHeight);

	return true;
}


void cDepthView::OnRender(const float deltaSeconds)
{
	if (g_root.m_isUpdate)
	{
		ProcessDepth(g_root.m_nTime, g_root.m_pDepthBuff
			, cDepthWidth, cDepthHeight
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
	// Make sure we've received valid data
	if (pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
	{
		// Update Texture
		cRenderer &renderer = GetRenderer();
		D3D11_MAPPED_SUBRESOURCE map;
		if (BYTE *dst = (BYTE*)m_depthTexture.Lock(renderer, map))
		{
			for (int i = 0; i < cDepthHeight; ++i)
			{
				for (int k = 0; k < cDepthWidth; ++k)
				{
					USHORT depth = pBuffer[i * cDepthWidth + k];
					BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);

					BYTE *p = dst + (i * map.RowPitch) + (k * 4);
					*p = intensity;
					*(p + 1) = intensity;
					*(p + 2) = intensity;
					*(p + 3) = 0xFF;
				}
			}

			m_depthTexture.Unlock(renderer);
		}
	}
}

