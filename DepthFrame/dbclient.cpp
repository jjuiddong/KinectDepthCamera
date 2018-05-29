
#include "stdafx.h"
#include "dbclient.h"

using namespace common;


//-------------------------------------------------------------------
// cTaskQuery
class cTaskQuery : public common::cTask
{
public:
	cTaskQuery(int id, cDBClient &dbClient, const sMeasureResult &result)
		: cTask(id, "cTaskQuery")
		, m_dbClient(dbClient)
		, m_result(result) {
	}
	virtual ~cTaskQuery() {
	}

	virtual eRunResult::Enum Run(const double deltaSeconds) { 

		if (!m_dbClient.m_isRun)
			return eRunResult::END;
		
		using namespace std;
		stringstream ss;

		ss << "{ \"VERSION\" : 1\n";
		ss << "\t,\"MEASURE_ID\" : " << m_result.id << "\n";
		ss << "\t,\"TYPE\" : " << m_result.type << "\n";
		ss << "\t,\"VOLUME\" : [\n";
		
		for (u_int i = 0; i < m_result.volumes.size(); ++i)
		{
			const sMeasureVolume &measure = m_result.volumes[i];

			ss << "\t";

			if (0 != i)
				ss << ", "; // comma

			ss << "{\n";

			ss << "\t\t \"ID\" : " << measure.id << "\n";
			ss << "\t\t ,\"HORZ\" : " << measure.horz << "\n";
			ss << "\t\t ,\"VERT\" : " << measure.vert << "\n";
			ss << "\t\t ,\"HEIGHT\" : " << measure.height << "\n";
			ss << "\t\t ,\"VOLUME\" : " << measure.volume << "\n";
			ss << "\t\t ,\"VW\" : " << measure.vw << "\n";
			ss << "\t\t ,\"POINTCOUNT\" : " << measure.pointCount << "\n";
			
			ss << "\t}\n";
		}

		ss << "\t] \n";

		// save contour 
		if (2 == m_result.type)
		{
			ss << "\n";
			ss << "\t,\"CONTOUR\" : [\n";

			for (u_int i = 0; i < m_result.volumes.size(); ++i)
			{
				const sMeasureVolume &measure = m_result.volumes[i];

				ss << "\t";

				if (0 != i)
					ss << ", "; // comma

				ss << "{\n";

				ss << "\t\t \"ID\" : " << measure.id << "\n";
				ss << "\t\t ,\"LEVEL\" : " << measure.contour.level << "\n";
				ss << "\t\t ,\"LOOP\" : " << measure.contour.loop << "\n";
				ss << "\t\t ,\"LOWERH\" : " << measure.contour.lowerH << "\n";
				ss << "\t\t ,\"UPPERH\" : " << measure.contour.upperH << "\n";
				ss << "\t\t ,\"VERTEX\" : [\n";
				ss << "\t\t ";

				for (u_int k=0; k < measure.contour.contour.m_data.size(); ++k)
				{
					auto &vtx = measure.contour.contour.m_data[k];
					if (k != 0)
						ss << ", ";
					ss << vtx.x << ", " << vtx.y;
				}

				ss << " ]\n";
				ss << "\t}\n";
			}

			ss << "\t] \n";
		}
		
		ss << "}";

		if (m_dbClient.m_sql.IsConnected())
		{
			string str = "INSERT INTO  tb_cargo_mear(MEASURE_DATA) VALUES ('";
			str += ss.str();
			str += "');";
			MySQLQuery query(&m_dbClient.m_sql, str);
			const int result = query.ExecuteInsert();
		}

		// 임시로 파일로 저장한다.
		string fileName = "jsonoutput_" + common::GetCurrentDateTime() + ".volume";
		ofstream ofs(fileName);
		if (ofs.is_open())
		{
			ofs << ss.str();
		}

		return eRunResult::END; 
	}


public:
	cDBClient &m_dbClient;
	sMeasureResult m_result;
};



//-------------------------------------------------------------------
// cDBClient
cDBClient::cDBClient()
	: m_isRun(true)
{
}

cDBClient::~cDBClient()
{
	Clear();
}


bool cDBClient::Create()
{
	if (m_sql.IsConnected())
		return true;

	if (!m_sql.Connect("127.0.0.1", 3306, "root", "1111", "volume"))
	{
		return false;
	}

	return true;
}


bool cDBClient::Insert(const sMeasureResult &result)
{
	if (!m_sql.IsConnected())
		return true;

	static int id = 0;
	m_thread.AddTask(new cTaskQuery(id++, *this, result));
	if (!m_thread.IsRun())
		m_thread.Start();
	
	return true;
}


void cDBClient::Clear()
{
	m_isRun = false;
	m_thread.Clear();
}
