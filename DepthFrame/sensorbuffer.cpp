
#include "stdafx.h"
#include "sensorbuffer.h"
#include "plyreader.h"
#include "datreader.h"

using namespace graphic;


cSensorBuffer::cSensorBuffer()
	: m_width(0)
	, m_height(0)
	, m_plane(Vector3(0, 0, 0), 0)
{
	ZeroMemory(&m_diffAvrs, sizeof(m_diffAvrs));
	m_srcImg = cv::Mat(480, 640, CV_32FC1);
}

cSensorBuffer::~cSensorBuffer()
{
	Clear();
}


void cSensorBuffer::Render(graphic::cRenderer &renderer
	, const char *techniqName //= "Unlit"
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
	XMVECTOR diffuse = XMLoadFloat4((XMFLOAT4*)&common::Vector4(.7f, .7f, .7f, 1.f));
	renderer.m_cbMaterial.m_v->diffuse = diffuse;
	renderer.m_cbMaterial.Update(renderer, 2);

	m_vtxBuff.Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	renderer.GetDevContext()->DrawInstanced(m_pointCloudCount, 1, 0, 0);
}


bool cSensorBuffer::ReadKinectSensor(cRenderer &renderer
	, INT64 nTime
	, const USHORT* pBuffer
	, USHORT nMinDepth
	, USHORT nMaxDepth)
{
	const int w = g_kinectDepthWidth;
	const int h = g_kinectDepthHeight;
	if (m_depthBuff.size() != (w * h))
	{
		m_pointCloudCount = 0;
		m_width = w;
		m_height = h;
		m_vertices.resize(w * h);
		m_colors.resize(w * h);
		m_depthBuff.resize(w * h);
		m_vtxBuff.Clear();
		m_vtxBuff.Create(renderer, w * h, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	}

	memcpy(&m_depthBuff[0], pBuffer, sizeof(USHORT) * w * h);

	ProcessKinectDepthBuff(renderer, nTime, pBuffer, nMinDepth, nMaxDepth);

	return true;
}


bool cSensorBuffer::ReadPlyFile(cRenderer &renderer, const string &fileName)
{
	const int w = g_baslerDepthWidth;
	const int h = g_baslerDepthHeight;
	if (m_depthBuff.size() != (w * h))
	{
		m_pointCloudCount = 0;
		m_width = w;
		m_height = h;
		m_vertices.resize(w * h);
		m_colors.resize(w * h);
		m_depthBuff.resize(w * h);
		m_depthBuff2.resize(w * h);
		m_vtxBuff.Clear();
		m_vtxBuff.Create(renderer, w * h, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	}

	cPlyReader reader;
	if (!reader.Read(fileName))
		return false;

	Transform tfm;
	tfm.scale = Vector3(1, 1, 1)*0.1f;
	Matrix44 tm = tfm.GetMatrix();

	// plane calculation
	if (!m_plane.N.IsEmpty())
	{
		Quaternion q;
		q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
		tm *= q.GetMatrix();

		Vector3 center = m_volumeCenter * q.GetMatrix();
		center.y = 0;

		Matrix44 T;
		T.SetPosition(Vector3(-center.x, m_plane.D, -center.z));

		tm *= T;
	}


	float diffAvrs = 0;
	for (u_int i = 0; i < reader.m_vertices.size(); ++i)
	{
		Vector3 pos = reader.m_vertices[i] * tm;

		if (!isnan(m_vertices[i].x) && !isnan(reader.m_vertices[i].x))
			diffAvrs += abs(pos.y - m_vertices[i].y);

		m_vertices[i] = pos;
	}
	diffAvrs /= (float)m_vertices.size();
	m_diffAvrs.AddValue(diffAvrs);

	for (u_int i = reader.m_vertices.size(); i < w*h; ++i)
		m_vertices[i] = Vector3(0,0,0);


	// Update Point Cloud
	m_pointCloudCount = 0;
	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		int cnt = 0;
		for (int i = 0; i < m_height; ++i)
		{
			for (int k = 0; k < m_width; ++k)
			{
				const Vector3 p1 = GetVertex(k, i);
				//const Vector3 p2 = GetVertex(k - 1, i);
				//const Vector3 p3 = GetVertex(k, i - 1);
				//const Vector3 p4 = GetVertex(k + 1, i);
				//const Vector3 p5 = GetVertex(k, i + 1);

				//const float l1 = p1.Distance(p2);
				//const float l2 = p1.Distance(p3);
				//const float l3 = p1.Distance(p4);
				//const float l4 = p1.Distance(p5);

				//const float maxDist = g_root.m_depthDensity;
				//if ((l1 > maxDist)
				//	|| (l2 > maxDist)
				//	|| (l3 > maxDist)
				//	|| (l4 > maxDist)
				//	)
				//	continue;

				if (p1.IsEmpty())
					continue;

				//dst->p = p1;
				//m_vertices[cnt++] = p1;

				//dst->p = m_vertices[cnt++];
				dst->p = p1;

				++cnt;
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


bool cSensorBuffer::ReadDatFile(cRenderer &renderer, const string &fileName)
{
	cDatReader reader;
	if (!reader.Read(fileName))
		return false;

	ReadDatFile(renderer, reader);
	
	return true;
}


bool cSensorBuffer::ReadDatFile(graphic::cRenderer &renderer, const cDatReader &reader)
{
	const int w = g_baslerDepthWidth;
	const int h = g_baslerDepthHeight;
	if (m_depthBuff.size() != (w * h))
	{
		m_pointCloudCount = 0;
		m_width = w;
		m_height = h;
		m_vertices.resize(w * h);
		m_colors.resize(w * h);
		m_depthBuff.resize(w * h);
		m_depthBuff2.resize(w * h);
		m_vtxBuff.Clear();
		m_vtxBuff.Create(renderer, w * h, sizeof(sVertex), D3D11_USAGE_DYNAMIC);
	}

	Transform tfm;
	tfm.scale = Vector3(1, 1, 1)*0.1f;
	Matrix44 tm = tfm.GetMatrix();

	// plane calculation
	if (!m_plane.N.IsEmpty())
	{
		Quaternion q;
		q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
		tm *= q.GetMatrix();

		Vector3 center = m_volumeCenter * q.GetMatrix();
		center.y = 0;

		Matrix44 T;
		T.SetPosition(Vector3(-center.x, m_plane.D, -center.z));

		tm *= T;
	}


	float diffAvrs = 0;
	for (u_int i = 0; i < reader.m_vertices.size(); ++i)
	{
		Vector3 pos = reader.m_vertices[i] * tm;

		if (!isnan(m_vertices[i].x) && !isnan(reader.m_vertices[i].x))
			diffAvrs += abs(pos.y - m_vertices[i].y);
	
		m_vertices[i] = pos;
	}
	diffAvrs /= (float)m_vertices.size();
	m_diffAvrs.AddValue(diffAvrs);

	memcpy(&m_depthBuff[0], &reader.m_intensity[0], reader.m_intensity.size() * sizeof(unsigned short));
	memcpy(&m_depthBuff2[0], &reader.m_confidence[0], reader.m_confidence.size() * sizeof(unsigned short));


	// Update Point Cloud
	m_pointCloudCount = 0;
	if (sVertex *dst = (sVertex*)m_vtxBuff.Lock(renderer))
	{
		int cnt = 0;
		for (int i = 0; i < m_height; ++i)
		{
			for (int k = 0; k < m_width; ++k)
			{
				const Vector3 p1 = GetVertex(k, i);
				if (p1.IsEmpty())
					continue;

				dst->p = p1;

				++cnt;
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
				const Vector3 p2 = GetVertex(k - 1, i);
				const Vector3 p3 = GetVertex(k, i - 1);
				const Vector3 p4 = GetVertex(k + 1, i);
				const Vector3 p5 = GetVertex(k, i + 1);

				const float l1 = p1.Distance(p2);
				const float l2 = p1.Distance(p3);
				const float l3 = p1.Distance(p4);
				const float l4 = p1.Distance(p5);

				const float maxDist = g_root.m_depthDensity;
				if ((l1 > maxDist)
					|| (l2 > maxDist)
					|| (l3 > maxDist)
					|| (l4 > maxDist)
					)
					continue;


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

	const USHORT depth = m_depthBuff[y * m_width + x];
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
			mostNearVertex = vtx;
		}
	}

	return mostNearVertex;
}


void cSensorBuffer::GeneratePlane(common::Vector3 pos[3])
{
	Plane plane(pos[0], pos[1], pos[2]);
	m_plane = plane;
}


// sensor plane, volume center space change
void cSensorBuffer::ChangeSpace(cRenderer &renderer)
{
	Quaternion q;
	q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
	const Matrix44 tm = q.GetMatrix();

	Vector3 center = m_volumeCenter * tm;
	center.y = 0;

	for (u_int i = 0; i < m_vertices.size(); ++i)
	{
		Vector3 pos = m_vertices[i];
		m_vertices[i] = pos * tm;
		m_vertices[i].y += m_plane.D;
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


void cSensorBuffer::MeasureVolume(cRenderer &renderer)
{
	// Calculate Height Distribution
	g_root.m_distribCount = 0;
	ZeroMemory(g_root.m_hDistrib, sizeof(g_root.m_hDistrib));

	for (auto &vtx : m_vertices)
	{
		if (abs(vtx.x) > 150.f)
			continue;
		if (abs(vtx.z) > 150.f)
			continue;

		const int h = (int)(vtx.y * 10.f);
		if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
		{
			g_root.m_distribCount++;
			g_root.m_hDistrib[h] += 1.f;
		}
	}

	// height distribute pulse
	ZeroMemory(&g_root.m_hDistrib2, sizeof(g_root.m_hDistrib2));
	{
		const float minArea = 300.f;
		const float limitLowArea = 30.f;
		int state = 0;
		int startIdx = 0, endIdx = 0;
		int maxArea = 0;
		for (int i = 0; i < ARRAYSIZE(g_root.m_hDistrib); ++i)
		{
			const float a = g_root.m_hDistrib[i];

			switch (state)
			{
			case 0:
				if (a > minArea)
				{
					state = 1;
					startIdx = i;
					maxArea = i;
				}
				break;

			case 1:
				if (a > g_root.m_hDistrib[maxArea])
				{
					maxArea = i;
				}
				if (a < limitLowArea)
				{
					state = 2;
					endIdx = i;
				}
				break;

			case 2:
				state = 0;
				// find first again
				for (int k = startIdx; k >= 0; --k)
				{
					if (g_root.m_hDistrib[k] < limitLowArea)
					{
						startIdx = k;
						break;
					}
				}

				for (int k = startIdx; k < endIdx; ++k)
					g_root.m_hDistrib2[k] = 1;
				g_root.m_hDistrib2[maxArea] = 2;
				break;
			}
		}
	}


	// height distribute differential - 2
	//ZeroMemory(&g_root.m_hDistribDifferential, sizeof(g_root.m_hDistribDifferential));
	//float oldD = 0;
	//float oldArea = 0;
	//for (int i = 0; i < ARRAYSIZE(g_root.m_hDistrib); ++i)
	//{
	//	if (i == 0)
	//	{
	//		g_root.m_hDistribDifferential.AddValue(0);
	//		continue;
	//	}

	//	const float a = g_root.m_hDistrib[i];
	//	const float d = g_root.m_hDistrib[i] - g_root.m_hDistrib[i - 1];
	//	if ((d * oldD <= 0) && ((oldArea > 300) || (a > 300)))
	//		g_root.m_hDistribDifferential.AddValue(a);
	//	else
	//		g_root.m_hDistribDifferential.AddValue(0);

	//	oldD = d;
	//	oldArea = a;
	//}


	// Generate Area Floor
	u_int floor = 0;
	int state = 0;
	int startIdx = 0;
	int maxAreaIdx = 0;
	for (int i = 0; i < ARRAYSIZE(g_root.m_hDistrib2); ++i)
	{
		switch (state)
		{
		case 0:
			if (g_root.m_hDistrib2[i] > 0)
			{
				state = 1;
				startIdx = i;

				if (g_root.m_hDistrib2[i] > 1)
					maxAreaIdx = i;
			}
			break;

		case 1:
			if (g_root.m_hDistrib2[i] <= 0)
			{
				if ((maxAreaIdx==0) || (i - startIdx > 80)) // 범위가 너무크면 무시
				{
					state = 0;
				}
				else
				{
					state = 2;
				}
			}
			if (g_root.m_hDistrib2[i] > 1)
				maxAreaIdx = i;
		}

		if (state != 2)
			continue;
		state = 0;

		if (g_root.m_areaBuff.size() <= floor)
		{
			cVertexBuffer *vtxBuff = new cVertexBuffer();
			vtxBuff->Create(renderer, m_width*m_height, sizeof(sVertex), D3D11_USAGE_DYNAMIC);

			cRoot::sAreaFloor *areaFloor = new cRoot::sAreaFloor;
			areaFloor->vtxBuff = vtxBuff;
			g_root.m_areaBuff.push_back(areaFloor);
		}

		cRoot::sAreaFloor *areaFloor = g_root.m_areaBuff[floor++];
		areaFloor->startIdx = startIdx;
		areaFloor->maxIdx = maxAreaIdx;
		areaFloor->areaCnt = 0;
		areaFloor->areaMin = INT_MAX;
		areaFloor->areaMax = 0;
		ZeroMemory(&areaFloor->areaGraph, sizeof(areaFloor->areaGraph));

		// Generate AreaFloor Vertex
		if (sVertex *dst = (sVertex*)areaFloor->vtxBuff->Lock(renderer))
		{
			for (auto &vtx : m_vertices)
			{
				if (abs(vtx.x) > 150.f)
					continue;
				if (abs(vtx.z) > 150.f)
					continue;

				const int mostHighIdx = maxAreaIdx;
				const int h = (int)(vtx.y * 10.f);
				if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
				{
					const bool ok = ((h - mostHighIdx) == 0)
						|| (((h - mostHighIdx) > 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[0]))
						|| (((h - mostHighIdx) < 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[1]));

					if (ok)
					{
						++areaFloor->areaCnt;
						dst->p = vtx + Vector3(0, 0.01f, 0);
						++dst;
					}
				}
			}

			areaFloor->vtxBuff->Unlock(renderer);
		}

		if (areaFloor->areaCnt > areaFloor->areaMax)
			areaFloor->areaMax = areaFloor->areaCnt;
		if (areaFloor->areaCnt < areaFloor->areaMin)
			areaFloor->areaMin = areaFloor->areaCnt;

		areaFloor->areaGraph.AddValue((float)areaFloor->areaCnt);
	}
	g_root.m_areaFloorCnt = floor;

	// minimum height 10
	//int mostHighIdx = 10;
	//for (int i = 10; i < ARRAYSIZE(g_root.m_hDistrib); ++i)
	//{
	//	if (g_root.m_hDistrib[i] > g_root.m_hDistrib[mostHighIdx])
	//	{
	//		mostHighIdx = i;
	//	}
	//}

	//g_root.m_areaCount = 0;
	//cRenderer &renderer = GetRenderer();
	//if (sVertex *dst = (sVertex*)m_vtxBuff2.Lock(renderer))
	//{
	//	for (auto &vtx : m_vertices)
	//	{
	//		if (abs(vtx.x) > 15.f)
	//			continue;
	//		if (abs(vtx.z) > 15.f)
	//			continue;

	//		const int h = (int)(vtx.y * 10.f);
	//		if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
	//		{
	//			const bool ok = ((h - mostHighIdx) == 0)
	//				|| (((h - mostHighIdx) > 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[0]))
	//				|| (((h - mostHighIdx) < 0) && (abs(h - mostHighIdx) < g_root.m_heightErr[1]));
	//			
	//			if (ok)
	//			{
	//				++g_root.m_areaCount;
	//				dst->p = vtx + Vector3(0,0.01f,0);
	//				++dst;
	//			}
	//		}
	//	}

	//	m_vtxBuff2.Unlock(renderer);
	//}

	//if (g_root.m_areaCount > g_root.m_areaMax)
	//	g_root.m_areaMax = g_root.m_areaCount;
	//if (g_root.m_areaCount < g_root.m_areaMin)
	//	g_root.m_areaMin = g_root.m_areaCount;

	//g_root.m_areaGraph.AddValue((float)g_root.m_areaCount);
}


void cSensorBuffer::AnalysisDepth()
{
	// Analyze m_depthBuff
	int buff1[50000];
	ZeroMemory(buff1, sizeof(buff1));
	for (u_int i = 0; i < m_depthBuff.size(); ++i)
	{
		if (ARRAYSIZE(buff1) > m_depthBuff[i])
			++buff1[m_depthBuff[i]];
	}

	ZeroMemory(&m_analysis1, sizeof(m_analysis1));
	for (u_int i = 0; i < ARRAYSIZE(buff1); ++i)
		m_analysis1.AddValue((float)buff1[i]);

	// Analyze m_depthBuff2
	int buff2[50000];
	ZeroMemory(buff2, sizeof(buff2));
	for (u_int i = 0; i < m_depthBuff2.size(); ++i)
	{
		if (ARRAYSIZE(buff1) > m_depthBuff2[i])
			++buff2[m_depthBuff2[i]];
	}

	ZeroMemory(&m_analysis2, sizeof(m_analysis2));
	for (u_int i = 0; i < ARRAYSIZE(buff2); ++i)
		m_analysis2.AddValue((float)buff2[i]);
}


void cSensorBuffer::Clear()
{
}
