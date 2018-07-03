//
// 2018-03-21, jjuiddong
// *.dat, *.pcd  file reader
// custom point cloude data file
//	- 640 x 480
//	- xyz 
//	- intensity
//	- confidence
// 
#pragma once


class cDatReader
{
public:
	cDatReader();
	virtual ~cDatReader();

	bool Read(const char *fileName);
	bool Write(const char *fileName);
	void Clear();


public:
	double m_time; // Update Time
	vector<common::Vector3> m_vertices;
	vector<unsigned short> m_intensity;
	vector<unsigned short> m_confidence;
};
