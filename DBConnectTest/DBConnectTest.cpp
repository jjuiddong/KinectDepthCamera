// DBConnectTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>

using namespace std;


int main()
{
	MySQLConnection sql;
	if (!sql.Connect("192.168.123.139", 3306, "root2", "1111", "volume"))
	{
		cout << "fail" << endl;
		return 0;
	}

	MySQLQuery query(&sql
		, "INSERT INTO  tb_cargo_mear(STN_CO,VOL_LOC,CARR_C,FLT_NO,FLT_D,MAWB_NO,MEASURE_DATA) VALUES ('ICN', 'VOL01', 'KE', '404', '20170801', '18011111111', '{\"ERR_MSG\": \"\", \"WIDTH_M\": 100, \"WORK_NO\": \"550e8400-e29b-41d4-a716-446655440000\", \"ERR_CODE\": 0, \"HEIGHT_M\": 100, \"LENGTH_M\": 100}');");
	int result = query.ExecuteInsert();
	cout << "result = " << result << endl;

    return 0;
}
