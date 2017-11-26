#include "wav.h"

Wav::Wav(const string &filename) {
	f = fopen(filename.c_str(), "rb");
	if (f == NULL) {
		throw IO_Exception("No such file");
	}
	ReadHeader();
	ExtractDataInt16();
}
Wav::~Wav() {
	fclose(f);
}

void Wav::ReadHeader()
{
	//printf(">>>> read_header( %s )\n", filename);
	//null_header(header_ptr); // Fill header with zeroes.
	
	size_t blocks_read = fread(&head, sizeof(head), 1, f);
	if (blocks_read != 1) {
		// can't read header, because the file is too small.
		throw Format_Exception("Head hasn't been read.\n");
	}


	fseek(f, 0, SEEK_END); // seek to the end of the file
	//size_t file_size = ftell(f); // current position is a file size!
	data_size = ftell(f);
	//fclose(f);
	fseek(f, 0, SEEK_SET);

	CheckHeader();
}
void Wav::CheckHeader() {
	if (head.chunkId[0] != 0x52 ||
		head.chunkId[1] != 0x49 ||
		head.chunkId[2] != 0x46 ||
		head.chunkId[3] != 0x46)
	{
		throw Header_Exception("HEADER_RIFF_ERROR\n");
	}

	if (head.chunkSize != data_size - 8) {
		throw Header_Exception("HEADER_FILE_SIZE_ERROR\n");
	}

	if (head.format[0] != 0x57 ||
		head.format[1] != 0x41 ||
		head.format[2] != 0x56 ||
		head.format[3] != 0x45)
	{
		throw Header_Exception("HEADER_WAVE_ERROR\n");
	}

	if (head.subchunk1Id[0] != 0x66 ||
		head.subchunk1Id[1] != 0x6d ||
		head.subchunk1Id[2] != 0x74 ||
		head.subchunk1Id[3] != 0x20)
	{
		throw Header_Exception("HEADER_FMT_ERROR\n");
	}

	if (head.audioFormat != 1) {
		throw Header_Exception("HEADER_NOT_PCM\n");
	}

	if (head.subchunk1Size != 16) {
		throw Header_Exception("HEADER_SUBCHUNK1_ERROR\n");
	}

	if (head.byteRate != head.sampleRate * head.numChannels * head.bitsPerSample / 8) {
		throw Header_Exception("HEADER_BYTES_RATE_ERROR\n");
	}

	if (head.blockAlign != head.numChannels * head.bitsPerSample / 8) {
		throw Header_Exception("HEADER_BLOCK_ALIGN_ERROR\n");
	}
	if (head.bitsPerSample != 16) {
		throw Header_Exception("Bits per sample must be 16\n");
	}
	if (head.subchunk2Id[0] != 0x64 ||
		head.subchunk2Id[1] != 0x61 ||
		head.subchunk2Id[2] != 0x74 ||
		head.subchunk2Id[3] != 0x61)
	{
		throw Header_Exception("HEADER_FMT_ERROR\n");
	}

	if (head.subchunk2Size != data_size - 44)
	{
		throw Header_Exception("HEADER_SUBCHUNK2_SIZE_ERROR\n");
	}
}
void Wav::PrintInfo() {
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

void Wav::HeadRefactor(int chan_count, int sample_rate, int samples_count_per_chan) {

	int file_size_bytes = 44 + chan_count * (head.bitsPerSample / 8) * samples_count_per_chan;

	head.sampleRate = sample_rate;
	head.numChannels = chan_count;
	head.bitsPerSample = 16;

	head.chunkSize = file_size_bytes - 8;
	head.subchunk2Size = file_size_bytes - 44;

	head.byteRate = head.sampleRate * head.numChannels * head.bitsPerSample / 8;
	head.blockAlign = head.numChannels * head.bitsPerSample / 8;
}
void Wav::ExtractDataInt16()
{

	fseek(f, 44, SEEK_SET); // Seek to the begining of PCM data.

	int chan_count = head.numChannels;
	int samples_per_chan = (head.subchunk2Size / sizeof(short)) / chan_count;

	// 1. Reading all PCM data from file to a single vector.
	std::vector<short> all_channels;
	all_channels.resize(chan_count * samples_per_chan);
	size_t read_bytes = fread(all_channels.data(), 1, head.subchunk2Size, f);
	if (read_bytes != head.subchunk2Size) {
		printf("extract_data_int16() read only %zu of %u\n", read_bytes, head.subchunk2Size);
		throw Format_Exception("PCM data is bigger than it is declared in subchunk2Size.\n");
	}
	//fclose(f);

	// 2. Put all channels to its own vector.
	channels_data.resize(chan_count);
	for (size_t ch = 0; ch < channels_data.size(); ch++) {
		channels_data[ch].resize(samples_per_chan);
	}

	for (int ch = 0; ch < chan_count; ch++) {
		std::vector<short>& chdata = channels_data[ch];
		for (size_t i = 0; i < samples_per_chan; i++) {
			chdata[i] = all_channels[chan_count * i + ch];
		}
	}
	fseek(f, 0, SEEK_SET);
}

void Wav::MakeWavFile(const std::string filename) {
	//printf(">>>> make_wav_file( %s )\n", filename);

	int chan_count = head.numChannels;

	int samples_count_per_chan = (int)channels_data[0].size();

	// Verify that all channels have the same number of samples.
	for (size_t ch = 0; ch < chan_count; ch++) {
		if (channels_data[ch].size() != (size_t)samples_count_per_chan) {
			throw Format_Exception("Samples per channel differ from channel to channel\n");
		}
	}

	std::vector<short> all_channels;
	all_channels.resize(chan_count * samples_count_per_chan);

	for (int ch = 0; ch < chan_count; ch++) {
		const std::vector<short>& chdata = channels_data[ch];
		for (size_t i = 0; i < samples_count_per_chan; i++) {
			all_channels[chan_count * i + ch] = chdata[i];
		}
	}

	FILE* nf = fopen(filename.c_str(), "wb");
	if (nf == NULL) {
		throw ("Can't create/write to file.\n");
	}
	fwrite(&head, sizeof(wav_header_s), 1, nf);
	fwrite(all_channels.data(), sizeof(short), all_channels.size(), nf);
	fclose(nf);
}

void Wav::MakeMono()
{
	int chan_count = (int)channels_data.size();

	if (chan_count != 2 || chan_count == 1) {
		throw Parameters_Exception("Can't make mono out of " + std::to_string(chan_count) + " channel.\n");
	}

	int samples_count_per_chan = (int)channels_data[0].size();

	// Verify that all channels have the same number of samples.
	for (size_t ch = 0; ch < chan_count; ch++) {
		if (channels_data[ch].size() != (size_t)samples_count_per_chan) {
			throw Format_Exception("Samples per channel differ from channel to channel\n");
		}
	}

	std::vector< std::vector<short> > dest_mono;
	dest_mono.resize(1);
	std::vector<short>& mono = dest_mono[0];
	mono.resize(samples_count_per_chan);
	// Mono channel is an arithmetic mean of all (two) channels.
	for (size_t i = 0; i < samples_count_per_chan; i++) {
		mono[i] = (channels_data[0][i] + channels_data[1][i]) / 2;
	}
	channels_data = dest_mono;
	HeadRefactor(1, head.sampleRate, samples_count_per_chan);
}
void Wav::MakeReverb(double delay_seconds, float decay)
{
	int chan_count = (int)channels_data.size();
	int sample_rate = head.sampleRate;

	if (chan_count < 1) {
		throw Format_Exception("Channel count can't be fewer than 1");
	}

	int samples_count_per_chan = (int)channels_data[0].size();

	// Verify that all channels have the same number of samples.
	for (size_t ch = 0; ch < chan_count; ch++) {
		if (channels_data[ch].size() != (size_t)samples_count_per_chan) {
			throw Format_Exception("Samples per channel differ from channel to channel\n");
		}
	}

	int delay_samples = (int)(delay_seconds * sample_rate);


	for (size_t ch = 0; ch < chan_count; ch++) {
		std::vector<float> tmp;
		tmp.resize(channels_data[ch].size());

		// Convert signal from short to float
		for (size_t i = 0; i < samples_count_per_chan; i++) {
			tmp[i] = channels_data[ch][i];
		}

		// Add a reverb
		for (size_t i = 0; i < samples_count_per_chan - delay_samples; i++) {
			tmp[i + delay_samples] += decay * tmp[i];
		}

		// Find maximum signal's magnitude
		float max_magnitude = 0.0f;
		for (size_t i = 0; i < samples_count_per_chan - delay_samples; i++) {
			if (abs(tmp[i]) > max_magnitude) {
				max_magnitude = abs(tmp[i]);
			}
		}

		// Signed short can keep values from -32768 to +32767,
		// After reverb, usually there are values large 32000.
		// So we must scale all values back to [ -32768 ... 32768 ]
		float norm_coef = 30000.0f / max_magnitude;
		printf("max_magnitude = %.1f, coef = %.3f\n", max_magnitude, norm_coef);

		// Scale back and transform floats to shorts.
		for (size_t i = 0; i < samples_count_per_chan; i++) {
			channels_data[ch][i] = (short)(norm_coef * tmp[i]);
		}
	}
}