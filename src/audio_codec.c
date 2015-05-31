/***************************************************************************//**
* @file    audio_codec.c
* @brief   Audio format codecs.
* @author  A. Filyanov
*******************************************************************************/
#include "os_common.h"
#include "os_debug.h"
#include "os_file_system.h"
#include "audio_codec.h"
#include "audio_codec_wav.h"
#include "audio_codec_mp3.h"
#undef malloc
#undef free
#include "os_memory.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define MDL_NAME                "audio_codec"
#undef  MDL_STATUS_ITEMS
#define MDL_STATUS_ITEMS        &status_codec_v[0]

const StatusItem status_codec_v[] = {
//audio codec common
    {"Undefined status"},
    {"Format unsupported"},
    {"Format mismatch"},
    {"Format error"},
    {"Encode error"},
    {"Decode error"},
    {"No frame found"},
    {"Output buffer full"}
};

//------------------------------------------------------------------------------
/// @brief   Init audio codecs.
/// @return  #Status.
Status AudioCodecsInit(void);

//-----------------------------------------------------------------------------
const AudioCodecItf* audio_codecs_v[AUDIO_CODEC_LAST];

/*****************************************************************************/
Status AudioCodecInit_(void);
Status AudioCodecInit_(void)
{
Status s = S_UNDEF;
    HAL_MemSet(audio_codecs_v, 0x0, sizeof(audio_codecs_v));
    audio_codecs_v[AUDIO_CODEC_WAV] = &audio_codec_wav;
    audio_codecs_v[AUDIO_CODEC_MP3] = &audio_codec_mp3;

    for (Size i = 0; i < AUDIO_CODEC_LAST; ++i) {
        OS_ASSERT_VALUE(audio_codecs_v[i]);
        OS_ASSERT_VALUE(audio_codecs_v[i]->Init);
        IF_STATUS(s = audio_codecs_v[i]->Init(OS_NULL)) {
            return s;
        }
    }
    return s;
}

/*****************************************************************************/
AudioCodecHd AudioCodecGet(const AudioFormat format)
{
AudioCodecHd audio_codec = audio_codecs_v[format];
    return audio_codec;
}

/*****************************************************************************/
Status AudioFileFormatInfoGet(ConstStrP file_path_str_p, AudioFormatInfo* info_p)
{
#define FILE_BUF_SIZE 0x4800
OS_FileHd file_hd;
Size file_buf_size = OS_MemoryFreeGet(OS_MEM_HEAP_APP);
    if (file_buf_size > FILE_BUF_SIZE) { file_buf_size = FILE_BUF_SIZE; }
void* file_buf_p = OS_MallocEx(file_buf_size, OS_MEM_HEAP_APP);
Status s = S_UNDEF;
    if (OS_NULL == info_p) { return s = S_INVALID_PTR; }
    if (OS_NULL == file_buf_p) { return s = S_OUT_OF_MEMORY; }
    IF_OK(s = OS_FileOpen(&file_hd, file_path_str_p,
                          BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) | BIT(OS_FS_FILE_OP_MODE_READ))) {
        IF_OK(s = OS_FileRead(file_hd, file_buf_p, FILE_BUF_SIZE)) {
            IF_OK(s = OS_FileClose(&file_hd)) {
                AudioCodecHd codec_hd;
                ConstStrP file_ext_p = OS_StrChr(file_path_str_p, '.');
                if (OS_NULL != file_ext_p) {
                    ++file_ext_p;
                    if (!OS_StrCmp(file_ext_p, "wav")) {
                        codec_hd = audio_codecs_v[AUDIO_CODEC_WAV];
                    } else if (!OS_StrCmp(file_ext_p, "mp3")) {
                        codec_hd = audio_codecs_v[AUDIO_CODEC_MP3];
                    } else { s = S_AUDIO_CODEC_FORMAT_UNSUPPORTED; }
                    IF_OK(s = AudioCodecOpen(codec_hd, OS_NULL)) {
                        IF_STATUS(s = AudioCodecIsFormat(codec_hd, file_buf_p, FILE_BUF_SIZE, info_p)) {
                            if (S_AUDIO_CODEC_FORMAT_MISMATCH == s) {
                                //May not enough buffer memory to find out the header format.
                            } else if (S_INVALID_SIZE == s) {
                            } else {
                            }
                            OS_LOG_S(D_WARNING, s);
                        }
                        IF_STATUS(AudioCodecClose(codec_hd, OS_NULL)) {}
                    } else { OS_LOG_S(D_WARNING, s); }
                }
            }
        }
    }
    OS_FreeEx(file_buf_p, OS_MEM_HEAP_APP);
    return s;
}

/*****************************************************************************/
Status AudioCodecInit(const AudioCodecHd codec_hd, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec init");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->Init);
    return ((AudioCodecItf*)codec_hd)->Init(args_p);
}

/*****************************************************************************/
Status AudioCodecDeInit(const AudioCodecHd codec_hd, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec deinit");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->DeInit);
    return ((AudioCodecItf*)codec_hd)->DeInit(args_p);
}

/*****************************************************************************/
Status AudioCodecOpen(const AudioCodecHd codec_hd, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec open");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->Open);
    return ((AudioCodecItf*)codec_hd)->Open(args_p);
}

/*****************************************************************************/
Status AudioCodecClose(const AudioCodecHd codec_hd, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec close");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->Close);
    return ((AudioCodecItf*)codec_hd)->Close(args_p);
}

/*****************************************************************************/
Status AudioCodecEncode(const AudioCodecHd codec_hd, U8* data_in_p, Size size, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec encode");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->Encode);
    return ((AudioCodecItf*)codec_hd)->Encode(data_in_p, size, args_p);
}

/*****************************************************************************/
Status AudioCodecDecode(const AudioCodecHd codec_hd, U8* data_in_p, Size size_in,
                        U8* data_out_p, Size size_out, AudioFrameInfo* frame_info_p)
{
    OS_LOG(D_DEBUG, "Audio codec decode");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->Decode);
    return ((AudioCodecItf*)codec_hd)->Decode(data_in_p, size_in, data_out_p, size_out, frame_info_p);
}

/*****************************************************************************/
Status AudioCodecIsFormat(const AudioCodecHd codec_hd, U8* data_in_p, Size size, AudioFormatInfo* info_p)
{
    OS_LOG(D_DEBUG, "Audio codec is format");
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->IsFormat);
    return ((AudioCodecItf*)codec_hd)->IsFormat(data_in_p, size, info_p);
}

/*****************************************************************************/
Status AudioCodecIoCtl(const AudioCodecHd codec_hd, const U32 request_id, void* args_p)
{
    OS_LOG(D_DEBUG, "Audio codec ioctl req: %u", request_id);
    OS_ASSERT_VALUE(codec_hd);
    OS_ASSERT_VALUE(((AudioCodecItf*)codec_hd)->IoCtl);
    return ((AudioCodecItf*)codec_hd)->IoCtl(request_id, args_p);
}

#endif //(OS_AUDIO_ENABLED)