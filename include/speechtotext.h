#pragma once

#include <vector>
#include "miniaudio.h"
#include <fvad.h>
#include <sndfile.h>

class SpeechToText
{
public:
    const char *ProcessSpeech(const char *projectId);

private:
    void Capture(const char *fileName, int captureLengthSeconds);
    void writeFloatSamplesToWavFile(const std::vector<float> &samples, const char *filename, ma_uint32 sampleRate, ma_uint32 channels);
    bool process_sf(SNDFILE *infile, Fvad *vad, size_t framelen, SNDFILE *outfile);
    bool VoiceActivityDetection(const char *in_fname, const char *out_fname);
};