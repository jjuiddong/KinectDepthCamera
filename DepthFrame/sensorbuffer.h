//
// 2018-03-20, jjuiddong
// Sensor Buffer
//	- depth
//	- color
//
#pragma once


class cSensorBuffer
{
public:
	cSensorBuffer();
	virtual ~cSensorBuffer();

	void Render(graphic::cRenderer &renderer);

	bool ReadKinectSensor(graphic::cRenderer &renderer
		, INT64 nTime
		, const USHORT* pBuffer
		, USHORT nMinDepth
		, USHORT nMaxDepth);

	bool ReadPlyFile(graphic::cRenderer &renderer, const string &fileName);
	bool ReadDatFile(graphic::cRenderer &renderer, const string &fileName);
	inline common::Vector3 Get3DPos(const int x, const int y, USHORT nMinDepth, USHORT nMaxDepth);
	inline common::Vector3 GetVertex(const int x, const int y);
	common::Vector3 PickVertex(const common::Ray &ray);
	void GeneratePlane(common::Vector3 pos[3]);
	void ChangeSpace(graphic::cRenderer &renderer);
	void MeasureVolume(graphic::cRenderer &renderer);
	void Clear();

	bool ProcessKinectDepthBuff(graphic::cRenderer &renderer
		, INT64 nTime
		, const USHORT* pBuffer
		, USHORT nMinDepth
		, USHORT nMaxDepth);


public:
	int m_width;
	int m_height;
	int m_pointCloudCount;
	vector<USHORT> m_depthBuff;
	vector<USHORT> m_depthBuff2;
	vector<common::Vector3> m_vertices;
	vector<graphic::cColor> m_colors;
	common::Plane m_plane;
	common::Vector3 m_volumeCenter;
	graphic::cVertexBuffer m_vtxBuff;
};
