// Convert pcd ascii file to pcd binary file

#include "stdafx.h"

int main()
{
	using namespace std;
	
	list<string> exts;
	exts.push_back("pcd");
	list<string> files;

	common::CollectFiles(exts, "c:\\Project\\Media\\DepthSave\\0", files);

	for (auto &fileName : files)
	{
		ifstream ifs(fileName.c_str());
		if (!ifs.is_open())
			continue;

		vector<float> datas;
		string line;
		while (getline(ifs, line))
		{
			stringstream ss(line);
			while (ss)
			{
				float v;
				ss >> v;
				datas.push_back(v);
			}
		}

		int a = 0;


	}

    return 0;
}

