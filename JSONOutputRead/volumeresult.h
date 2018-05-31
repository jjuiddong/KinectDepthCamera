//
// 2018-05-30, jjuiddong
// volume result (*.volume) reader
//
#pragma once


class cVolumeResult
{
public:
	cVolumeResult();
	virtual ~cVolumeResult();

	bool Read(const char *fileName);


public:
	struct sVolume {
		int id;
		float horz;
		float vert;
		float height;
		float volume;
		float vw;
		int pointCnt;
	};

	struct sContour {
		int id;
		int level;
		int loop;
		float lowerH;
		float upperH;
		vector<common::Vector2> vertices;
	};

	int m_version;
	int m_measureId;
	int m_type;
	vector<sVolume> m_volumes;
	vector<sContour> m_contours;
};
