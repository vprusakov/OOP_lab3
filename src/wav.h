#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "WavExceptions.h"
#include "wav_header.h"

using namespace std;
class Wav {
public:
	Wav(const std::string &filename);
	void ReadHeader();
	void PrintInfo();
	void ExtractDataInt16();
	void MakeWavFile(const std::string filename);
	void MakeMono();
	void MakeReverb(double delay_seconds, float decay);
	~Wav();
private:
	FILE *f;
	wav_header_s head;
	size_t data_size;
	vector<vector<short>> channels_data;

	void HeadRefactor(int chan_count, int sample_rate, int samples_count_per_chan);
	//void null_header(wav_header_s* header_ptr);
	void CheckHeader();
};