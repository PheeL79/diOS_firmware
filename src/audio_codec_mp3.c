/***************************************************************************//**
* @file    audio_codec_mp3.c
* @brief   MP3 audio format codec.
* @author  A. Filyanov
*******************************************************************************/
#include "os_common.h"
#include "os_debug.h"
#include "os_file_system.h"
#include "audio_codec_mp3.h"
#include "mp3dec.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define MDL_NAME                "aud_codec_mp3"
#undef  MDL_STATUS_ITEMS
#define MDL_STATUS_ITEMS        &status_codec_mp3_v[0]

const StatusItem status_codec_mp3_v[] = {
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
typedef MP3FrameInfo AudioFormatHeaderMp3;

//------------------------------------------------------------------------------
static Status Init(void* args_p);
static Status DeInit(void* args_p);
static Status Open(void* args_p);
static Status Close(void* args_p);
static Status Decode(U8* data_in_p, Size size_in, U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p);
static Status IsFormat(U8* data_in_p, Size size, AudioFormatInfo* info_p);
static Status IoCtl(const U32 request_id, void* args_p);

//------------------------------------------------------------------------------
static ConstStrP file_extensions_str = "mp3";
static HMP3Decoder* mp3_decoder_hd;

const AudioCodecItf audio_codec_mp3 = {
    .Init           = Init,
    .DeInit         = DeInit,
    .Open           = Open,
    .Close          = Close,
    .Encode         = OS_NULL,
    .Decode         = Decode,
    .IsFormat       = IsFormat,
    .IoCtl          = IoCtl
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
    mp3_decoder_hd = MP3InitDecoder(); //Currently supports only one instance!
    if (OS_NULL != mp3_decoder_hd) {
        s = S_OK;
    } else { s = S_INVALID_PTR; }
    return s;
}

/*****************************************************************************/
Status Close(void* args_p)
{
Status s = S_UNDEF;
    if (OS_NULL != mp3_decoder_hd) {
        MP3FreeDecoder(mp3_decoder_hd);
        mp3_decoder_hd = OS_NULL;
        s = S_OK;
    } else { s = S_INVALID_PTR; }
    return s;
}

/*****************************************************************************/
Status Decode(U8* data_in_p, Size size_in, U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p)
{
U8* data_in_tmp_p = data_in_p;
U8* data_out_tmp_p= data_out_p;
Int offset = MP3FindSyncWord(data_in_p, size_in);
MP3FrameInfo frame_info;
Status s = S_OK;

    while ((0 <= offset) && (0 < size_in)) {
        data_in_p += offset;
        size_in   -= offset;
        Int res = MP3GetNextFrameInfo(mp3_decoder_hd, &frame_info, data_in_p);
        if (ERR_MP3_NONE == res) {
            Int frame_size_u8 = frame_info.outputSamps * BIT_SHIFT_RIGHT(frame_info.bitsPerSample, 3);
            // Is space for the frame data in the output buffer?
            if (frame_size_u8 > size_out) { // no
                s = S_AUDIO_CODEC_OUTPUT_BUFFER_FULL;
                break;
            }
//            if (frame_info_p->audio_info.sample_rate != frame_info.samprate) {
//                break;
//            }
            res = MP3Decode(mp3_decoder_hd, (unsigned char**)&data_in_p, (int*)&size_in, (short*)data_out_p, 0);
            if (ERR_MP3_NONE == res) {
                data_out_p += frame_size_u8;
                size_out   -= frame_size_u8;
            } else if ((ERR_MP3_INDATA_UNDERFLOW    == res) ||
                       (ERR_MP3_MAINDATA_UNDERFLOW  == res)) {
                MP3GetLastFrameInfo(mp3_decoder_hd, &frame_info);
                frame_size_u8 = frame_info.outputSamps * BIT_SHIFT_RIGHT(frame_info.bitsPerSample, 3);
                data_out_p += frame_size_u8;
                size_out   -= frame_size_u8;
            } else {
                //Check in "os_config.h" for "#define OS_FILE_SYSTEM_WORD_ACCESS 0"!!!
                OS_LOG_S(D_WARNING, (s = S_AUDIO_CODEC_DECODE_ERROR));
            }
        } else {
            //Try to find next valid frame.
            ++data_in_p;
            --size_in;
        }
        offset = MP3FindSyncWord(data_in_p, size_in);
    }
    if (data_in_p - data_in_tmp_p) {
        OS_MemCpy(data_in_tmp_p, data_in_p, size_in);
    } else {
        size_in = 0;
    }
    frame_info_p->buf_in_offset = size_in;
    frame_info_p->buf_out_size  = (data_out_p - data_out_tmp_p);
//    frame_info_p->audio_info.sample_rate    = frame_info.samprate;
//    frame_info_p->audio_info.sample_bits    = frame_info.bitsPerSample;
//    frame_info_p->audio_info.channels       = frame_info.nChans;
    return s;
}

/*****************************************************************************/
Status IsFormat(U8* data_in_p, Size size, AudioFormatInfo* info_p)
{
Int offset = MP3FindSyncWord(data_in_p, size);
Size header_size = size;

    while (0 <= offset) {
        AudioFormatHeaderMp3 mp3_hdr;
        data_in_p += offset;
        size      -= offset;
        if (ERR_MP3_NONE == MP3GetNextFrameInfo(mp3_decoder_hd, &mp3_hdr, data_in_p)) {
            if (OS_NULL != info_p) {
                info_p->format                  = AUDIO_FORMAT_MP3;
                info_p->header_size             = header_size - size;
                info_p->audio_info.sample_rate  = mp3_hdr.samprate;
                info_p->audio_info.sample_bits  = mp3_hdr.bitsPerSample;
                info_p->audio_info.channels     = (1 == mp3_hdr.nChans) ? OS_AUDIO_CHANNELS_MONO :
                                                      (2 == mp3_hdr.nChans) ? OS_AUDIO_CHANNELS_STEREO :
                                                          OS_AUDIO_CHANNELS_UNDEF;
                return S_OK;
            }
        } else {
            //Try to find next valid frame.
            ++data_in_p;
            --size;
        }
        offset = MP3FindSyncWord(data_in_p, size);
    }
    return S_AUDIO_CODEC_FORMAT_MISMATCH;
}

/*****************************************************************************/
Status IoCtl(const U32 request_id, void* args_p)
{
Status s = S_UNDEF;
    switch (request_id) {
// Standard audio codec's requests.
// Specific audio codec's requests.
        case AUDIO_CODEC_REQ_MP3_TAG_ID3V2_GET:
            s = S_OK;
            break;
        default:
            s = S_INVALID_REQ_ID;
            break;
    }
    return s;
}

#endif //(OS_AUDIO_ENABLED)