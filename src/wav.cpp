#include "wav.h"
#include "WavExceptions.h"

void Wav::ReadHeader(const string& filename)
{
	printf(">>>> read_header( %s )\n", filename);
	//null_header(header_ptr); // Fill header with zeroes.

	//FILE* f = fopen(filename.c_str(), "rb");
	FILE *f = FileException::OpenFile(filename, "rb");

	size_t blocks_read = fread(&head, sizeof(head), 1, f);
	if (blocks_read != 1) {
		// can't read header, because the file is too small.
		throw ExcBadFormat(filename);
	}

	fseek(f, 0, SEEK_END); // seek to the end of the file
	size_t file_size = ftell(f); // current position is a file size!
	fclose(f);

	CheckHeader();
}
void Wav::CheckHeader()
{
	if (head.chunkId[0] != 0x52 ||
		head.chunkId[1] != 0x49 ||
		head.chunkId[2] != 0x46 ||
		head.chunkId[3] != 0x46)
	{
		throw ExcHeader("HEADER_RIFF_ERROR\n");
	}

	if (head.chunkSize != file_size_bytes - 8) {
		throw ExcHeader("HEADER_FILE_SIZE_ERROR\n");
	}

	if (head.format[0] != 0x57 ||
		head.format[1] != 0x41 ||
		head.format[2] != 0x56 ||
		head.format[3] != 0x45)
	{
		throw ExcHeader("HEADER_WAVE_ERROR\n");
	}

	if (head.subchunk1Id[0] != 0x66 ||
		head.subchunk1Id[1] != 0x6d ||
		head.subchunk1Id[2] != 0x74 ||
		head.subchunk1Id[3] != 0x20)
	{
		throw ExcHeader("HEADER_FMT_ERROR\n");
	}

	if (head.audioFormat != 1) {
		throw ExcHeader("HEADER_NOT_PCM\n");
	}

	if (head.subchunk1Size != 16) {
		throw ExcHeader("HEADER_SUBCHUNK1_ERROR\n");
	}

	if (head.byteRate != head.sampleRate * head.numChannels * head.bitsPerSample / 8) {
		throw ExcHeader("HEADER_BYTES_RATE_ERROR\n");
	}

	if (head.blockAlign != head.numChannels * head.bitsPerSample / 8) {
		throw ExcHeader("HEADER_BLOCK_ALIGN_ERROR\n");
	}

	if (head.subchunk2Id[0] != 0x64 ||
		head.subchunk2Id[1] != 0x61 ||
		head.subchunk2Id[2] != 0x74 ||
		head.subchunk2Id[3] != 0x61)
	{
		throw ExcHeader("HEADER_FMT_ERROR\n");
	}

	if (head.subchunk2Size != file_size_bytes - 44)
	{
		throw ExcHeader("HEADER_SUBCHUNK2_SIZE_ERROR\n");
	}
}
void Wav::PrintInfo()
{
	printf("-------------------------\n");
	printf(" audioFormat   %u\n", head.audioFormat);
	printf(" numChannels   %u\n", head.numChannels);
	printf(" sampleRate    %u\n", head.sampleRate);
	printf(" bitsPerSample %u\n", head.bitsPerSample);
	printf(" byteRate      %u\n", head.byteRate);
	printf(" blockAlign    %u\n", head.blockAlign);
	printf(" chunkSize     %u\n", head.chunkSize);
	printf(" subchunk1Size %u\n", head.subchunk1Size);
	printf(" subchunk2Size %u\n", head.subchunk2Size);
	printf("-------------------------\n");
}

void Wav::ExtracrDataInt16(string& filename)
{
	if (head.bitsPerSample != 16) {
		throw FileIncorrectFormat("UNSUPPORTED_FORMAT: Only 16-bit samples is supported.");
	}

	FILE* f = fopen(filename.c_str(), "rb");
	if (!f) {
		throw ExcIO(filename);
	}
	fseek(f, 44, SEEK_SET); // Seek to the begining of PCM data.

	int chan_count = head.numChannels;
	int samples_per_chan = (head.subchunk2Size / sizeof(short)) / chan_count;

	// 1. Reading all PCM data from file to a single vector.
	std::vector<short> all_channels;
	all_channels.resize(chan_count * samples_per_chan);
	size_t read_bytes = fread(all_channels.data(), 1, head.subchunk2Size, f);
	if (read_bytes != head.subchunk2Size) {
		printf	("extract_data_int16() read only %zu of %u\n", read_bytes, head.subchunk2Size);
		return IO_ERROR;
	}
	fclose(f);


	// 2. Put all channels to its own vector.
	channels.resize(chan_count);
	for (size_t ch = 0; ch < channels.size(); ch++) {
		channels[ch].resize(samples_per_chan);
	}

	for (int ch = 0; ch < chan_count; ch++) {
		std::vector<short>& chdata = channels[ch];
		for (size_t i = 0; i < samples_per_chan; i++) {
			chdata[i] = all_channels[chan_count * i + ch];
		}
	}
}

void Wav::MakeWavFile(const char* filename, int sample_rate)
{
	printf(">>>> make_wav_file( %s )\n", filename);

	int chan_count = (int)channels.size();

	if (chan_count < 1) {
		return BAD_PARAMS;
	}

	int samples_count_per_chan = (int)chdata[0].size();

	// Verify that all channels have the same number of samples.
	for (size_t ch = 0; ch < chan_count; ch++) {
		if (chdata[ch].size() != (size_t)samples_count_per_chan) {
			return BAD_PARAMS;
		}
	}

	//err = fill_header(&header, chan_count, 16, sample_rate, samples_count_per_chan);
	if (err != WAV_OK) {
		return err;
	}

	std::vector<short> all_channels;
	all_channels.resize(chan_count * samples_count_per_chan);

	for (int ch = 0; ch < chan_count; ch++) {
		const std::vector<short>& chdata = channels[ch];
		for (size_t i = 0; i < samples_count_per_chan; i++) {
			all_channels[chan_count * i + ch] = chdata[i];
		}
	}

	FILE* f = fopen(filename, "wb");
	fwrite(&head, sizeof(head), 1, f);
	fwrite(all_channels.data(), sizeof(short), all_channels.size(), f);
	if (!f) {
		throw ExcIO(filename);
	}

	fclose(f);

	return WAV_OK;
}

void Wav::MakeMono()
{
	int chan_count = (int)channels.size();

	if (chan_count != 2) {
		return BAD_PARAMS;
	}

	int samples_count_per_chan = (int)channels[0].size();

	// Verify that all channels have the same number of samples.
	for (size_t ch = 0; ch < chan_count; ch++) {
		if (channels[ch].size() != (size_t)samples_count_per_chan) {
			return BAD_PARAMS;
		}
	}

	std::vector< std::vector<short> > dest_mono;
	dest_mono.resize(1);
	std::vector<short>& mono = dest_mono[0];
	mono.resize(samples_count_per_chan);
	// Mono channel is an arithmetic mean of all (two) channels.
	for (size_t i = 0; i < samples_count_per_chan; i++) {
		mono[i] = (channels[0][i] + channels[1][i]) / 2;
	}
	channels = dest_mono;
	HeadRefactor(1, head.bitsPerSample, head.sampleRate, samples_count_per_chan);
}

void Wav::HeadRefactor(int chan_count, int bits_per_sample, int sample_rate, int samples_count_per_chan)
{
	if (bits_per_sample != 16) {
		return UNSUPPORTED_FORMAT;
	}

	if (chan_count < 1) {
		return BAD_PARAMS;
	}

	int file_size_bytes = 44 + chan_count * (bits_per_sample / 8) * samples_count_per_chan;

	head.sampleRate = sample_rate;
	head.numChannels = chan_count;
	head.bitsPerSample = 16;

	head.chunkSize = file_size_bytes - 8;
	head.subchunk2Size = file_size_bytes - 44;

	head.byteRate = head.sampleRate * head.numChannels * head.bitsPerSample / 8;
	head.blockAlign = head.numChannels * head.bitsPerSample / 8;
}