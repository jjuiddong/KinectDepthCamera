// -100cm, vw : 166.6666...
// - 50cm, vw : 20.833333...
// - 47 x 30 x 27.5cm, vw : 6.4625

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


bool ReadFile(const char *filePath, const int measureId, const int measureId2, const float idealVW
	, const int smallCount = 100 )
{
	list<string> exts;
	exts.push_back("volume");
	list<string> files;
	common::CollectFiles(exts, filePath, files);
	files.sort();

	vector<cVolumeResult> results;

	bool startRead = false;
	for (auto &fileName : files)
	{
		cVolumeResult result;
		if (result.Read(fileName.c_str()))
		{
			//if (result.m_measureId == measureId)
			if ((result.m_measureId >= measureId)
				&& (result.m_measureId <= measureId2))
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

	float vwAvr = 0;
	float vwErr = 0;
	for (auto &result : results)
	{
		float vw = 0;
		for (auto &volume : result.m_volumes)
			vw += volume.vw;
		vwErr += (abs(vw - idealVW) / idealVW) * 100.f;
		vwAvr += vw;
	}
	vwAvr /= results.size();
	vwErr /= results.size();

	// vw 차이가 가장 적은 걸 기준으로 정렬
	g_idealVW = idealVW;
	std::sort(results.begin(), results.end(), sort1);

	float vwAvr2 = 0;
	float vwErr2 = 0;
	for (u_int i = 0; i < (u_int)smallCount; ++i)
	{
		cVolumeResult &vr = results[i];
		float vw = 0;
		for (auto &volume : vr.m_volumes)
			vw += volume.vw;

		vwErr2 += (abs(vw - idealVW) / idealVW) * 100.f;
		vwAvr2 += vw;
	}
	vwAvr2 /= smallCount;
	vwErr2 /= smallCount;

	cout << fixed;
	cout.precision(2);

	cout << "file count = " << results.size() << endl;
	cout << "vwAvr = " << vwAvr << endl;
	cout << "vwErr = " << vwErr << endl;

	cout << "vwAvr2 = " << vwAvr2 << endl;
	cout << "vwErr2 = " << vwErr2 << endl;

	common::StrPath outFileName;
	outFileName.Format("output_%d_%d.txt", measureId, measureId2);	
	ofstream ofs(outFileName.c_str());
	if (ofs.is_open())
	{
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
		{
			float vw = 0;
			for (auto &volume : result.m_volumes)
				vw += volume.vw;
			ofs << vw << "\t";
		}
		ofs << endl;
	}

	return true;
}


// #1, 1m box
void measure9()
{
	const float idealVW = 166.66666f;
	ReadFile("..\\media\\TestData-2018-05-30-#1\\", 9, 9, idealVW);
}


// #4. 100cm + 50cm
void measure11()
{
	const float idealVW = 166.66666f + 20.8333333f; // 187.45f
	ReadFile("..\\media\\TestData-2018-05-30-#4\\", 11, 11, idealVW);
}

// #6. 100cm + 50cm + 30cm
void measure12()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	ReadFile("..\\media\\TestData-2018-05-30-#6\\", 12, 12, idealVW);
}

// #5. 100cm + 50cm
void measure13()
{
	const float idealVW = 166.66666f + 20.8333333f; // 187.45f
	ReadFile("..\\media\\TestData-2018-05-30-#5\\", 13, 13, idealVW);
}

// #8. 100cm + 50cm + 30cm
void measure14()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	ReadFile("..\\media\\TestData-2018-05-30-#8\\", 14, 14, idealVW);
}

// #7. 100cm + 50cm + 30cm
void measure15()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	ReadFile("..\\media\\TestData-2018-05-30-#7\\", 15, 15, idealVW);
}

// #2. 50cm
void measure16()
{
	const float idealVW = 20.8333333f;
	ReadFile("..\\media\\TestData-2018-05-30-#2\\", 16, 16, idealVW);
}

// #3. 30cm
void measure17()
{
	const float idealVW = 6.4625f;
	ReadFile("..\\media\\TestData-2018-05-30-#3\\", 17, 17, idealVW);
}

// #3. 30cm - move
void measure17_2()
{
	const float idealVW = 6.4625f;
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 18, 27, idealVW);

	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 18, 18, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 19, 19, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 20, 20, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 21, 21, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 22, 22, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 23, 23, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 24, 24, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 25, 25, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 26, 26, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#3-2\\", 27, 27, idealVW, 1);
}

// #6. 100cm + 50cm + 30cm - move
void measure28()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	//ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 28, 38, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 28, 28, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 29, 29, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 30, 30, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 31, 31, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 32, 32, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 33, 33, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 34, 34, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 35, 35, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 36, 36, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 37, 37, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#6-2\\", 38, 38, idealVW, 1);
}

// #7. 100cm + 50cm + 30cm - move
void measure39()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	//ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 39, 49, idealVW, 10);

	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 39, 39, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 40, 40, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 41, 41, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 42, 42, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 43, 43, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 44, 44, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 45, 45, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 46, 46, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 47, 47, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 48, 48, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#7-2\\", 49, 49, idealVW, 1);
}

// #8. 100cm + 50cm + 30cm - move
void measure50()
{
	const float idealVW = 166.66666f + 20.8333333f + 6.4625f; // 193.96f
	ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 50, 59, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 50, 50, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 51, 51, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 52, 52, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 53, 53, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 54, 54, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 55, 55, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 56, 56, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 57, 57, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 58, 58, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 59, 59, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 60, 60, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 61, 61, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 62, 62, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 63, 63, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 64, 64, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 65, 65, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#8-2\\", 66, 66, idealVW, 1);
}


// #5. 100cm + 50cm - move
void measure67()
{
	const float idealVW = 166.66666f + 20.8333333f; // 187.45f
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 67, 76, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 67, 67, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 68, 68, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 69, 69, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 70, 70, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 71, 71, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 72, 72, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 73, 73, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 74, 74, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 75, 75, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#5-2\\", 76, 76, idealVW, 1);
}

// #4. 100cm + 50cm - move
void measure77()
{
	const float idealVW = 166.66666f + 20.8333333f; // 187.45f
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 77, 86, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 77, 77, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 78, 78, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 79, 79, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 80, 80, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 81, 81, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 82, 82, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 83, 83, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 84, 84, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 85, 85, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#4-2\\", 86, 86, idealVW, 1);
}


// #1. 100cm - move
void measure87()
{
	const float idealVW = 166.66666f; // 166.666f
	//ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 87, 96, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 87, 87, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 88, 88, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 89, 89, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 90, 90, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 91, 91, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 92, 92, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 93, 93, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 94, 94, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 95, 95, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#1-2\\", 96, 96, idealVW, 1);

}

// #2. 50cm - move
void measure97()
{
	const float idealVW = 20.8333333f;
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 97, 107, idealVW, 10);

	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 97, 97, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 98, 98, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 99, 99, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 100, 100, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 101, 101, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 102, 102, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 103, 103, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 104, 104, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 105, 105, idealVW, 1);
	//ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 106, 106, idealVW, 1);
	ReadFile("..\\media\\TestData-2018-05-30-#2-2\\", 107, 107, idealVW, 1);
}



int main()
{
	//measure9();
	//measure11();
	//measure12();
	//measure13();
	//measure14();
	//measure15();
	//measure16();
	//measure17();
	//measure17_2();
	//measure28();
	//measure39();
	measure50();
	//measure67();
	//measure77();
	//measure87();
	//measure97();

	return 0;
}
