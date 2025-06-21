#include "texttospeech.h"
#include <string.h>

TextToSpeech::TextToSpeech()
{
    this->buflength = 500;
    this->position = 0;
    this->end_position = 0;
    this->flags = espeakCHARS_AUTO;
    this->position_type = POS_CHARACTER;
    this->user_data = NULL;
    this->identifier = NULL;
    this->pLastText = new char[this->buflength];
}

bool TextToSpeech::Init()
{
    bool ret = false;

    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_PLAYBACK; // AUDIO_OUTPUT_SYNCH_PLAYBACK;
    char *path = NULL;
    char voicename[] = {"mb-us1"}; // Set voice by its name
    int options = 0;

    if (espeak_Initialize(output, buflength, path, options) == -1)
        ret = true;
    if (!ret && espeak_SetVoiceByName(voicename) != EE_OK)
        ret = true;
    if (!ret && espeak_SetParameter(espeakVOLUME, 200, 0) != EE_OK)
        ret = true;
    if (!ret && espeak_SetParameter(espeakWORDGAP, 5, 0) != EE_OK)
        ret = true;
    if (!ret && espeak_SetParameter(espeakRATE, 150, 0) != EE_OK)
        ret = true;

    // espeak_SetSynthCallback(SynthCallback);

    if (ret)
        printf("ERROR:Speech::%s() fails\n", __func__);

    return ret;
}

bool TextToSpeech::Talk(const char *text)
{
    bool ret = false;

    if (espeak_IsPlaying() == 0 && strcmp(this->pLastText, text) != 0)
    { // if we are not speaking and we wanted to say something that is not the same that we just said
        if (espeak_Synth(text, this->buflength, this->position, this->position_type,
                         this->end_position, this->flags, this->identifier, this->user_data) != EE_OK)
            ret = true;
        strcpy(this->pLastText, text);
    }
    else
    {
        ret = true;
    }

    return ret;
}