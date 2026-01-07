#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

/* --------------------------------------------------
   CONFIG
-------------------------------------------------- */
#define WAV_SAMPLE_COUNT EI_CLASSIFIER_RAW_SAMPLE_COUNT

/* --------------------------------------------------
   WAV PCM buffer (16-bit signed)
   Fill this by reading a .wav file
-------------------------------------------------- */
static int16_t wav_pcm[WAV_SAMPLE_COUNT];

/* --------------------------------------------------
   SIGNAL CALLBACK
-------------------------------------------------- */
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        // Normalize int16 → float (-1.0 to 1.0)
        out_ptr[i] = wav_pcm[offset + i] / 32768.0f;
    }
    return 0;
}

/* --------------------------------------------------
   MAIN
-------------------------------------------------- */
int main() {

    /* --------------------------------------------------
       1. LOAD WAV FILE INTO wav_pcm[]
       (example: fread, file I/O, SD card, flash, etc.)
       -------------------------------------------------- */

    FILE *f = fopen("test.wav", "rb");
    if (!f) {
        printf("Failed to open WAV file\n");
        return 1;
    }

    // Skip WAV header (44 bytes for standard PCM)
    fseek(f, 44, SEEK_SET);

    fread(wav_pcm, sizeof(int16_t), WAV_SAMPLE_COUNT, f);
    fclose(f);

    /* --------------------------------------------------
       2. CREATE SIGNAL
       -------------------------------------------------- */
    signal_t signal;
    signal.total_length = WAV_SAMPLE_COUNT;
    signal.get_data = get_signal_data;

    /* --------------------------------------------------
       3. RUN CLASSIFIER
       -------------------------------------------------- */
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    if (err != EI_IMPULSE_OK) {
        printf("Classifier error: %d\n", err);
        return 1;
    }

    /* --------------------------------------------------
       4. PRINT RESULTS
       -------------------------------------------------- */
    printf("Timing:\n");
    printf("  DSP: %d ms\n", result.timing.dsp);
    printf("  Classification: %d ms\n", result.timing.classification);

    printf("Predictions:\n");
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        printf("  %s: %.5f\n",
               result.classification[i].label,
               result.classification[i].value);
    }

    return 0;
}
