#pragma once


template<unsigned int MAXSIZE>
struct sGraph
{
	int idx;
	float values[MAXSIZE];
	static const int size = MAXSIZE;

	void AddValue(const float v) {
		values[idx] = v;
		idx = (idx + 1) % ARRAYSIZE(values);
	}

	float GetCurValue() {
		const int i = (ARRAYSIZE(values) + (idx - 1)) % ARRAYSIZE(values);
		return values[i];
	}

	float GetPrevValue(const int offset = -2) {
		const int i = (ARRAYSIZE(values) + (idx + offset)) % ARRAYSIZE(values);
		return values[i];
	}

	float GetAverage()
	{
		float avr = 0;
		for (int i = 0; i < 10; ++i)
			avr += values[(ARRAYSIZE(values) + (idx - (i + 1))) % ARRAYSIZE(values)] * 0.1f;
		return avr;
	}
};
