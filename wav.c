/*
gcc -Wall -DWAV_TEST wav.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "wav.h"

#pragma pack(push, 1)
typedef struct {
	char 	 group_id[4];
	uint32_t filelength;
	char 	 riff_type[4];
} wav_header;

typedef struct {
	char     chunk_id[4];
	uint32_t chunksize;
	uint16_t audio_fmt;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
} fmt_chunk;

typedef struct {
	char     chunk_id[4];
	uint32_t chunksize;
} data_chunk;
#pragma pack(pop)

wav_t *wav_create(uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t initial_nsamples) {
	wav_t *w = malloc(sizeof *w);
	w->num_channels = num_channels;
	w->sample_rate = sample_rate;
	w->bits_per_sample = bits_per_sample;

	w->nbytes = 0;
	if(initial_nsamples) {
		w->abytes = initial_nsamples * bits_per_sample / 8;
	} else {
		w->abytes = sample_rate * bits_per_sample / 8;
	}
	w->bytes = malloc(w->abytes);
	return w;
}

void wav_free(wav_t *w) {
	free(w->bytes);
	free(w);
}

int wav_add_samples(wav_t *w, void *samples, uint32_t nsamples) {

	uint32_t nbytes = nsamples * w->bits_per_sample / 8;

	if(w->nbytes + nbytes > w->abytes) {
		while(w->nbytes + nbytes > w->abytes)
			w->abytes <<= 1;
		w->bytes = realloc(w->bytes, w->abytes);
		if(!w->bytes)
			return 0;
	}
	memcpy(w->bytes + w->nbytes, samples, nbytes);
	w->nbytes += nbytes;
	return 1;
}

wav_t *wav_load(const char *filename) {
	FILE *f = fopen(filename, "rb");
	if(!f)
		return NULL;

	wav_t *w = NULL;
	wav_header hdr;
	if(fread(&hdr, sizeof hdr, 1, f) != 1) {
		perror("header");
		goto end;
	}

	if(memcmp(hdr.group_id, "RIFF", 4)) {
		fprintf(stderr, "not RIFF");
		goto end;
	}

	if(memcmp(hdr.riff_type, "WAVE", 4)) {
		fprintf(stderr, "not WAVE");
		goto end;
	}

	fmt_chunk fmt;
	if(fread(&fmt, sizeof fmt, 1, f) != 1) {
		perror("format chunk");
		goto end;
	}

	if(memcmp(fmt.chunk_id, "fmt ", 4)) {
		fprintf(stderr, "not fmt??");
		goto end;
	}

	printf("length ............: %u bytes\n", hdr.filelength);

	printf("chunksize .........: %u bytes\n", fmt.chunksize);
	printf("audio_fmt .........: %u\n", fmt.audio_fmt);
	printf("num_channels ......: %u\n", fmt.num_channels);
	printf("sample_rate .......: %u Hz\n", fmt.sample_rate);
	printf("byte_rate .........: %u\n", fmt.byte_rate);
	printf("block_align .......: %u\n", fmt.block_align);
	printf("bits_per_sample ...: %u\n", fmt.bits_per_sample);

	if(fmt.audio_fmt != 1 || fmt.chunksize != 16) {
		fprintf(stderr, "Unsupported format");
	}

	data_chunk data;
	if(fread(&data, sizeof data, 1, f) != 1) {
		perror("data chunk");
		goto end;
	}
	if(memcmp(data.chunk_id, "data", 4)) {
		fprintf(stderr, "not data??");
		goto end;
	}
	printf("data size .........: %u\n", data.chunksize);

	w = wav_create(fmt.num_channels, fmt.sample_rate, fmt.bits_per_sample, data.chunksize);
	if(fread(w->bytes, 1, data.chunksize, f) != data.chunksize) {
		perror("fread samples");
		wav_free(w); w = NULL;
		goto end;
	}
	w->nbytes = data.chunksize;
	printf("%u bytes read\n", data.chunksize);
end:
	fclose(f);
	return w;
}

int wav_save(wav_t *w, const char *filename) {
	FILE *f = fopen(filename, "wb");
	if(!f || !w) return 0;
	int rv = 0;

	wav_header hdr;
	memcpy(&hdr.group_id, "RIFF", 4);
	memcpy(&hdr.riff_type, "WAVE", 4);
	if(fwrite(&hdr, sizeof hdr, 1, f) != 1)
		goto end;

	fmt_chunk fmt;
	memset(&fmt, 0, sizeof fmt);

	fmt.chunksize = 16;
	fmt.audio_fmt = 1;
	fmt.num_channels = w->num_channels;
	fmt.sample_rate = w->sample_rate;
	fmt.byte_rate = w->sample_rate * w->num_channels * w->bits_per_sample / 8;
	fmt.block_align = w->num_channels * w->bits_per_sample / 8;
	fmt.bits_per_sample = w->bits_per_sample;

	memcpy(&fmt.chunk_id, "fmt ", 4);
	if(fwrite(&fmt, sizeof fmt, 1, f) != 1)
		goto end;

	data_chunk data;
	memset(&data, 0, sizeof data);
	memcpy(&data.chunk_id, "data", 4);
	data.chunksize = w->nbytes;
	if(fwrite(&data, sizeof data, 1, f) != 1)
		goto end;
	if(fwrite(w->bytes, 1, w->nbytes, f) != w->nbytes)
		goto end;

	hdr.filelength = ftell(f) - 8;
	rewind(f);
	if(fwrite(&hdr, sizeof hdr, 1, f) != 1)
		goto end;

	rv = 1;

end:
	fclose(f);
	return rv;
}

#ifdef WAV_TEST
int main(int argc, char *argv[]) {
	if(argc > 1) {
		wav_t *w = wav_load(argv[1]);
		if(w) {
			if(argc > 2) {
				if(wav_save(w, argv[2])) {
					printf("save success");
				} else {
					printf("save fail");
				}
			}
			wav_free(w);
		} else {
			fprintf(stderr, "load fail %s\n", argv[1]);
		}
	} else {
		fprintf(stderr, "missing argument\n");
	}
	return 0;
}
#endif
