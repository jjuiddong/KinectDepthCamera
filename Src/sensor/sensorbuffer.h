//
// 2018-03-20, jjuiddong
// Sensor Buffer
//	- Perspective Projection Vertex
//	- Intensity map
//	- Confidence map
//
#pragma once

#include "baslersync.h"


class cDatReader;

class cSensorBuffer
{
public:
	cSensorBuffer();
	virtual ~cSensorBuffer();

	bool ReadKinectSensor(graphic::cRenderer &renderer
		, INT64 nTime
		, const USHORT* pBuffer
		, USHORT nMinDepth
		, USHORT nMaxDepth);

	bool ReadPlyFile(graphic::cRenderer &renderer, const string &fileName);
	bool ReadDatFile(graphic::cRenderer &renderer, const string &fileName);
	bool ReadDatFile(graphic::cRenderer &renderer, const cDatReader &reader);
	void Zeros(graphic::cRenderer &renderer);

	void Render(graphic::cRenderer &renderer
		, const char *techniqName = "Unlit"
		, const bool isAphablend = false
		, const XMMATRIX &parentTm = graphic::XMIdentity);

	void RenderTessellation(graphic::cRenderer &renderer 
		, const XMMATRIX &parentTm = graphic::XMIdentity);

	inline common::Vector3 Get3DPos(const int x, const int y, USHORT nMinDepth, USHORT nMaxDepth);
	inline common::Vector3 GetVertex(const int x, const int y);
	common::Vector3 PickVertex(const common::Ray &ray);
	//void GeneratePlane(common::Vector3 pos[3]);
	void ChangeSpace(graphic::cRenderer &renderer);
	bool UpdatePointCloudBySelf(graphic::cRenderer &renderer);
	bool UpdatePointCloudAllConfig(graphic::cRenderer &renderer);
	void AnalysisDepth();
	void Clear();


protected:
	void InitBuffer(graphic::cRenderer &renderer);
	bool UpdatePointCloud(graphic::cRenderer &renderer
		, const vector<common::Vector3> &vertices
		, const vector<unsigned short> &intensity
		, const vector<unsigned short> &confidence
	);
	bool ProcessKinectDepthBuff(graphic::cRenderer &renderer
		, INT64 nTime
		, const USHORT* pBuffer
		, USHORT nMinDepth
		, USHORT nMaxDepth);


public:
	bool m_isLoaded;
	int m_width;
	int m_height;
	bool m_isUpdatePointCloud;
	double m_time; // update time
	int m_frameId;
	int m_pointCloudCount;
	bool m_mergeOffset; // 높이에 따른 오프셋 변화처리 (Right 카메라만 적용)

	vector<common::Vector3> m_srcVertices; // source vertices, (balser source vertices)
	vector<common::Vector3> m_vertices; // change space vertices, (perspectiv projection vertex), no apply offset
	vector<USHORT> m_intensity;
	vector<USHORT> m_confidence;
	common::Plane m_planeSub;
	graphic::cVertexBuffer m_vtxBuff;
	common::Transform m_offset;
	common::sRectf m_cullRect; // 영역 안에 들어올 때만 PointCloud에 적용된다.
	common::sRectf m_cullExtraRect; // 영역에 포함되지 않을 때, PointCloud에 적용된다.
	graphic::cShader11 m_shader;

	// Analysis
	sGraph<50000> m_analysis1;
	sGraph<50000> m_analysis2;
	sGraph<30> m_diffAvrs;
};
