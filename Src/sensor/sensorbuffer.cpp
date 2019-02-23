
#include "stdafx.h"
#include "sensorbuffer.h"

using namespace graphic;


cSensorBuffer::cSensorBuffer()
	: m_width(0)
	, m_height(0)
	, m_isLoaded(false)
	, m_time(0)
	, m_frameId(0)
	, m_isUpdatePointCloud(true)
	, m_mergeOffset(false)
{
	ZeroMemory(&m_diffAvrs, sizeof(m_diffAvrs));
}

cSensorBuffer::~cSensorBuffer()
{
	Clear();
}


void cSensorBuffer::Render(graphic::cRenderer &renderer
	, const char *techniqName //= "Unlit"
	, const bool isAphablend //= false
	, const XMMATRIX &parentTm //= XMIdentity
)
{
	cShader11 *shader = renderer.m_shaderMgr.FindShader(eVertexType::POSITION);
	assert(shader);
	shader->SetTechnique(techniqName);
	shader->Begin();
	shader->BeginPass(renderer, 0);

	renderer.m_cbPerFrame.m_v->mWorld = XMMatrixTranspose(parentTm);
	renderer.m_cbPerFrame.Update(renderer);
	XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&common::Vector4(.7f, .7f, .7f, isAphablend? .4f : 1.f));
	renderer.m_cbMaterial.m_v->diffuse = diffuse;
	renderer.m_cbMaterial.Update(renderer, 2);

	m_vtxBuff.Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

	if (isAphablend)
	{
		CommonStates states(renderer.GetDevice());
		renderer.GetDevContext()->OMSetBlendState(states.NonPremultiplied(), 0, 0xffffffff);
		renderer.GetDevContext()->DrawInstanced(m_pointCloudCount, 1, 0, 0);
		renderer.GetDevContext()->OMSetBlendState(NULL, 0, 0xffffffff);
	}
	else
	{
		renderer.GetDevContext()->DrawInstanced(m_pointCloudCount, 1, 0, 0);
	}
}


// Tessellation 으로 Point Cloud를 출력한다.
// 해상도를 높이기 위해서, X-Z평면으로 Quad를 펼쳐서 Point를 확대해서 출력한다.
void cSensorBuffer::RenderTessellation(graphic::cRenderer &renderer
	, const XMMATRIX &parentTm //= graphic::XMIdentity
)
{
	if (!m_shader.m_effect)
	{
		if (!m_shader.Create(renderer, "../media/shader11/tess-pos.fxo", "Unlit", eVertexType::POSITION))
			return;
	}

	cShader11 *shader = &m_shader;
	assert(shader);
	shader->SetTechnique("Unlit");
	shader->Begin();
	shader->BeginPass(renderer, 0);

	renderer.m_cbPerFrame.m_v->mWorld = XMMatrixTranspose(parentTm);
	renderer.m_cbPerFrame.Update(renderer);
	XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&common::Vector4(.7f, .7f, .7f, 1.f));
	renderer.m_cbMaterial.m_v->diffuse = diffuse;
	renderer.m_cbMaterial.Update(renderer, 2);
	renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * 0.56f * 2.f;// 3.f;
	renderer.m_cbTessellation.Update(renderer, 6);

	m_vtxBuff.Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	renderer.GetDevContext()->Draw(m_pointCloudCount, 0);
}


bool cSensorBuffer::ReadKinectSensor(cRenderer &renderer
	, INT64 nTime
	, const USHORT* pBuffer
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	const int w = g_kinectDepthWidth;
	const int h = g_kinectDepthHeight;
	if (m_intensity.size() != (w * h))
	{
		m_pointCloudCount = 0;
		m_width = w;
		m_height = h;
		m_vertices.resize(w * h);
		m_intensity.resize(w * h);
		m_vtxBuff.Clear();
		m_vtxBuff.Create(renderer, w * h, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	}

	memcpy(&m_intensity[0], pBuffer, sizeof(USHORT) * w * h);

	ProcessKinectDepthBuff(renderer, nTime, pBuffer, nMinDepth, nMaxDepth);

	m_isLoaded = true;
	return true;
}


bool cSensorBuffer::ReadPlyFile(cRenderer &renderer, const string &fileName)
{
	cPlyReader reader;
	if (!reader.Read(fileName))
		return false;

	UpdatePointCloud(renderer, reader.m_vertices, {}, {});

	m_isLoaded = true;
	m_frameId++;
	return true;
}


bool cSensorBuffer::ReadDatFile(cRenderer &renderer, const string &fileName)
{
	m_isLoaded = false;
	cDatReader reader;
	if (!reader.Read(fileName.c_str()))
		return false;

	ReadDatFile(renderer, reader);
	
	return true;
}


bool cSensorBuffer::ReadDatFile(graphic::cRenderer &renderer, const cDatReader &reader)
{
	UpdatePointCloud(renderer, reader.m_vertices, reader.m_intensity, reader.m_confidence);

	m_isLoaded = true;
	m_time = reader.m_time;
	m_frameId++;

	return true;
}


// initialize buffer data to Zero
void cSensorBuffer::Zeros(graphic::cRenderer &renderer)
{
	InitBuffer(renderer);
	
	for (auto &vtx : m_srcVertices)
		vtx = Vector3(0, 0, 0);
	for (auto &vtx : m_vertices)
		vtx = Vector3(0, 0, 0);

	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		for (int i = 0; i < m_height; ++i)
			for (int k = 0; k < m_width; ++k)
				dst++->p = Vector3(0,0,0);
		m_vtxBuff.Unlock(renderer);
	}
}


void cSensorBuffer::InitBuffer(graphic::cRenderer &renderer)
{
	const int w = g_baslerDepthWidth;
	const int h = g_baslerDepthHeight;
	if (m_intensity.size() != (w * h))
	{
		m_pointCloudCount = 0;
		m_width = w;
		m_height = h;
		m_vertices.resize(w * h);
		m_intensity.resize(w * h);
		m_confidence.resize(w * h);
		m_vtxBuff.Clear();
		m_vtxBuff.Create(renderer, w * h, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	}
}


// Update, verext buffer, intensity buffer, confidence buffer
bool cSensorBuffer::UpdatePointCloud(cRenderer &renderer
	, const vector<Vector3> &vertices
	, const vector<unsigned short> &intensity
	, const vector<unsigned short> &confidence
)
{
	InitBuffer(renderer);

	// Copy Vertices
	{
		if (m_srcVertices.size() != vertices.size())
			m_srcVertices.resize(vertices.size());
		const Vector3 *src = &vertices[0];
		Vector3 *dst = &m_srcVertices[0];
		const u_int vertexSize = vertices.size();
		for (u_int i = 0; i < vertexSize; ++i)
			*dst++ = *src++;
	}

	memcpy(&m_intensity[0], &intensity[0], intensity.size() * sizeof(intensity[0]));
	memcpy(&m_confidence[0], &confidence[0], confidence.size() * sizeof(confidence[0]));

	UpdatePointCloudAllConfig(renderer);
	UpdatePointCloudBySelf(renderer);

	//if (m_vertices.size() > (u_int)m_pointCloudCount)
	//{
	//	Vector3 *dst = &m_vertices[m_pointCloudCount];
	//	for (u_int i = (u_int)m_pointCloudCount; i < m_vertices.size(); ++i)
	//		*dst++ = Vector3(0, 0, 0);
	//}

	return true;
}


bool cSensorBuffer::UpdatePointCloudBySelf(graphic::cRenderer &renderer)
{
	// Update Point Cloud
	m_pointCloudCount = 0;
	if (!m_isUpdatePointCloud)
		return false;

	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		Vector3 *src = &m_vertices[0];
		const u_int vertexSize = m_vertices.size();
		for (u_int i = 0; i < vertexSize; ++i)
		{
			const Vector3 &pos = *src++;

			if (pos.IsEmpty())
				continue;

			if (g_root.m_isRangeCulling)
			{
				if (pos.x < g_root.m_cullRangeMin.x)
					continue;
				if (pos.y < g_root.m_cullRangeMin.y)
					continue;
				if (pos.z < g_root.m_cullRangeMin.z)
					continue;
				if (pos.x > g_root.m_cullRangeMax.x)
					continue;
				if (pos.y > g_root.m_cullRangeMax.y)
					continue;
				if (pos.z > g_root.m_cullRangeMax.z)
					continue;
			}

			dst->p = pos;
			++dst;
			++m_pointCloudCount;
		}

		m_vtxBuff.Unlock(renderer);
	}

	return true;
}


// configuration 정보를 적용해서 포인트 클라우드를 업데이트 한다.
bool cSensorBuffer::UpdatePointCloudAllConfig(graphic::cRenderer &renderer)
{
	Transform tfm;
	tfm.scale = Vector3(1, 1, 1)*0.1f; // basler값은 1mm 단위이므로, 1cm 단위로 바꾸기위해서 0.1을 곱한다.
	Matrix44 tm = tfm.GetMatrix();

	// plane calculation
	if (!g_root.m_groundPlane.N.IsEmpty())
	{
		Quaternion q;
		q.SetRotationArc(g_root.m_groundPlane.N, Vector3(0, 1, 0));
		tm *= q.GetMatrix();

		Vector3 center = g_root.m_volumeCenter * q.GetMatrix();
		center.y = 0;

		Matrix44 T;
		T.SetPosition(Vector3(-center.x, g_root.m_groundPlane.D, -center.z));

		tm *= T;
	}

	{
		Quaternion q;
		q.SetRotationArc(m_planeSub.N, Vector3(0, 1, 0));
		tm *= q.GetMatrix();

		Matrix44 T;
		T.SetPosition(Vector3(0, m_planeSub.D, 0));

		tm *= T;
	}

	tm *= m_offset.GetMatrix();

	//const float f = (75.290f - 78.090f) / 120.f;

	const bool isCull = !m_cullRect.IsEmpty();

	float maxDiff = 0.f;
	float diffAvrs = 0;
	Vector3 *srcVtx = &m_srcVertices[0];
	Vector3 *dstVtx = &m_vertices[0];
	const u_int vertexSize = m_srcVertices.size();
	for (u_int i = 0; i < vertexSize; ++i)
	{
		Vector3 pos = *srcVtx * tm;

		if (isCull) // Culling Enable?
		{
			if (!isnan(pos.x) && !isnan(srcVtx->x))
			{
				if (m_cullRect.IsIn(pos.x, pos.z)
					&& !m_cullExtraRect.IsIn(pos.x, pos.z))
				{
					// Calc Max Difference
					const float diff = abs(pos.y - dstVtx->y);
					if (!dstVtx->IsEmpty())
					{
						diffAvrs += diff;
						if (diff > maxDiff)
						{
							maxDiff = diff;
						}
					}
				}
				else
				{
					pos = Vector3(0, 0, 0);
				}
			}
			else
			{
				pos = Vector3(0, 0, 0);
			}
		}
		else
		{
			if (!isnan(pos.x) && !isnan(srcVtx->x))
			{
				const float diff = abs(pos.y - dstVtx->y);
				if (!dstVtx->IsEmpty())
				{
					diffAvrs += diff;
					if (diff > maxDiff)
					{
						maxDiff = diff;
					}
				}
			}
			else
			{
				pos = Vector3(0, 0, 0);
			}
		}

		// 높이에 따라, z축으로 줄어든다. (Right 카메라)
		//if (m_mergeOffset)
		//{
		//	pos.z += f * (pos.y - 30.f);
		//}

		*dstVtx = pos;
		++dstVtx;
		++srcVtx;
	}
	diffAvrs /= (float)m_vertices.size();
	m_diffAvrs.AddValue(diffAvrs);

	return true;
}


bool cSensorBuffer::ProcessKinectDepthBuff(cRenderer &renderer
	, INT64 nTime
	, const USHORT* pBuffer
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	for (int i = 0; i < m_height; ++i)
	{
		for (int k = 0; k < m_width; ++k)
		{
			const Vector3 pos = Get3DPos(k, i, nMinDepth, nMaxDepth);
			m_vertices[i*m_width + k] = pos + Vector3(0,10,0);
		}
	}

	// Update Point Cloud
	m_pointCloudCount = 0;
	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		int cnt = 0;

		for (int i = 0; i < m_width; ++i)
		{
			for (int k = 0; k < m_height; ++k)
			{
				const Vector3 p1 = GetVertex(k, i);
				dst->p = p1;
				m_vertices[cnt++] = p1;

				++dst;
				++m_pointCloudCount;
			}
		}

		m_vtxBuff.Unlock(renderer);

		for (u_int i = (u_int)cnt; i < m_vertices.size(); ++i)
			m_vertices[i] = Vector3(0, 0, 0);
	}

	return true;
}


inline Vector3 cSensorBuffer::Get3DPos(const int x, const int y, USHORT nMinDepth, USHORT nMaxDepth)
{
	if ((x < 0) || (x >= m_width)
		|| (y < 0) || (y >= m_height))
		return Vector3(0, 0, 0);

	const USHORT depth = m_intensity[y * m_width + x];
	if (depth < nMinDepth)
		return Vector3();

	const float w = 10.f;
	const float h = ((float)m_height / (float)m_width) * w;
	const float d = (float)depth / (float)nMaxDepth;

	// x = -w ~ +w
	// y = -h ~ +h
	const float x0 = ((float)x - (m_width / 2.f)) / (m_width / 2.f) * w;
	const float y0 = -((float)y - (m_height / 2.f)) / (m_height / 2.f) * h;
	const Vector3 p0(x0, y0, 10);
	const Vector3 p1 = p0 * 50.f;
	const float len = p0.Distance(p1);

	const Vector3 p2 = p0.Normal() * (d * len);

	return p2;
}


inline common::Vector3 cSensorBuffer::GetVertex(const int x, const int y)
{
	if ((x < 0) || (x >= m_width)
		|| (y < 0) || (y >= m_height))
		return Vector3(0, 0, 0);

	return m_vertices[y*m_width + x];
}


Vector3 cSensorBuffer::PickVertex(const Ray &ray)
{
	Vector3 mostNearVertex;
	float maxDot = 0;

	for (int i=0; i < m_pointCloudCount; ++i)
	{
		auto &vtx = m_vertices[i];
		const Vector3 p = vtx;
		const Vector3 v = (p - ray.orig).Normal();
		const float d = abs(ray.dir.DotProduct(v));
		if (maxDot < d)
		{
			maxDot = d;
			mostNearVertex = p;
		}
	}

	return mostNearVertex;
}


// sensor plane, volume center space change
void cSensorBuffer::ChangeSpace(cRenderer &renderer)
{
	Quaternion q;
	q.SetRotationArc(g_root.m_groundPlane.N, Vector3(0, 1, 0));
	const Matrix44 tm = q.GetMatrix();

	Vector3 center = g_root.m_volumeCenter * tm;
	center.y = 0;

	for (u_int i = 0; i < m_vertices.size(); ++i)
	{
		Vector3 pos = m_vertices[i];
		m_vertices[i] = pos * tm;
		m_vertices[i].y += g_root.m_groundPlane.D;
		m_vertices[i].x -= center.x;
		m_vertices[i].z -= center.z;
	}

	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		for (u_int i = 0; i < m_vertices.size(); ++i)
		{
			dst->p = m_vertices[i];
			++dst;
		}
		m_vtxBuff.Unlock(renderer);
	}
}


void cSensorBuffer::AnalysisDepth()
{
	// Analyze m_depthBuff
	int buff1[50000];
	ZeroMemory(buff1, sizeof(buff1));
	for (u_int i = 0; i < m_intensity.size(); ++i)
	{
		if (ARRAYSIZE(buff1) > m_intensity[i])
			++buff1[m_intensity[i]];
	}

	ZeroMemory(&m_analysis1, sizeof(m_analysis1));
	for (u_int i = 0; i < ARRAYSIZE(buff1); ++i)
		m_analysis1.AddValue((float)buff1[i]);

	// Analyze m_depthBuff2
	int buff2[50000];
	ZeroMemory(buff2, sizeof(buff2));
	for (u_int i = 0; i < m_confidence.size(); ++i)
	{
		if (ARRAYSIZE(buff1) > m_confidence[i])
			++buff2[m_confidence[i]];
	}

	ZeroMemory(&m_analysis2, sizeof(m_analysis2));
	for (u_int i = 0; i < ARRAYSIZE(buff2); ++i)
		m_analysis2.AddValue((float)buff2[i]);
}


void cSensorBuffer::Clear()
{
	m_vtxBuff.Clear();
}
