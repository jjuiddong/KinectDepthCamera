
#include "stdafx.h"
#include "ptxreader.h"


cPtxReader::cPtxReader()
{

}

cPtxReader::~cPtxReader()
{

}


bool cPtxReader::Read(const char *fileName)
{
	const string fmt = GetFileFormat(fileName);


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
	return "";
}
