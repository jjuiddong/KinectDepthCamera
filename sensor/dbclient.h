//
// 2018-05-26, jjuiddong
// dbclient 
//
#pragma once


class cDBClient
{
public:
	cDBClient();
	virtual ~cDBClient();

	bool Create();
	bool Insert(const sMeasureResult &result);
	void Clear();
	static string GetResult2JSon(const sMeasureResult &result);
	bool WriteExcel(const bool isIntegral);


public:
	bool m_isRun; // for terminate thread
	MySQLConnection m_sql;
	vector<sMeasureResult> m_results;
	common::cThread m_thread;
};
