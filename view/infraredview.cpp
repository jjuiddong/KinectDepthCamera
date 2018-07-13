
#include "stdafx.h"
#include "infraredview.h"

using namespace graphic;
using namespace framework;

cInfraredView::cInfraredView(const string &name)
	: framework::cDockWindow(name)
{
}

cInfraredView::~cInfraredView()
{
}


bool cInfraredView::Init(graphic::cRenderer &renderer)
{
	m_infraredTexture.Create(renderer, g_baslerColorWidth, g_baslerColorHeight, DXGI_FORMAT_R32_FLOAT);
	return true;
}


void cInfraredView::OnRender(const float deltaSeconds)
{
	//UpdateInfraredImage();
	ImGui::Image(m_infraredTexture.m_texSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 50));
}


void cInfraredView::Process(const size_t camIdx //=0
)
{
	RET(g_root.m_baslerCam.m_sensors.size() <= camIdx);

	cSensor *sensor = g_root.m_baslerCam.m_sensors[camIdx];
	ProcessInfrared(&sensor->m_buffer.m_intensity[0]
		, sensor->m_buffer.m_width, sensor->m_buffer.m_height);
}


void cInfraredView::UpdateKinectInfraredImage()
{
	if (!g_root.m_kinect.IsConnect())
		return;

	IInfraredFrame* pInfraredFrame = NULL;
	HRESULT hr = g_root.m_kinect.m_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);
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

	ProcessInfrared(pBuffer, nWidth, nHeight);


error:
	SAFE_RELEASE(pFrameDescription);
	SAFE_RELEASE(pInfraredFrame);
}


// Update Infrared Texture
void cInfraredView::ProcessInfrared(const UINT16* pBuffer, int nWidth, int nHeight)
{
	cRenderer &renderer = GetRenderer();
	D3D11_MAPPED_SUBRESOURCE map;
	if (BYTE *dst = (BYTE*)m_infraredTexture.Lock(renderer, map))
	{
		for (int i = 0; i < nHeight; ++i)
		{
			for (int k = 0; k < nWidth; ++k)
			{
				float *p = (float*)(dst + (i * map.RowPitch) + k * sizeof(float));
				*p = (float)(pBuffer[i * nWidth + k]) / USHRT_MAX;
			}
		}

		m_infraredTexture.Unlock(renderer);
	}
}
