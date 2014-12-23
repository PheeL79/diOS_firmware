/***************************************************************************//**
* @file    audio_codec.h
* @brief   Audio format codecs.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _AUDIO_CODEC_H_
#define _AUDIO_CODEC_H_

#include "os_audio.h"

//-----------------------------------------------------------------------------
typedef const void* AudioCodecHd;

enum {
    S_AUDIO_CODEC_UNDEF = S_MODULE,
    S_AUDIO_CODEC_FORMAT_UNSUPPORTED,
    S_AUDIO_CODEC_FORMAT_MISMATCH,
    S_AUDIO_CODEC_FORMAT_ERROR,
    S_AUDIO_CODEC_ENCODE_ERROR,
    S_AUDIO_CODEC_DECODE_ERROR,
    S_AUDIO_CODEC_NO_FRAME,
    S_AUDIO_CODEC_OUTPUT_BUFFER_FULL,
    S_AUDIO_CODEC_LAST
};

enum {
    AUDIO_CODEC_REQ_STD_UNDEF = 64,
    AUDIO_CODEC_REQ_STD_LAST
};

enum {
    AUDIO_CODEC_WAV,
    AUDIO_CODEC_MP3,
    AUDIO_CODEC_LAST,
    AUDIO_CODEC_UNDEF
};

typedef enum {
    AUDIO_FORMAT_WAV,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_LAST,
    AUDIO_FORMAT_UNDEF
} AudioFormat;

typedef struct {
    AudioFormat     format;
    Size            header_size;
    OS_AudioInfo    audio_info;
    void*           audio_info_ext[0];
} AudioFormatInfo;

typedef struct {
    Size            buf_in_offset;
    Size            buf_out_size;
//    OS_AudioInfo    audio_info;
} AudioFrameInfo;

//------------------------------------------------------------------------------
typedef struct {
    Status  (*Init)(void* args_p);
    Status  (*DeInit)(void* args_p);
    Status  (*Open)(void* args_p);
    Status  (*Close)(void* args_p);
    Status  (*IoCtl)(const U32 request_id, void* args_p);
    Status  (*Encode)(U8* data_in_p, Size size, void* args_p);
    Status  (*Decode)(U8* data_in_p, Size size_in, U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p);
    Status  (*IsFormat)(U8* data_in_p, Size size, AudioFormatInfo* info_p);
//    Status  (*FileExtensionsGet)(ConstStrP* file_ext_str_pp);
} AudioCodecItf;

//typedef struct {
//    ConstStr            name[OS_DRIVER_NAME_LEN];
//    AudioCodecItf*      itf_p;
//} AudioCodecConfig;

//-----------------------------------------------------------------------------
///// @brief      Create audio codec.
///// @param[in]  cfg_p          Codec's config.
///// @param[out] codec_hd_p     Codec's handle.
///// @return     #Status.
//Status          AudioCodecCreate(const AudioCodecConfig* cfg_p, AudioCodecHd* codec_hd_p);
//
///// @brief      Delete audio codec.
///// @param[in]  codec_hd       Codec's handle.
///// @return     #Status.
//Status          AudioCodecDelete(const AudioCodecHd codec_hd);

Status          AudioCodecInit(const AudioCodecHd codec_hd, void* args_p);
Status          AudioCodecDeInit(const AudioCodecHd codec_hd, void* args_p);
Status          AudioCodecOpen(const AudioCodecHd codec_hd, void* args_p);
Status          AudioCodecClose(const AudioCodecHd codec_hd, void* args_p);
Status          AudioCodecEncode(const AudioCodecHd codec_hd, U8* data_p, Size size, void* args_p);
Status          AudioCodecDecode(const AudioCodecHd codec_hd, U8* data_in_p,  Size size_in,
                                 U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p);
Status          AudioCodecIsFormat(const AudioCodecHd codec_hd, U8* data_p, Size size, AudioFormatInfo* info_p);
Status          AudioCodecIoCtl(const AudioCodecHd codec_hd, const U32 request_id, void* args_p);

AudioCodecHd    AudioCodecGet(const AudioFormat format);
Status          AudioFileFormatInfoGet(ConstStrP file_path_str_p, AudioFormatInfo* info_p);

#endif // _AUDIO_CODEC_H_