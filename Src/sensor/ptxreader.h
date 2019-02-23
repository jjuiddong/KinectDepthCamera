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


public:
	string GetFileFormat(const char *fileName);

};
