#include "stdafx.h"
#include "datreader.h"

using namespace graphic;

cDatReader::cDatReader()
	: m_time(0)
{
	m_vertices.resize(640 * 480);
	m_intensity.resize(640 * 480);
	m_confidence.resize(640 * 480);
}

cDatReader::~cDatReader()
{
	Clear();
}


// *.dat, *.pcd file read
bool cDatReader::Read(const char *fileName)
{
	using namespace std;

	Clear();

	StrPath readFileName;

	// is Compressed format?
	const StrPath path = fileName;
	const bool isCompressed = (!strcmp(path.GetFileExt(), ".pcdz")
		|| !strcmp(path.GetFileExt(), ".PCDZ"));
	if (isCompressed)
	{
		assert(0);
	}
	else
	{
		readFileName = fileName;
	}

	ifstream ifs(readFileName.c_str(), ios::binary);
	if (!ifs.is_open())
		return false;

	ifs.read((char*)&m_vertices[0], sizeof(float) * 3 * 640 * 480);
	ifs.read((char*)&m_intensity[0], sizeof(unsigned short) * 640 * 480);
	ifs.read((char*)&m_confidence[0], sizeof(unsigned short) * 640 * 480);
	ifs.close();

	if (isCompressed)
		remove(readFileName.c_str()); // remove unzip temporary file

	return true;
}


bool cDatReader::Write(const char *fileName)
{
	using namespace std;
	ofstream ofs(fileName, ios::binary);	
	if (!ofs.is_open())
		return false;

	const int nPixel = m_vertices.size();
	ofs.write((char*)&m_vertices[0], sizeof(float)*nPixel * 3);
	ofs.write((char*)&m_intensity[0], sizeof(uint16_t)*nPixel);
	ofs.write((char*)&m_confidence[0], sizeof(uint16_t)*nPixel);
	ofs.close();
	return true;
}


void cDatReader::Clear()
{
}
