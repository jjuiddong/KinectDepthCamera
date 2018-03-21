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


	//bool isReadData = false;
	//string line;
	//while (getline(ifs, line))
	//{
	//	if (isReadData)
	//	{
	//		stringstream ss(line);
	//		string sx, sy, sz, si;
	//		ss >> sx >> sy >> sz >> si;

	//		float x=0, y=0, z=0, i=0;
	//		if (sx != "nan")
	//			x = (float)atof(sx.c_str());
	//		if (sy != "nan")
	//			y = (float)atof(sy.c_str());
	//		if (sz != "nan")
	//			z = (float)atof(sz.c_str());
	//		if (si != "nan")
	//			i = (float)atof(si.c_str());

	//		m_vertices.push_back(Vector3(x, y, z));
	//	}
	//	else
	//	{
	//		if (line.find("DATA ASCII") != string::npos)
	//			isReadData = true;
	//	}
	//}

	return true;
}


void cDatReader::Clear()
{
}
