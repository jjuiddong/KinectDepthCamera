//
// 2018-05-30, jjuiddong
// volume result (*.volume) reader
//
#pragma once


class cVolumeResult
{
public:
	struct sVolume {
		int id;
		float horz;
		float vert;
		float height;
		float volume;
		float vw;
		int pointCnt;

		sVolume() : horz(0), vert(0), height(0) {}
	};

	struct sContour {
		int id;
		int level;
		int loop;
		float lowerH;
		float upperH;
		vector<common::Vector2> vertices;
	};

	cVolumeResult();
	virtual ~cVolumeResult();

	bool Read(const char *fileName);
	void Add(const sVolume &volume);


public:
	int m_version;
	int m_measureId;
	int m_type;
	float m_error;
	vector<sVolume> m_volumes;
	vector<sContour> m_contours;
};
