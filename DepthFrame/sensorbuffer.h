//
// 2018-03-20, jjuiddong
// Sensor Buffer
//	- depth
//	- color
//
#pragma once


class cDatReader;

class cSensorBuffer
{
public:
	cSensorBuffer();
	virtual ~cSensorBuffer();

	void Render(graphic::cRenderer &renderer, const char *techniqName = "Unlit"
		, const XMMATRIX &parentTm = graphic::XMIdentity);

	bool ReadKinectSensor(graphic::cRenderer &renderer
		, INT64 nTime
		, const USHORT* pBuffer
		, USHORT nMinDepth
		, USHORT nMaxDepth);

	bool ReadPlyFile(graphic::cRenderer &renderer, const string &fileName);
	bool ReadDatFile(graphic::cRenderer &renderer, const string &fileName);
	bool ReadDatFile(graphic::cRenderer &renderer, const cDatReader &reader);
	inline common::Vector3 Get3DPos(const int x, const int y, USHORT nMinDepth, USHORT nMaxDepth);
	inline common::Vector3 GetVertex(const int x, const int y);
	common::Vector3 PickVertex(const common::Ray &ray);
	void GeneratePlane(common::Vector3 pos[3]);
	void ChangeSpace(graphic::cRenderer &renderer);
	void MeasureVolume(graphic::cRenderer &renderer);
	void AnalysisDepth();
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
	vector<USHORT> m_depthBuff; // intensity
	vector<USHORT> m_depthBuff2; // confidence
	vector<common::Vector3> m_vertices;
	vector<graphic::cColor> m_colors;
	cv::Mat m_srcImg;
	common::Plane m_plane;
	common::Vector3 m_volumeCenter;
	graphic::cVertexBuffer m_vtxBuff;

	// Analysis
	sGraph<50000> m_analysis1;
	sGraph<50000> m_analysis2;
	sGraph<30> m_diffAvrs;

};
