//
// 2018-03-19, jjuiddong
// *.ply file reader
//
#pragma once


class cPlyReader
{
public:
	cPlyReader();
	virtual ~cPlyReader();

	bool Read(const string &fileName);
	void Clear();


public:
	BYTE * m_data;
};
