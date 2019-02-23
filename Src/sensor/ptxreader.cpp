
#include "stdafx.h"
#include "ptxreader.h"


cPtxReader::cPtxReader()
	: m_width(0)
	, m_height(0)
{
}

cPtxReader::~cPtxReader()
{
}


bool cPtxReader::Read(const char *fileName)
{
	const string fmt = GetFileFormat(fileName);

	if (fmt == "ptx")
		return ReadPTX(fileName);
	else if (fmt == "btx")
		return ReadBTX(fileName);

	return false;
}


// Read ASCII Format Point Cloud Data
bool cPtxReader::ReadPTX(const char *fileName)
{
	using namespace std;
	ifstream ifs(fileName);
	if (!ifs.is_open())
		return false;

	int state = 0;
	int width = 0;
	int height = 0;
	string line;
	while (getline(ifs, line))
	{
		switch (state)
		{
		case 0: width = atoi(line.c_str()); state++;  break;
		case 1: height = atoi(line.c_str()); state++; break;
		case 2:
		{
			// read transform
			int cnt = 0;
			do
			{
				// nothing~
			} while ((cnt++ < 7) && getline(ifs, line));

			// initialize buffer
			if (((u_int)width > 1000) || ((u_int)height > 1000))
				goto $error;

			m_vertices.reserve(width * height);
			m_colors.reserve(width * height);
			m_intensity.reserve(width * height);

			m_width = width;
			m_height = height;

			state++;
		}
		break;

		case 3:
		{
			stringstream ss(line);
			Vector3 vtx;
			float intensity;
			common::Vector3i color;
			ss >> vtx.x >> vtx.y >> vtx.z >> intensity >> color.x >> color.y >> color.z;
			const Vector3 colors(color.x / 255.f, color.y / 255.f, color.z / 255.f);

			m_vertices.push_back(vtx * 1000.f);
			m_intensity.push_back(intensity);
			m_colors.push_back(colors);
		}
		break;
		}
	}

	return true;


$error:
	return false;
}


// Read Binary PTX Format Point Cloud Data
bool cPtxReader::ReadBTX(const char *fileName)
{

	return true;
}


// "ptx" or "btx" string
// if not exist, return empty string
string cPtxReader::GetFileFormat(const char *fileName)
{
	using namespace std;
	ifstream ifs(fileName);
	if (!ifs.is_open())
		return "";

	char ext[3];
	ifs.read(ext, sizeof(ext));
	if ((ext[0] == 'B') && (ext[1] == 'T') && (ext[2] == 'X'))
		return "btx";
	return "ptx";
}
