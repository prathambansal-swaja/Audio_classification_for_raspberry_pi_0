#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

/* -----------------------------
   CONFIG
----------------------------- */
#define SAMPLE_RATE        16000
#define CHANNELS           1
#define FRAME_COUNT        EI_CLASSIFIER_RAW_SAMPLE_COUNT

/* -----------------------------
   AUDIO BUFFER
----------------------------- */
static int16_t mic_buffer[FRAME_COUNT];

/* -----------------------------
   SIGNAL CALLBACK
----------------------------- */
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = mic_buffer[offset + i] / 32768.0f;
    }
    return 0;
}

/* -----------------------------
   MAIN
----------------------------- */
int main() {

    /* -----------------------------
       ALSA SETUP
    ----------------------------- */
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;

    if (snd_pcm_open(&pcm_handle, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0) < 0) {
        printf("ERROR: Cannot open audio device\n");
        return 1;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    snd_pcm_hw_params_set_access(pcm_handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params,
                                 SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate(pcm_handle, params,
                               SAMPLE_RATE, 0);

    snd_pcm_hw_params(pcm_handle, params);
    snd_pcm_prepare(pcm_handle);

    printf("ALSA microphone initialized\n");

    /* -----------------------------
       SIGNAL WRAPPER
    ----------------------------- */
    signal_t signal;
    signal.total_length = FRAME_COUNT;
    signal.get_data = get_signal_data;

    /* -----------------------------
       INFERENCE LOOP
    ----------------------------- */
    while (true) {

        /* Capture audio */
        int frames = snd_pcm_readi(pcm_handle,
                                   mic_buffer,
                                   FRAME_COUNT);

        if (frames < 0) {
            snd_pcm_prepare(pcm_handle);
            continue;
        }

        /* Run inference */
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR err =
            run_classifier(&signal, &result, false);

        if (err != EI_IMPULSE_OK) {
            printf("Inference error: %d\n", err);
            continue;
        }

        /* Print results */
        printf("\nPredictions:\n");
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            printf("  %s: %.5f\n",
                   result.classification[i].label,
                   result.classification[i].value);
        }

        usleep(100000);  // throttle prints
    }

    snd_pcm_close(pcm_handle);
    return 0;
}
