#include "stdafx.h"
#include "datreader.h"

using namespace graphic;

cDatReader::cDatReader()
	: m_time(0)
{
	//ZeroMemory(m_vertices, sizeof(m_vertices));
	//ZeroMemory(m_intensity, sizeof(m_intensity));
	//ZeroMemory(m_confidence, sizeof(m_confidence));
	m_vertices.resize(640 * 480);
	m_intensity.resize(640 * 480);
	m_confidence.resize(640 * 480);
}

cDatReader::~cDatReader()
{
	Clear();
}


// *.dat file read
bool cDatReader::Read(const string &fileName)
{
	using namespace std;

	Clear();

	ifstream ifs(fileName, ios::binary);
	if (!ifs.is_open())
		return false;

	ifs.read((char*)&m_vertices[0], sizeof(float) * 3 * 640 * 480);
	ifs.read((char*)&m_intensity[0], sizeof(unsigned short) * 640 * 480);
	ifs.read((char*)&m_confidence[0], sizeof(unsigned short) * 640 * 480);

	return true;
}


bool cDatReader::Write(const string &fileName)
{
	using namespace std;
	ofstream o(fileName.c_str(), ios::binary);	
	if (o)
	{
		const int nPixel = m_vertices.size();
		o.write((char*)&m_vertices[0], sizeof(float)*nPixel * 3);
		o.write((char*)&m_intensity[0], sizeof(uint16_t)*nPixel);
		o.write((char*)&m_confidence[0], sizeof(uint16_t)*nPixel);
	}

	return o.is_open();
}


void cDatReader::Clear()
{
}
