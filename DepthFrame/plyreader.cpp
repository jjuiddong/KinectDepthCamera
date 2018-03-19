
#include "stdafx.h"
#include "plyreader.h"


cPlyReader::cPlyReader()
	: m_data(NULL)
{
}

cPlyReader::~cPlyReader()
{
	Clear();
}


// *.ply file read
bool cPlyReader::Read(const string &fileName)
{
	using namespace std;
	int dataPtrSize = 0;

	Clear();

	// find data pointer
	ifstream ifs(fileName, ios::binary);
	if (!ifs.is_open())
		return false;

	char fmt[3];
	ifs.read(fmt, 3);
	if (('p' != fmt[0]) || ('l' != fmt[1]) || ('y' != fmt[2]))
		return false;

	bool isHeaderRead = false;
	while (!ifs.eof() && !isHeaderRead)
	{
		int idx = 0;
		char line[256];
		ZeroMemory(line, ARRAYSIZE(line));
		while (ifs.read(line + idx, 1))
		{
			if ('\n' == line[idx])
			{
				line[idx-1] = NULL; // '\r'
				line[idx] = NULL;  // '\n'
				break;
			}
			++idx;
		}

		if (string(line) == "end_header")
		{
			isHeaderRead = true;
			break;
		}
	}

	const int fileSize = (int)common::FileSize(fileName);
	const int totalDataSize = fileSize - ifs.get();
	m_data = new BYTE[totalDataSize];
	ifs.read((char*)m_data, totalDataSize);

	for (int i = 0; i < 10; ++i)
	{
		float f1 = *(float*)&m_data[0];
		float f2 = *(float*)&m_data[4];
		float f3 = *(float*)&m_data[8];

		BYTE b1 = m_data[12];
		BYTE b2 = m_data[13];
		BYTE b3 = m_data[14];

		float f4 = *(float*)&m_data[15];
		float f5 = *(float*)&m_data[19];
		float f6 = *(float*)&m_data[23];
	}


	// data memory copy
	//{
	//	const int fileSize = (int)common::FileSize(fileName);

	//	ifstream ifs(fileName, ios::binary);
	//	if (!ifs.is_open())
	//		return false;

	//	ifs.seekg(dataPtrSize);

	//	const int totalDataSize = fileSize - dataPtrSize;
	//	m_data = new BYTE[totalDataSize];

	//	ifs.read((char*)m_data, totalDataSize);
	//}

	return true;
}


void cPlyReader::Clear()
{
	SAFE_DELETEA(m_data);
}
