//
// 2019-02-23, jjuiddong
// ptx file reader
//
// - point cloud file format *.ptx
//		- binary ptx format : *.btx
//		- https://www.laserscanningforum.com/forum/viewtopic.php?t=743
//
#pragma once


class cPtxReader
{
public:
	cPtxReader();
	virtual ~cPtxReader();

	bool Read(const char *fileName);


protected:
	bool ReadPTX(const char *fileName);
	bool ReadBTX(const char *fileName);
	string GetFileFormat(const char *fileName);


public:
	int m_width;
	int m_height;
	vector<common::Vector3> m_vertices;
	vector<common::Vector3> m_colors;
	vector<float> m_intensity;
};
