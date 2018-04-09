//
// 2018-03-21, jjuiddong
// *.dat file reader
// custom point cloude data file
//	- xyz 
//	- intensity
// 
#pragma once


class cDatReader
{
public:
	cDatReader();
	virtual ~cDatReader();

	bool Read(const string &fileName);
	void Clear();


public:
	double m_time; // Update Time
	vector<common::Vector3> m_vertices;
	vector<unsigned short> m_intensity;
	vector<unsigned short> m_confidence;
};