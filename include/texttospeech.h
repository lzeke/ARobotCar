#pragma once

#include <espeak-ng/speak_lib.h>

class TextToSpeech
{
public:
    TextToSpeech();
    bool Init();
    bool Talk(const char *text);

private:
    int buflength;
    unsigned int position;
    unsigned int end_position;
    unsigned int flags;
    espeak_POSITION_TYPE position_type;
    unsigned int *identifier;
    void *user_data;
    char *pLastText; // to remember last text, so not to repeat
};