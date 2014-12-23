/***************************************************************************//**
* @file    audio_codec_wav.h
* @brief   WAV PCM audio format codec.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _AUDIO_CODEC_WAV_H_
#define _AUDIO_CODEC_WAV_H_

#include "audio_codec.h"

//-----------------------------------------------------------------------------
enum {
    S_AUDIO_CODEC_WAV_UNDEF = S_AUDIO_CODEC_LAST,
    S_AUDIO_CODEC_WAV_LAST
};

//-----------------------------------------------------------------------------
extern const AudioCodecItf audio_codec_wav;

#endif // _AUDIO_CODEC_WAV_H_