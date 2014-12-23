/***************************************************************************//**
* @file    audio_codec_mp3.c
* @brief   MP3 audio format codec.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _AUDIO_CODEC_MP3_H_
#define _AUDIO_CODEC_MP3_H_

#include "audio_codec.h"

//------------------------------------------------------------------------------
#undef malloc
#undef free
#define CODEC_MP3_MEMORY    OS_MEM_RAM_INT_CCM
#define malloc(s)           OS_MallocEx(s, CODEC_MP3_MEMORY)
#define free(p)             OS_FreeEx(p, CODEC_MP3_MEMORY)

enum {
    S_AUDIO_CODEC_MP3_UNDEF = S_AUDIO_CODEC_LAST,
    S_AUDIO_CODEC_MP3_LAST
};

// Audio codec specific requests.
enum {
    AUDIO_CODEC_REQ_MP3_UNDEF = AUDIO_CODEC_REQ_STD_LAST,
    AUDIO_CODEC_REQ_MP3_TAG_ID3V2_GET,
    AUDIO_CODEC_REQ_MP3_LAST
};

//------------------------------------------------------------------------------
extern const AudioCodecItf audio_codec_mp3;

#endif // _AUDIO_CODEC_MP3_H_