// This is for speech-to-text conversion. The idea is that we capture the audio through miniaudio.h using 
// the Capture() method below.
// Then we filter it and select the voice parts - this is done through Voice Activity Detection
// using the fvad library and the VoiceActivityDetection() method below.
// Finally, we submit the selected voice-segments to Google's speech-to-text API using the
// VoiceParsing() method of the GoogleSpeechToText class (in the googlespeechtotext library) and this
// return the desired text.

// for capturing
#include <stdio.h>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "speechtotext.h"

// for VAD (Voice Activity Detection)
#define AUDIO_SAMPLE_RATE 16000
#define MINIMUM_FILE_SIZE_TO_PROCESS 10000

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sndfile.h>
#include <filesystem>
#include <chrono>

// Google speech-to-text interface
#include "googlespeechtotext.h"

#define CAPTURE_LENGTH 3 // seconds

extern bool debug;
extern char *voiceString;

const char *SpeechToText::ProcessSpeech(const char *projectId)
{
    bool ret = false;
    const char *speechText = NULL;
    const char *captureFileName = "test.wav";
    const char *vadCleanedFileName = "speech.wav";

    if (std::filesystem::exists(captureFileName))
    {
        if (std::remove(captureFileName) != 0)
        {
            printf("ERROR:%s(): couldn't remove captureFile\n", __func__);
            ret = true;
        }
    }

    if (std::filesystem::exists(vadCleanedFileName))
    {
        if (std::remove(vadCleanedFileName) != 0)
        {
            printf("ERROR:%s(): couldn't remove vadCleanedFile\n", __func__);
            ret = true;
        }
    }

    if (!ret)
        this->Capture(captureFileName, CAPTURE_LENGTH); // this writes into test.wav

    if (!ret)
    {
        if (!std::filesystem::exists(captureFileName))
        {
            printf("ERROR:%s(): couldn't capture file\n", __func__);
            ret = true;
        }
    }

    // auto start = std::chrono::high_resolution_clock::now();
    if (!ret)
        ret = this->VoiceActivityDetection(captureFileName, vadCleanedFileName);

    if (!ret)
    {
        if (!std::filesystem::exists(vadCleanedFileName) || std::filesystem::file_size(vadCleanedFileName) < MINIMUM_FILE_SIZE_TO_PROCESS)
        {
            if (::debug)
                printf("There is no audio file to process after VAD\n");
            ret = true;
        }
    }

    if (::debug)
        printf("File size of vad file:%ld\n", std::filesystem::file_size(vadCleanedFileName));

    if (!ret)
        ret = (new GoogleSpeechToText())->VoiceParsing(projectId, vadCleanedFileName);

    if (!ret)
        speechText = ::voiceString;
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // printf("Speech processing took %d microseconds\n", duration.count());
    return speechText;
}

void SpeechToText::writeFloatSamplesToWavFile(const std::vector<float> &samples, const char *filename, ma_uint32 sampleRate, ma_uint32 channels)
{
    ma_encoder encoder;
    ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channels, sampleRate);

    if (ma_encoder_init_file(filename, &encoderConfig, &encoder) != MA_SUCCESS)
    {
        printf("ERROR: %s(): Failed to initialize output file.\n", __func__);
        return;
    }

    ma_encoder_write_pcm_frames(&encoder, samples.data(), samples.size(), NULL);

    ma_encoder_uninit(&encoder);
}

void SpeechToText::Capture(const char *fileName, int captureLengthSeconds)
{
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;
    std::vector<float> recordedAudio;

    // set up device
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_f32;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate = AUDIO_SAMPLE_RATE;
    deviceConfig.pUserData = &recordedAudio;

    deviceConfig.dataCallback = [](ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
    {
        std::vector<float> *audioData = static_cast<std::vector<float> *>(pDevice->pUserData);

        // Store the raw audio data in a vector
        const float *input = (const float *)pInput;
        for (ma_uint32 i = 0; i < frameCount * 1; i++)
        {
            audioData->push_back(input[i]);
        }

        (void)pOutput;
    };

    // start device
    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS)
    {
        printf("ERROR: %s(): Failed to initialize capture device.\n", __func__);
        return;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS)
    {
        ma_device_uninit(&device);
        printf("ERROR: %s(): Failed to start device.\n", __func__);
        return;
    }

    sleep(captureLengthSeconds); // this will be the length of the recording

    ma_device_uninit(&device);

    // printf("Number of vectors in the sample: %d\n", recordedAudio.size());

    if (recordedAudio.size() > 0)
    {
        // write vector of samples to a wav file
        writeFloatSamplesToWavFile(recordedAudio, fileName, AUDIO_SAMPLE_RATE, 1);
    }
}

// bool SpeechToText:: process_sf(SNDFILE *infile, Fvad *pVad, size_t framelen, SNDFILE *outfile)
bool SpeechToText::process_sf(SNDFILE *infile, Fvad *pVad, size_t framelen, SNDFILE *outfile)
{
    bool ret = false;
    double *buf0 = NULL;
    int16_t *buf1 = NULL;
    int vadres, prev = -1;
    long frames[2] = {0, 0};
    long segments[2] = {0, 0};

    if (framelen > SIZE_MAX / sizeof(double) || !(buf0 = (double *)malloc(framelen * sizeof *buf0)) || !(buf1 = (int16_t *)malloc(framelen * sizeof *buf1)))
    {
        printf("ERROR: %s(): failed to allocate buffers\n", __func__);
        ret = true;
    }

    while (!ret && sf_read_double(infile, buf0, framelen) == (sf_count_t)framelen)
    {
        // Convert the read samples to int16
        for (size_t i = 0; i < framelen; i++)
            buf1[i] = buf0[i] * INT16_MAX;

        vadres = fvad_process(pVad, buf1, framelen);
        if (vadres < 0)
        {
            printf("ERROR: %s(): fvad_process failed\n", __func__);
            ret = true;
            break;
        }

        vadres = !!vadres; // make sure it is 0 or 1

        if (vadres == 1)
        {
            sf_write_double(outfile, buf0, framelen);
        }

        frames[vadres]++;
        if (prev != vadres)
            segments[vadres]++;
        prev = vadres;
    }

    if (!ret)
    {
        // printf("voice detected in %ld of %ld frames (%.2f%%)\n",
        //     frames[1], frames[0] + frames[1],
        //     frames[0] + frames[1] ?
        //         100.0 * ((double)frames[1] / (frames[0] + frames[1])) : 0.0);
        // printf("%ld voice segments, average length %.2f frames\n",
        //     segments[1], segments[1] ? (double)frames[1] / segments[1] : 0.0);
        // printf("%ld non-voice segments, average length %.2f frames\n",
        //     segments[0], segments[0] ? (double)frames[0] / segments[0] : 0.0);
        if (::debug)
            printf("Number of voice segments: %ld\n", segments[1]);
    }

    if (buf0)
        free(buf0);
    if (buf1)
        free(buf1);

    return ret;
}

bool SpeechToText::VoiceActivityDetection(const char *in_fname, const char *out_fname)
{
    bool ret = false;
    int mode = 3; // 0-3, 3 being the most agressive noise supression
    SNDFILE *in_sf = NULL;
    SNDFILE *out_sf = NULL;
    SF_INFO in_info = {0};

    Fvad *pVad = fvad_new();
    if (!pVad)
    {
        printf("ERROR: %s(): pVad initialization fails.\n", __func__);
        ret = true;
    }

    if (!ret)
    {
        if (fvad_set_mode(pVad, mode) < 0)
        {
            printf("ERROR: %s(): fvad_set_mode fails.\n", __func__);
            ret = true;
        }
    }

    if (!ret)
    {
        in_sf = sf_open(in_fname, SFM_READ, &in_info);
        if (!in_sf)
        {
            printf("ERROR: %s(): sf_open fails for input file.\n", __func__);
            ret = true;
        }
    }

    if (!ret)
    {
        // we are picking the sample rate of the in file
        SF_INFO out_info = (SF_INFO){.samplerate = in_info.samplerate, .channels = 1, .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16};
        out_sf = sf_open(out_fname, SFM_WRITE, &out_info);
        if (!in_sf)
        {
            printf("ERROR: %s(): sf_open fails for output file.\n", __func__);
            ret = true;
        }
    }

    if (!ret)
    {
        if (in_info.channels != 1)
        {
            printf("ERROR: %s(): only single-channel wav files supported; input file has %d channels\n", __func__, in_info.channels);
            ret = true;
        }
    }

    if (!ret)
    {
        if (fvad_set_sample_rate(pVad, in_info.samplerate) < 0)
        {
            printf("ERROR: %s(): invalid sample rate: %d Hz\n", __func__, in_info.samplerate);
            ret = true;
        }
    }

    int frame_ms = 10; // 10msec

    if (!ret)
    {
        if (process_sf(in_sf, pVad, (size_t)in_info.samplerate / 1000 * frame_ms, out_sf))
        {
            printf("ERROR: %s(): process_sf fails\n", __func__);
            ret = true;
        }
    }

    if (in_sf)
        sf_close(in_sf);
    if (out_sf)
        sf_close(out_sf);
    if (pVad)
        fvad_free(pVad);

    return ret;
}
