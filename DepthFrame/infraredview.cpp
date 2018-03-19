
#include "stdafx.h"
#include "infraredview.h"

using namespace graphic;
using namespace framework;

cInfraredView::cInfraredView(const string &name)
	: framework::cDockWindow(name)
	, m_infraredBuffer(NULL)
{
}

cInfraredView::~cInfraredView()
{
	SAFE_DELETEA(m_infraredBuffer);
}


bool cInfraredView::Init(graphic::cRenderer &renderer)
{
	m_infraredTexture.Create(renderer, cInfraredWidth, cInfraredHeight, DXGI_FORMAT_R32_FLOAT);
	return true;
}


void cInfraredView::OnRender(const float deltaSeconds)
{
	UpdateInfraredImage();
	ImGui::Image(m_infraredTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));

}


void cInfraredView::UpdateInfraredImage()
{
	if (!g_root.m_isUpdate)
		return;
	if (!g_root.m_pInfraredFrameReader)
		return;

	IInfraredFrame* pInfraredFrame = NULL;
	HRESULT hr = g_root.m_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);
	if (FAILED(hr))
		return;

	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nHeight = 0;
	int nWidth = 0;
	UINT nBufferSize = 0;
	UINT16 *pBuffer = NULL;

	hr = pInfraredFrame->get_RelativeTime(&nTime);
	if (FAILED(hr))
		goto error;

	hr = pInfraredFrame->get_FrameDescription(&pFrameDescription);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Width(&nWidth);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Height(&nHeight);
	if (FAILED(hr))
		goto error;

	hr = pInfraredFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
	if (FAILED(hr))
		goto error;

	ProcessInfrared(nTime, pBuffer, nWidth, nHeight);


error:
	SAFE_RELEASE(pFrameDescription);
	SAFE_RELEASE(pInfraredFrame);
}


// Update Infrared Texture
void cInfraredView::ProcessInfrared(INT64 nTime, const UINT16* pBuffer, int nWidth, int nHeight)
{
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_infraredTexture.Lock(renderer, map))
	{
		for (int i = 0; i < cInfraredHeight; ++i)
		{
			for (int k = 0; k < cInfraredWidth; ++k)
			{
				float *p = (float*)(dst + (i * map.RowPitch) + k * sizeof(float));
				*p = (float)(pBuffer[i * cInfraredWidth + k]) / USHRT_MAX;
			}
		}

		m_infraredTexture.Unlock(renderer);
	}
}
