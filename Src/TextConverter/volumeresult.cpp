
#include "stdafx.h"
#include "volumeresult.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


cVolumeResult::cVolumeResult()
	: m_version(0)
	, m_measureId(0)
	, m_type(0)
{
}

cVolumeResult::~cVolumeResult()
{
}


bool cVolumeResult::Read(const char *fileName)
{
	m_volumes.clear();
	m_contours.clear();

	try
	{
		using boost::property_tree::ptree;

		ptree props;
		boost::property_tree::read_json(fileName, props);

		m_version = props.get<int>("VERSION");
		m_measureId = props.get<int>("MEASURE_ID");
		m_type = props.get<int>("TYPE");

		ptree &children = props.get_child("VOLUME");
		for (ptree::value_type vt : children)
		{
			sVolume volume;
			volume.id = vt.second.get<int>("ID");
			volume.horz = vt.second.get<float>("HORZ");
			volume.vert = vt.second.get<float>("VERT");
			volume.height = vt.second.get<float>("HEIGHT");
			volume.volume = vt.second.get<float>("VOLUME");
			volume.vw = vt.second.get<float>("VW");
			volume.pointCnt = vt.second.get<int>("POINTCOUNT");

			m_volumes.push_back(volume);
		}

		ptree &children2 = props.get_child("CONTOUR");
		for (ptree::value_type vt : children2)
		{
			sContour contour;
			contour.id = vt.second.get<int>("ID");
			contour.level = vt.second.get<int>("LEVEL");
			contour.loop = vt.second.get<int>("LOOP");
			contour.lowerH = vt.second.get<float>("LOWERH");
			contour.upperH = vt.second.get<float>("UPPERH");

			ptree &children3 = vt.second.get_child("VERTEX");
			int n = 0;
			common::Vector2 vtx;
			for (ptree::value_type vt : children3)
			{
				if ((n % 2) == 0)
				{
					vtx.x = (float)atof(vt.second.data().c_str());
				}
				else
				{
					vtx.y = (float)atof(vt.second.data().c_str());
					contour.vertices.push_back(vtx);
				}
				n++;
			}

			m_contours.push_back(contour);
		}

	}
	catch (std::exception&e)
	{
		//cout << "Error, " << e.what();
		string errMsg = e.what();
		return false;
	}

	return true;
}


// 소수점 두째 자리에서 반올림, 소수점 첫째 자리 표시
void cVolumeResult::Add(const sVolume &volume)
{
	sVolume v = volume;
	if ((volume.horz != 0) && (volume.vert != 0) && (volume.height != 0))
	{
		v.vw = (volume.horz * volume.vert * volume.height) / 6000.f;
	}

	m_volumes.push_back(v);
}
