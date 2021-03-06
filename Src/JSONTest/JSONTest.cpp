// JSONTest.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


int main()
{
	using namespace std;
	//string fileName = "jsontest.txt";
	//string fileName = "jsonoutput_2018-05-27-00-55-15-304.txt";
	//string fileName = "jsonoutput_2018-05-27-01-02-02-464.txt";
	//string fileName = "jsonoutput_2018-05-28-15-13-38-432.txt";
	string fileName = "jsonoutput_2018-05-28-16-56-15-945.volume";
	
	try
	{
		using boost::property_tree::ptree;

		ptree props;
		boost::property_tree::read_json(fileName, props);
		ptree &children = props.get_child("VOLUME");
		for (ptree::value_type vt : children)
		{
			//sProjectInfo info;
			int v0 = vt.second.get<int>("ID");
			float v1 = vt.second.get<float>("HORZ");
			float v2 = vt.second.get<float>("VERT");
			float v3 = vt.second.get<float>("HEIGHT");
			float v4 = vt.second.get<float>("VOLUME");
			float v5 = vt.second.get<float>("VW");
			int v6 = vt.second.get<int>("POINTCOUNT");
			int a = 0;
		}

		ptree &children2 = props.get_child("CONTOUR");
		for (ptree::value_type vt : children2)
		{
			//sProjectInfo info;
			int v0 = vt.second.get<int>("ID");
			int v1 = vt.second.get<int>("LEVEL");
			int v2 = vt.second.get<int>("LOOP");
			float v3 = vt.second.get<float>("LOWERH");
			float v4 = vt.second.get<float>("UPPERH");
			int a = 0;
		}

	}
	catch (std::exception&e)
	{
		cout << "Error, " << e.what();
		return false;
	}

    return 0;
}

