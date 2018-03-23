#include "stdafx.h"
#include "datreader.h"

using namespace graphic;

cDatReader::cDatReader()
{
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

	m_vertices.resize(640 * 480);
	//m_colors.reserve(640 * 480);
	m_intensity.resize(640 * 480);
	m_confidence.resize(640 * 480);

	ifs.read((char*)&m_vertices[0], sizeof(float) * 3 * 640 * 480);
	ifs.read((char*)&m_intensity[0], sizeof(unsigned short) * 640 * 480);
	ifs.read((char*)&m_confidence[0], sizeof(unsigned short) * 640 * 480);

	return true;
}


void cDatReader::Clear()
{
}
