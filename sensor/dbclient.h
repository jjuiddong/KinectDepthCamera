//
// 2018-05-26, jjuiddong
// dbclient 
//
#pragma once


struct sMeasureVolume
{
	int id;
	float horz;
	float vert;
	float height;
	common::Vector3 pos;
	float volume;
	float vw;
	int pointCount;

	sContourInfo contour;
};


struct sMeasureResult
{
	int id; // measure id
	int type; // 1:delay measure, 2:snap measure
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
