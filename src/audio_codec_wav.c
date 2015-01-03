/***************************************************************************//**
* @file    audio_codec_wav.c
* @brief   WAV PCM audio format codec.
* @author  A. Filyanov
*******************************************************************************/
#include "os_common.h"
#include "os_debug.h"
#include "os_file_system.h"
#include "audio_codec_wav.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define MDL_NAME                "aud_codec_wav"
#undef  MDL_STATUS_ITEMS
#define MDL_STATUS_ITEMS        &status_codec_wav_v[0]

const StatusItem status_codec_wav_v[] = {
//audio codec common
    {"Undefined status"},
    {"Format unsupported"},
    {"Format mismatch"},
    {"Format error"},
    {"Encode error"},
    {"Decode error"},
    {"No frame found"},
    {"Output buffer full"}
//audio codec custom
};

//------------------------------------------------------------------------------
typedef struct {
    U32   chunk_id;       /* 0 */
    U32   file_size;      /* 4 */
    U32   file_format;    /* 8 */
    U32   sub_chunk1_id;  /* 12 */
    U32   sub_chunk1_size;/* 16*/
    U16   audio_format;   /* 20 */
    U16   nbr_channels;   /* 22 */
    U32   sample_rate;    /* 24 */

    U32   byte_rate;      /* 28 */
    U16   block_align;    /* 32 */
    U16   bits_per_sample;/* 34 */
    U32   sub_chunk2_id;  /* 36 */
    U32   sub_chunk2_size;/* 40 */
} AudioFormatHeaderWav;

//------------------------------------------------------------------------------
static Status Init(void* args_p);
static Status DeInit(void* args_p);
static Status Open(void* args_p);
static Status Close(void* args_p);
static Status Decode(U8* data_in_p, Size size_in, U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p);
static Status IsFormat(U8* data_in_p, Size size, AudioFormatInfo* info_p);
//static Status IoCtl(const U32 request_id, void* args_p);

//------------------------------------------------------------------------------
static ConstStrP file_extensions_str = "wav";

const AudioCodecItf audio_codec_wav = {
    .Init           = Init,
    .DeInit         = DeInit,
    .Open           = Open,
    .Close          = Close,
    .Encode         = OS_NULL,
    .Decode         = Decode,
    .IsFormat       = IsFormat,
//    .IoCtl          = IoCtl
};

/*****************************************************************************/
Status Init(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/*****************************************************************************/
Status DeInit(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/*****************************************************************************/
Status Open(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/*****************************************************************************/
Status Close(void* args_p)
{
Status s = S_UNDEF;
    s = S_OK;
    return s;
}

/*****************************************************************************/
Status Decode(U8* data_in_p, Size size_in, U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p)
{
Status s = S_OK;
    OS_MemCpy(data_out_p, data_in_p, size_out);
    frame_info_p->buf_out_size = size_out;
    return s;
}

/*****************************************************************************/
Status IsFormat(U8* data_in_p, Size size, AudioFormatInfo* info_p)
{
const AudioFormatHeaderWav* wav_hdr_p = (AudioFormatHeaderWav*)data_in_p;
const Size header_size = sizeof(AudioFormatHeaderWav);
    if (header_size > size) {
        return S_SIZE_MISMATCH;
    }
    if ((0x46464952 == wav_hdr_p->chunk_id) && (1 == wav_hdr_p->audio_format)) {
        if (OS_NULL != info_p) {
            info_p->format                  = AUDIO_FORMAT_WAV;
            info_p->header_size             = header_size;
            info_p->audio_info.sample_rate  = wav_hdr_p->sample_rate;
            info_p->audio_info.sample_bits  = wav_hdr_p->bits_per_sample;
            info_p->audio_info.channels     = (1 == wav_hdr_p->nbr_channels) ? OS_AUDIO_CHANNELS_MONO :
                                                  (2 == wav_hdr_p->nbr_channels) ? OS_AUDIO_CHANNELS_STEREO :
                                                      OS_AUDIO_CHANNELS_UNDEF;
            return S_OK;
        }
    }
    return S_AUDIO_CODEC_FORMAT_MISMATCH;
}

/*****************************************************************************/
//Status IoCtl(const U32 request_id, void* args_p)
//{
//Status s = S_UNDEF;
//    switch (request_id) {
//// Standard audio codec's requests.
//        default:
//            s = S_UNDEF_REQ_ID;
//            break;
//    }
//    return s;
//}

#endif //(OS_AUDIO_ENABLED)