#pragma once
#include <string>
#include <vector>
#include "wav_header.h"

using namespace std;
class Wav {
public:
	void ReadHeader(const string& filename);
	void PrintInfo();
	void ExtracrDataInt16(string& filename);
	void MakeWavFile(const char* filename, int sample_rate);
	void MakeMono();
private:
	wav_header_s head;
	vector<vector<short>> channels;

	void HeadRefactor(int chan_count, int bits_per_sample, int sample_rate, int samples_count_per_chan);
	void null_header(wav_header_s* header_ptr);
	void CheckHeader();
};