#include "stdafx.h"
#include <string>
#include <iostream>
#include "volumeresult.h"

using namespace std;


float g_idealVW = 0.f;
bool sort1(const cVolumeResult &result1, const cVolumeResult &result2)
{
	float vw1 = 0.f;
	for (auto &v : result1.m_volumes)
		vw1 += v.vw;

	float vw2 = 0.f;
	for (auto &v : result2.m_volumes)
		vw2 += v.vw;

	const float f1 = abs(vw1 - g_idealVW);
	const float f2 = abs(vw2 - g_idealVW);
	return f1 < f2;
}


// 1m box
void measure9()
{
	list<string> exts;
	exts.push_back("volume");
	list<string> files;
	common::CollectFiles(exts, "..\\media\\TestData-2018-05-30-9\\", files);
	files.sort();

	vector<cVolumeResult> results;

	bool startRead = false;
	for (auto &fileName : files)
	{
		cVolumeResult result;
		if (result.Read(fileName.c_str()))
		{
			if (result.m_measureId == 9)
			{
				cout << "add " << fileName << endl;

				startRead = true;
				if (result.m_type == 2)
					results.push_back(result);
			}
			else if (startRead)
			{
				break;
			}
		}
	}

	ofstream ofs("1m box.txt");
	ofs.setf(ios::fixed);
	ofs.precision(2);

	ofs << "horz\t";
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			ofs << volume.horz << "\t";
	ofs << endl;

	ofs << "vert\t";
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			ofs << volume.vert << "\t";
	ofs << endl;

	ofs << "height\t";
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			ofs << volume.height << "\t";
	ofs << endl;

	ofs << "volume\t";
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			ofs << volume.volume << "\t";
	ofs << endl;

	ofs << "vw\t";
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			ofs << volume.vw << "\t";
	ofs << endl;

	
	float vwAvr = 0;
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			vwAvr += volume.vw;
	vwAvr /= results.size();
	const float vwErr = ((vwAvr - 166.66666f) / 166.6666f) * 100.f;

	// vw 차이가 가장 적은 걸 기준으로 정렬
	g_idealVW = 166.66666f;
	std::sort(results.begin(), results.end(), sort1);

	float vwAvr2 = 0;
	for (u_int i = 0; i < 100; ++i)
	{
		cVolumeResult &vr = results[i];
		for (auto &volume : vr.m_volumes)
		{
			cout << volume.vw << endl;
			vwAvr2 += volume.vw;
		}
	}
	vwAvr2 /= 100;
	const float vwErr2 = ((vwAvr2 - 166.66666f) / 166.6666f) * 100.f;

	const float err0 = ((171.716833333f - 166.66666f) / 166.6666f) * 100.f;

}


// #4. 1m + 1.5m
void measure11()
{
	list<string> exts;
	exts.push_back("volume");
	list<string> files;
	common::CollectFiles(exts, "..\\media\\TestData-2018-05-30-11\\", files);
	files.sort();

	vector<cVolumeResult> results;

	bool startRead = false;
	for (auto &fileName : files)
	{
		cVolumeResult result;
		if (result.Read(fileName.c_str()))
		{
			if (result.m_measureId == 11)
			{
				cout << "add " << fileName << endl;

				startRead = true;
				if (result.m_type == 2)
					results.push_back(result);
			}
			else if (startRead)
			{
				break;
			}
		}
	}

	const float idealVW = 187.45f;

	float vwAvr = 0;
	for (auto &result : results)
		for (auto &volume : result.m_volumes)
			vwAvr += volume.vw;
	vwAvr /= results.size();
	const float vwErr = ((vwAvr - idealVW) / idealVW) * 100.f;

	// vw 차이가 가장 적은 걸 기준으로 정렬
	g_idealVW = idealVW;
	std::sort(results.begin(), results.end(), sort1);

	float vwAvr2 = 0;
	for (u_int i = 0; i < 100; ++i)
	{
		cVolumeResult &vr = results[i];
		for (auto &volume : vr.m_volumes)
		{
			cout << volume.vw << endl;
			vwAvr2 += volume.vw;
		}
	}
	vwAvr2 /= 100;
	const float vwErr2 = ((vwAvr2 - idealVW) / idealVW) * 100.f;
	const float err0 = ((171.716833333f - idealVW) / idealVW) * 100.f;


	ofstream ofs("#4 100cm + 50cm box.txt");
	ofs.setf(ios::fixed);
	ofs.precision(2);

	ofs << "vw\t";
	for (auto &result : results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		ofs << vw << "\t";
	}
	ofs << endl;

}


int main()
{
	using namespace std;
	//string fileName = "../media/TestData-2018-05-30-2/jsonoutput_2018-05-30-00-59-50-917.volume";
	//cVolumeResult result;
	//result.Read(fileName.c_str());

	measure11();


	return 0;
}
