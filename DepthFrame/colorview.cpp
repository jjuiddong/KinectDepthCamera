
#include "stdafx.h"
#include "colorview.h"

using namespace graphic;
using namespace framework;

cColorView::cColorView(const string &name)
	: framework::cDockWindow(name)
	, m_colorBuffer(NULL)
{
}

cColorView::~cColorView()
{
	SAFE_DELETEA(m_colorBuffer);
}


bool cColorView::Init(graphic::cRenderer &renderer)
{
	m_colorTexture.Create(renderer, g_kinectColorWidth, g_kinectColorHeight);
	return true;
}


void cColorView::OnRender(const float deltaSeconds)
{
	UpdateColorImage();
	ImGui::Image(m_colorTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));
}


// Update Kinect ColorBuffer
void cColorView::UpdateColorImage()
{
	if (!g_root.m_kinect.m_pColorFrameReader)
		return;

	IColorFrame* pColorFrame = NULL;
	HRESULT hr = g_root.m_kinect.m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	if (FAILED(hr))
		return;

	INT64 nTime = 0;
	IFrameDescription* pFrameDescription = NULL;
	int nHeight = 0;
	int nWidth = 0;
	ColorImageFormat imageFormat;
	UINT nBufferSize = 0;
	//BYTE *pBuffer = NULL;

	hr = pColorFrame->get_RelativeTime(&nTime);
	if (FAILED(hr))
		goto error;

	hr = pColorFrame->get_FrameDescription(&pFrameDescription);
	if (FAILED(hr))
		goto error;

	hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Width(&nWidth);
	if (FAILED(hr))
		goto error;

	hr = pFrameDescription->get_Height(&nHeight);
	if (FAILED(hr))
		goto error;

	nBufferSize = nWidth * nHeight * 4;
	if (!m_colorBuffer)
	{
		m_colorBuffer = new BYTE[nBufferSize]; // RGBA
	}

	hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, m_colorBuffer, ColorImageFormat_Rgba);
	if (FAILED(hr))
		goto error;

	ProcessColor(nTime, m_colorBuffer, nWidth, nHeight);

error:
	SAFE_RELEASE(pFrameDescription);
	SAFE_RELEASE(pColorFrame);
}


void cColorView::ProcessColor(INT64 nTime, const BYTE* pBuffer, int nWidth, int nHeight)
{
	// Update Texture
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_colorTexture.Lock(renderer, map))
	{
		for (int i = 0; i < g_kinectColorHeight; ++i)
		{
			for (int k = 0; k < g_kinectColorWidth; ++k)
			{
				BYTE *p = dst + (i * map.RowPitch) + (k * 4);
				*p = pBuffer[i * (g_kinectColorWidth * 4) + (k * 4)];
				*(p + 1) = pBuffer[i * (g_kinectColorWidth * 4) + (k * 4) + 1];
				*(p + 2) = pBuffer[i * (g_kinectColorWidth * 4) + (k * 4) + 2];
				*(p + 3) = 0xFF;
			}
		}

		m_colorTexture.Unlock(renderer);
	}
}
