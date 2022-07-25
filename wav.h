/**
 * # `wav.h`
 * Utilities for manipulating PCM WAV files.
 *
 * ## License
 *
 *     Author: Werner Stoop
 *     This is free and unencumbered software released into the public domain.
 *     See http://unlicense.org/ for more details.
 *
 * ## References
 *
 * * <https://en.wikipedia.org/wiki/WAV>
 * * <http://soundfile.sapp.org/doc/WaveFormat/>
 * * <http://www.topherlee.com/software/pcm-tut-wavformat.html>
 * * <https://blogs.msdn.microsoft.com/dawate/2009/06/23/intro-to-audio-programming-part-2-demystifying-the-wav-format/>
 * * <http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html>
 *
 * ## API
 *
 * ```
 * #include <stdint.h>
 * #include "wav.h"
 * ```
 *
 * ### Structures
 *
 * `typedef struct wav_t;`
 *
 * Structure to hold the WAV file's contents
 */
typedef struct {
	uint16_t num_channels;
	uint32_t sample_rate;
	uint16_t bits_per_sample;

	uint32_t abytes;
	uint32_t nbytes;
	char *bytes;
} wav_t;

/**
 * ### Functions
 *
 * #### `wav_t *wav_create(uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t initial_nsamples)`
 *
 * Creates a `wav_t` object.
 *
 * * `num_channels` - Number of channels in the WAV file.
 * * `sample_rate` - The sample rate in Hz.
 * * `bits_per_sample` - Number of bits per sample; 8 or 16.
 * * `initial_nsamples` - Initial number of samples to allocate space for.
 *        The space will be resized dynamically in `wav_add_samples()` if required.
 *        If set to 0, space will be allocated for 1 second worth of data.
 *
 * Free the returned object with `wav_free()` after use.
 */
wav_t *wav_create(uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t initial_nsamples);

/**
 * #### `void wav_free(wav_t *w)`
 *
 * Deallocates the memory allocated to the `wav_t` object `w`.
 */
void wav_free(wav_t *w);

/**
 * #### `int wav_add_samples(wav_t *w, void *samples, uint32_t nsamples)`
 *
 * Adds samples to the `wav_t` object `w`.
 *
 * `samples` is a pointer to the sample data to add; `nsamples` is the number of
 * samples in `samples`.
 *
 * The data in samples must match the `bits_per_sample` parameter of the original
 * `wav_create()` call that created the `wav_t` object. That is to say, `samples`
 * must contain `nsamples * bits_per_sample/8` bytes of data.
 *
 * It returns 1 on success, 0 on failure.
 */
int wav_add_samples(wav_t *w, void *samples, uint32_t nsamples);

/**
 * #### `wav_t *wav_load(const char *filename)`
 *
 * Loads a WAV file from a file named in `filename`.
 *
 * It returns `NULL` if the file could not be loaded.
 *
 * Free the returned object with `wav_free()` after use.
 */
wav_t *wav_load(const char *filename);

/**
 * #### `int wav_save(wav_t *w, const char *filename)`
 *
 * Saves the `wav_t` object `w` to the WAV file named `filename`.
 *
 * It returns 1 on success, 0 on failure.
 */
int wav_save(wav_t *w, const char *filename);
