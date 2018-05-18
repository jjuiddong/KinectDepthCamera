
#include "stdafx.h"
#include "sensorbuffer.h"

using namespace graphic;


cSensorBuffer::cSensorBuffer()
	: m_width(0)
	, m_height(0)
	, m_plane(Vector3(0, 0, 0), 0)
	, m_isLoaded(false)
	, m_time(0)
	, m_frameId(0)
	, m_isUpdatePointCloud(true)
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
	if (!reader.Read(fileName))
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


// Update, verext buffer, intensity buffer, confidence buffer
bool cSensorBuffer::UpdatePointCloud(cRenderer &renderer
	, const vector<Vector3> &vertices
	, const vector<unsigned short> &intensity
	, const vector<unsigned short> &confidence
)
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
	for (u_int i = 0; i < vertices.size(); ++i)
	{
		const Vector3 pos = vertices[i] * tm;
		if (!isnan(pos.x) && !isnan(vertices[i].x))
			diffAvrs += abs(pos.y - m_vertices[i].y);
		m_vertices[i] = pos;
	}
	diffAvrs /= (float)m_vertices.size();
	m_diffAvrs.AddValue(diffAvrs);

	memcpy(&m_intensity[0], &intensity[0], intensity.size() * sizeof(intensity[0]));
	memcpy(&m_confidence[0], &confidence[0], confidence.size() * sizeof(confidence[0]));

	// Update Point Cloud
	m_pointCloudCount = 0;
	if (m_isUpdatePointCloud)
	{
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
			mostNearVertex = vtx;
		}
	}

	return mostNearVertex;
}


void cSensorBuffer::GeneratePlane(common::Vector3 pos[3])
{

	Matrix44 tm;
	if (m_plane.N.IsEmpty())
	{
		Plane plane(pos[0], pos[1], pos[2]);
		m_plane = plane;
	}
	else
	{
		// old plane
		{
			Quaternion q;
			q.SetRotationArc(m_plane.N, Vector3(0, 1, 0));
			tm *= q.GetMatrix();
			Matrix44 T;
			T.SetPosition(Vector3(0, m_plane.D, 0));
			tm *= T;
		}

		tm.Inverse2();

		common::Vector3 p[3];
		p[0] = pos[0] * tm;
		p[1] = pos[1] * tm;
		p[2] = pos[2] * tm;
		Plane plane(p[0], p[1], p[2]);


		// new plane
		//{
		//	Quaternion q;
		//	q.SetRotationArc(plane.N, Vector3(0, 1, 0));
		//	tm *= q.GetMatrix();
		//	//Matrix44 T;
		//	//T.SetPosition(Vector3(0, plane.D, 0));
		//	//tm *= T;
		//}
		//Vector3 N = Vector3(0, 1, 0) * tm.Inverse();
		//Plane p(N.Normal(), 0);
		m_plane = plane;

	}
	//m_plane = plane;
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


// 포인트 클라우드에서 높이 분포를 계산한다.
// 높이분포를 이용해서 면적분포 메쉬를 생성한다.
// 높이 별로 포인트 클라우드를 생성한다.
void cSensorBuffer::MeasureVolume(cRenderer &renderer)
{
	// Calculate Height Distribution
	g_root.m_distribCount = 0;
	ZeroMemory(g_root.m_hDistrib, sizeof(g_root.m_hDistrib));

	for (auto &vtx : m_vertices)
	{
		if (abs(vtx.x) > 200.f)
			continue;
		if (abs(vtx.z) > 200.f)
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
		//const float minArea = 300.f;
		const float minArea = 150.f;
		const float limitLowArea = 30.f;
		int state = 0; // 0: check, rising pulse, 1: check down pulse, 2: calc low height
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
				g_root.m_hDistrib2[maxArea] = 2; // 가장 분포가 큰 높이에는 2를 설정한다.
				break;
			}
		}
	}

	// Generate Area Floor
	const cColor colors[] = { cColor::YELLOW, cColor::RED, cColor::GREEN, cColor::BLUE };

	u_int floor = 0;
	int state = 0; // 0: check rising pulse, 1: check down pulse, 2: collect area floor
	int startIdx = 0;
	int endIdx = 0;
	int maxAreaIdx = 0;
	for (int i = 0; i < ARRAYSIZE(g_root.m_hDistrib2); ++i)
	{
		switch (state)
		{
		case 0:
			// 펄스 상승 체크
			if (g_root.m_hDistrib2[i] > 0)
			{
				state = 1;
				startIdx = i;

				if (g_root.m_hDistrib2[i] > 1)
					maxAreaIdx = i;
			}
			break;

		case 1:
			// 펄스 하강 체크
			if (g_root.m_hDistrib2[i] <= 0)
			{
				if ((maxAreaIdx==0) || (startIdx == 0) || (i - startIdx > 400)) // 범위가 너무크면 무시
				{
					state = 0; // 무시되는 펄스 (첫 번째 펄스)
				}
				else
				{
					state = 2; // 면적으로 계산한다.
					endIdx = i;
				}
			}
			if (g_root.m_hDistrib2[i] > 1)
				maxAreaIdx = i; // 가장 분포가 큰 높이 설정
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
		areaFloor->endIdx = endIdx;
		areaFloor->maxIdx = maxAreaIdx;
		areaFloor->areaCnt = 0;
		if ((floor - 1) < ARRAYSIZE(colors))
			areaFloor->color = colors[floor - 1].GetColor();
		else
			areaFloor->color = common::Vector4(1, 1, 1, 1);

		ZeroMemory(&areaFloor->areaGraph, sizeof(areaFloor->areaGraph));

		// Generate AreaFloor Vertex
		if (sVertex *dst = (sVertex*)areaFloor->vtxBuff->Lock(renderer))
		{
			for (auto &vtx : m_vertices)
			{
				if (abs(vtx.x) > 200.f) // x axis limit
					continue;
				if (abs(vtx.z) > 200.f) // y axis limit
					continue;

				const int h = (int)(vtx.y * 10.f);
				if ((h >= 0) && (h < ARRAYSIZE(g_root.m_hDistrib)))
				{
					const bool ok = (startIdx <= h) && (endIdx > h);
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

		areaFloor->areaGraph.AddValue((float)areaFloor->areaCnt);
	}
	g_root.m_areaFloorCnt = floor;
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
