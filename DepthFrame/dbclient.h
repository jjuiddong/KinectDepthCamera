//
// 2018-05-26, jjuiddong
// dbclient 
//
#pragma once


struct sMeasureVolume
{
	float horz;
	float vert;
	float height;
	float volume;
	float vw;
	int pointCount;
};


struct sMeasureResult
{
	vector<sMeasureVolume> volumes;
};


class cDBClient
{
public:
	cDBClient();
	virtual ~cDBClient();

	bool Create();
	bool Insert(const sMeasureResult &result);
	void Clear();


public:
	bool m_isRun; // for terminate thread
	MySQLConnection m_sql;
	common::cThread m_thread;
};
