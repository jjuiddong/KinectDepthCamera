// TextConverter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "volumeresult.h"
using namespace std;
using namespace common;

vector<cVolumeResult> g_results;


float stddev(const float avr)
{
	if (g_results.empty())
		return 0;

	float dev = 0;
	for (auto &result : g_results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		dev += (avr - vw) * (avr - vw);
	}

	return sqrt(dev / g_results.size());
}


float average()
{
	if (g_results.empty())
		return 0;

	float avr = 0;
	for (auto &result : g_results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		avr += vw;
	}
	return avr / g_results.size();
}


float error_rate(const float ideal_vw)
{
	if (g_results.empty())
		return 0;

	float err = 0;
	float avr = 0;
	for (auto &result : g_results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		avr += vw;

		err += (abs(vw - ideal_vw) / ideal_vw) * 100.f;

	}

	return err / g_results.size();
}


float average_error(const float ideal_vw)
{
	if (g_results.empty())
		return 0;

	float error = 0;
	for (auto &result : g_results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;

		result.m_error = (abs(vw - ideal_vw) / ideal_vw) * 100.f;
		error += result.m_error;
	}

	return error / g_results.size();
}


float stddev_error(const float avr)
{
	if (g_results.empty())
		return 0;

	float dev = 0;
	for (auto &result : g_results)
	{
		dev += (avr - result.m_error) * (avr - result.m_error);
	}

	return sqrt(dev / g_results.size());
}

void output(const char *fileName)
{
	ofstream ofs(fileName);
	if (!ofs.is_open())
		return;

	ofs << fixed;
	ofs.precision(1);

	ofs << "horz \t";
	for (auto &result : g_results)
		for (auto &volume : result.m_volumes)
			ofs << volume.horz << "\t";
	ofs << endl;

	ofs << "vert \t";
	for (auto &result : g_results)
		for (auto &volume : result.m_volumes)
			ofs << volume.vert << "\t";
	ofs << endl;

	ofs << "height \t";
	for (auto &result : g_results)
		for (auto &volume : result.m_volumes)
			ofs << volume.height << "\t";
	ofs << endl;

	ofs << "vw \t";
	for (auto &result : g_results)
		for (auto &volume : result.m_volumes)
			ofs << volume.vw << "\t";
	ofs << endl;

	ofs << "vw total \t";
	for (auto &result : g_results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		ofs << vw << "\t";
	}
	ofs << endl;
}


// #1
void test1_box100cm()
{
	ifstream ifs("#1_100cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		cVolumeResult::sVolume volume;
		ss >> volume.horz >> volume.vert >> volume.height;
		vr.Add(volume);
		g_results.push_back(vr);
	}

	const float IDEAL_VW = 166.6666f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;


	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#1_100cm_out.txt");
}


//#2
void test2_box50cm()
{
	ifstream ifs("#2_50cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		cVolumeResult::sVolume volume;
		ss >> volume.horz >> volume.vert >> volume.height;
		vr.Add(volume);
		g_results.push_back(vr);
	}

	const float IDEAL_VW = 20.833333f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;

	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#2_50cm_out.txt");
}


// #3
void test3_box30cm()
{
	ifstream ifs("#3_30cm_2.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		cVolumeResult::sVolume volume;
		ss >> volume.horz >> volume.vert >> volume.height;
		vr.Add(volume);
		g_results.push_back(vr);
	}

	const float IDEAL_VW = 6.462f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#3_30cm_out.txt");
}


// #4
void test4_box100_50cm()
{
	ifstream ifs("#4_100cm+50cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		float vw1, vw2 = 0;
		ss >> vw1 >> vw2;

		cVolumeResult::sVolume volume1;
		volume1.vw = vw1;
		vr.Add(volume1);

		cVolumeResult::sVolume volume2;
		volume2.vw = vw2;
		vr.Add(volume2);

		g_results.push_back(vr);
	}

	// 166.666 + 20.83333
	const float IDEAL_VW = 187.5;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#4_100cm+50cm_out.txt");
}


// #5
void test5_box100_50cm()
{
	ifstream ifs("#5_100cm+50cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		float vw1, vw2 = 0;
		ss >> vw1 >> vw2;

		cVolumeResult::sVolume volume1;
		volume1.vw = vw1;
		vr.Add(volume1);

		cVolumeResult::sVolume volume2;
		volume2.vw = vw2;
		vr.Add(volume2);

		g_results.push_back(vr);
	}

	const float IDEAL_VW = 187.5;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#5_100cm+50cm_out.txt");
}


// #6
void test6_box100_50_30cm()
{
	ifstream ifs("#6_100cm+50cm+30cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{stringstream ss(line.c_str());
		cVolumeResult vr;
		float vw1, vw2, vw3;
		ss >> vw1 >> vw2 >> vw3;

		cVolumeResult::sVolume volume1;
		volume1.vw = vw1;
		vr.Add(volume1);

		g_results.push_back(vr);
	}

	// 166.6666 + 20.83333 + 6.462
	const float IDEAL_VW = 193.961f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#6_100cm+50cm+30cm_out.txt");
}


// #7
void test7_box100_50_30cm()
{
	ifstream ifs("#7_100cm+50cm+30cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		float vw1, vw2, vw3;
		ss >> vw1 >> vw2 >> vw3;

		cVolumeResult::sVolume volume1;
		volume1.vw = vw1;
		vr.Add(volume1);

		g_results.push_back(vr);
	}

	// 166.6666 + 20.83333 + 6.462
	const float IDEAL_VW = 193.961f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#7_100cm+50cm+30cm_out.txt");
}


// #8
void test8_box100_50_30cm()
{
	ifstream ifs("#8_100cm+50cm+30cm.txt");
	if (!ifs.is_open())
		return;

	Str128 line;
	while (ifs.getline(line.m_str, line.SIZE))
	{
		stringstream ss(line.c_str());
		cVolumeResult vr;
		float vw1, vw2, vw3;
		ss >> vw1 >> vw2 >> vw3;

		cVolumeResult::sVolume volume1;
		volume1.vw = vw1;
		vr.Add(volume1);

		g_results.push_back(vr);
	}

	// 166.6666 + 20.83333 + 6.462
	const float IDEAL_VW = 193.961f;
	const float avr = average();
	const float err = error_rate(IDEAL_VW);
	const float dev = stddev(avr);
	const float avr_error = average_error(IDEAL_VW);
	const float dev_error = stddev_error(avr_error);

	cout << "average = " << avr << endl;
	cout << "stddev = " << dev << endl;
	//cout << "error = " << (abs(avr - IDEAL_VW) / IDEAL_VW) * 100.f << endl;
	cout << "error = " << err << endl;
	//cout << "average error = " << avr_error << endl;
	//cout << "stddev error = " << dev_error << endl;

	output("#8_100cm+50cm+30cm_out.txt");
}


int main()
{
	//test1_box100cm();
	test2_box50cm();
	//test3_box30cm();
	//test4_box100_50cm();
	//test5_box100_50cm();
	//test6_box100_50_30cm();
	//test7_box100_50_30cm();
	//test8_box100_50_30cm();

    return 0;
}

