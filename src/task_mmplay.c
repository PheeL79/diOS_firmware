/***************************************************************************//**
* @file    task_mmplay.c
* @brief   Multimedia player task.
* @author  A. Filyanov
*******************************************************************************/
#include "drv_audio.h"
#include "os_debug.h"
#include "os_signal.h"
#include "os_audio.h"
#include "os_environment.h"
#include "os_task_audio.h"
#include "task_mmplay.h"

//-----------------------------------------------------------------------------
#define MDL_NAME            "task_mmplay"
#undef  MDL_STATUS_ITEMS
#define MDL_STATUS_ITEMS    &status_mmplay_v[0]

#define AUDIO_BUF_SIZE      0x4000
#define AUDIO_BUF_MEMORY    OS_MEM_RAM_EXT_SRAM
#define AUDIO_DMA_SIZE_MAX  U16_MAX

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
} FileAudioFormatWav;

//------------------------------------------------------------------------------
static Status           Play(TaskArgs* task_args_p, const FileAudioFormat format);
static FileAudioFormat  AudioFormatGet(void* file_audio_data_p);
static Status           FileAudioFormatGet(ConstStrPtr file_path_str_p, FileAudioFormat* format_p);
static Status           FileAudioWavInfoGet(ConstStrPtr file_path_str_p, FileAudioFormatWav* info_p);
//static void             AudioBitRateConvert(void* data_in_p, void* data_out_p, Size size,
//                                            const OS_AudioBits bit_rate_in, const OS_AudioBits bit_rate_out);
static void             VolumeApply(void* data_out_p, Size size, const OS_AudioBits bit_rate, const OS_AudioVolume volume);
static void             AudioDecode(void* data_out_p, Size size, const FileAudioFormat format);
static void             DecodeMp3(void* data_out_p, Size size);

//------------------------------------------------------------------------------
TaskArgs task_mmplay_args;

const StatusItem status_mmplay_v[] = {
//file system
    {"Undefined status"},
    {"Unsupported format"},
    {"Format mismatch"}
};

//------------------------------------------------------------------------------
OS_TaskConfig task_mmplay_cfg = {
    .name       = OS_TASK_NAME_MMPLAY,
    .func_main  = OS_TaskMain,
    .func_power = OS_TaskPower,
    .args_p     = (void*)&task_mmplay_args,
    .attrs      = BIT(OS_TASK_ATTR_SINGLE),
    .timeout    = 4,
    .prio_init  = OS_TASK_PRIO_NORMAL,
    .prio_power = OS_PWR_PRIO_DEFAULT + 3,
    .stack_size = OS_STACK_SIZE_MIN,
    .stdin_len  = OS_STDIN_LEN
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
FileAudioFormat* audio_format_p = &(task_args_p->audio_format);
Status s = S_UNDEF;

    task_args_p->state = MMPLAY_STATE_UNDEF;
    //Check file format.
    IF_OK(s = FileAudioFormatGet(task_args_p->file_path_str_p, audio_format_p)) {
        if (FILE_AUDIO_FORMAT_UNDEF != *audio_format_p) {
            task_args_p->audio_format =*audio_format_p;
            task_args_p->audio_dev_hd = OS_AudioDeviceDefaultGet(DIR_OUT);
            if (OS_NULL != task_args_p->audio_dev_hd) {
                OS_AudioInfo* audio_info_p = &(task_args_p->audio_info);
                if (FILE_AUDIO_FORMAT_WAV == *audio_format_p) {
                    FileAudioFormatWav wav_info;
                    IF_OK(s = FileAudioWavInfoGet(task_args_p->file_path_str_p, &wav_info)) {
                        audio_info_p->sample_rate  = wav_info.sample_rate;
                        audio_info_p->sample_bits  = wav_info.bits_per_sample;
                        audio_info_p->channels     = (1 == wav_info.nbr_channels) ? OS_AUDIO_CHANNELS_MONO :
                                                         (2 == wav_info.nbr_channels) ? OS_AUDIO_CHANNELS_STEREO :
                                                              OS_AUDIO_CHANNELS_UNDEF;
                    }
//                } else if (FILE_AUDIO_FORMAT_MP3 == *audio_format_p) {
//                    FileAudioFormatMp3 mp3_info;
//                    IF_OK(s = FileAudioMp3InfoGet(task_args_p->file_path_str_p, &mp3_info)) {
//                        audio_info_p->sample_rate  = mp3_info.sample_rate;
//                        audio_info_p->sample_bits  = mp3_info.bits_per_sample;
//                        audio_info_p->channels     = mp3_info.nbr_channels;
//                    }
                } else { s = S_MMPLAY_FORMAT_UNSUPPORTED; }
                IF_OK(s = OS_AudioDeviceIoSetup(task_args_p->audio_dev_hd, task_args_p->audio_info, DIR_OUT)) {
                    task_args_p->audio_buf_size = AUDIO_BUF_SIZE;
                    task_args_p->audio_buf_p    = OS_MallocEx(task_args_p->audio_buf_size, AUDIO_BUF_MEMORY);
                    if (OS_NULL == task_args_p->audio_buf_p) { return s = S_NO_MEMORY; }
                    IF_OK(s = OS_FileOpen(&task_args_p->file_hd, task_args_p->file_path_str_p,
                                          BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) | BIT(OS_FS_FILE_OP_MODE_READ))) {
                        IF_OK(s = OS_AudioDeviceOpen(task_args_p->audio_dev_hd, OS_NULL)) {
                            IF_OK(s = Play(task_args_p, task_args_p->audio_format)) { //Play the audio file.
                            }
                            IF_STATUS(s) {
                                IF_OK(s = OS_AudioDeviceClose(task_args_p->audio_dev_hd)) {}
                            }
                        }
                        IF_STATUS(s) {
                            IF_OK(s = OS_FileClose(&task_args_p->file_hd)) {}
                        }
                    }
                    IF_STATUS(s) {
                        OS_FreeEx(task_args_p->audio_buf_p, AUDIO_BUF_MEMORY);
                    }
                }
            } else { s = S_INVALID_REF; }
        } else { s = S_MMPLAY_FORMAT_UNSUPPORTED; }
    }
    IF_STATUS(s) { OS_LOG_S(D_WARNING, s); }
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
const OS_QueueHd stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
OS_Message* msg_p;
Size audio_buf_size = task_args_p->audio_buf_size / 2;
Status s = S_UNDEF;

	for(;;) {
        IF_STATUS(OS_MessageReceive(stdin_qhd, &msg_p, OS_BLOCK)) {
            //OS_LOG_S(D_WARNING, S_UNDEF_MSG);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_AUDIO_TX_COMPLETE:
                        {
                            void* audio_buf_p;
                            const OS_SignalData sig_data = OS_SignalDataGet(msg_p);
                            if (0 == sig_data) {
                                audio_buf_p = task_args_p->audio_buf_p;
                            } else if (1 == sig_data) {
                                audio_buf_p = ((U8*)task_args_p->audio_buf_p + audio_buf_size);
                            }

                            IF_OK(s = OS_FileRead(task_args_p->file_hd, audio_buf_p, audio_buf_size)) {
                                AudioDecode(audio_buf_p, audio_buf_size, task_args_p->audio_format);
                                VolumeApply(audio_buf_p, audio_buf_size, task_args_p->audio_info.sample_bits, OS_VolumeGet());
                            } else {
                                if ((S_FS_EOF == s) || (S_SIZE_MISMATCH == s)) {
                                    OS_LOG(D_DEBUG, "End of file");
                                    OS_TaskDelete(OS_THIS_TASK);
                                }
                            }
                        }
                        break;
                    case OS_SIG_AUDIO_ERROR:
                        OS_LOG_S(D_DEBUG, S_HARDWARE_FAULT);
                        break;
                    case OS_SIG_MMPLAY_PLAY:
                        if (MMPLAY_STATE_STOP == task_args_p->state) {
                            IF_STATUS(s = Play(task_args_p, task_args_p->audio_format)) {}
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_PAUSE:
                        if (MMPLAY_STATE_PLAY == task_args_p->state) {
                            IF_OK(s = OS_AudioPause(task_args_p->audio_dev_hd)) {
                                task_args_p->state = MMPLAY_STATE_PAUSE;
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_RESUME:
                        if (MMPLAY_STATE_PAUSE == task_args_p->state) {
                            IF_OK(s = OS_AudioResume(task_args_p->audio_dev_hd)) {
                                task_args_p->state = MMPLAY_STATE_PLAY;
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_STOP:
                        if ((MMPLAY_STATE_PLAY  == task_args_p->state) ||
                            (MMPLAY_STATE_PAUSE == task_args_p->state)) {
                            IF_OK(s = OS_AudioStop(task_args_p->audio_dev_hd)) {
                                IF_OK(s = OS_QueueClear(stdin_qhd)) {
                                    IF_OK(s = OS_FileLSeek(task_args_p->file_hd, 0)) {
                                        task_args_p->state = MMPLAY_STATE_STOP;
                                    }
                                }
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    default:
                        s = S_UNDEF_SIG;
                        break;
                }
                IF_STATUS(s) {
                    OS_LOG_S(D_WARNING, s);
                }
            } else {
                switch (msg_p->id) {
                    default:
                        OS_LOG_S(D_DEBUG, S_UNDEF_MSG);
                        break;
                }
                OS_MessageDelete(msg_p); // free message allocated memory
            }
        }
    }
}

/******************************************************************************/
Status OS_TaskPower(OS_TaskArgs* args_p, const OS_PowerState state)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
Status s = S_UNDEF;

    switch (state) {
        case PWR_STARTUP:
            s = S_OK;
            break;
        case PWR_OFF:
        case PWR_STOP:
            break;
        case PWR_SHUTDOWN:
            IF_OK(s = OS_AudioStop(task_args_p->audio_dev_hd)) {
                IF_OK(s = OS_QueueClear(OS_TaskStdInGet(OS_THIS_TASK))) {
                    IF_OK(s = OS_FileClose(&task_args_p->file_hd)) {
                        IF_OK(s = OS_AudioDeviceClose(task_args_p->audio_dev_hd)) {
                        }
                    }
                }
            }
            OS_FreeEx(task_args_p->audio_buf_p, AUDIO_BUF_MEMORY);
            break;
        case PWR_ON:
            IF_STATUS(s = OS_TaskInit(args_p)) {}
            break;
        default:
            s = S_OK;
            break;
    }
//error:
    IF_STATUS(s) { OS_LOG_S(D_WARNING, s); }
    return s;
}

/*****************************************************************************/
Status Play(TaskArgs* task_args_p, const FileAudioFormat format)
{
Size audio_header_size = 0;
Status s = S_UNDEF;

    if (FILE_AUDIO_FORMAT_WAV == format) {
        audio_header_size = sizeof(FileAudioFormatWav);
    } else {
        s = OS_FileClose(&task_args_p->file_hd);
        return s = S_INVALID_VALUE;
    }
    IF_OK(s = OS_FileLSeek(task_args_p->file_hd, audio_header_size)) {
        IF_OK(s = OS_FileRead(task_args_p->file_hd, task_args_p->audio_buf_p, task_args_p->audio_buf_size)) {
            VolumeApply(task_args_p->audio_buf_p, task_args_p->audio_buf_size, task_args_p->audio_info.sample_bits, OS_VolumeGet());
            IF_OK(s = OS_AudioPlay(task_args_p->audio_dev_hd, task_args_p->audio_buf_p, task_args_p->audio_buf_size)) {
                task_args_p->state = MMPLAY_STATE_PLAY;
            }
        }
    }
    return s;
}

/*****************************************************************************/
Status FileAudioFormatGet(ConstStrPtr file_path_str_p, FileAudioFormat* format_p)
{
#define FILE_BUF_Size       0x400
OS_FileHd file_hd;
void* file_buf_p = OS_Malloc(FILE_BUF_Size);
Status s = S_UNDEF;
    if (OS_NULL == format_p) { return s = S_INVALID_REF; }
    if (OS_NULL == file_buf_p) { return s = S_NO_MEMORY; }
    IF_OK(s = OS_FileOpen(&file_hd, file_path_str_p,
                          BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) | BIT(OS_FS_FILE_OP_MODE_READ))) {
        IF_OK(s = OS_FileRead(file_hd, file_buf_p, FILE_BUF_Size)) {
            IF_OK(s = OS_FileClose(&file_hd)) {
                *format_p = AudioFormatGet(file_buf_p);
            }
        }
    }
    OS_Free(file_buf_p);
    return s;
}

/*****************************************************************************/
FileAudioFormat AudioFormatGet(void* file_audio_data_p)
{
FileAudioFormat format = FILE_AUDIO_FORMAT_UNDEF;
    {
        const FileAudioFormatWav* wav_p = (FileAudioFormatWav*)file_audio_data_p;
        if ((0x46464952 == wav_p->chunk_id) && (1 == wav_p->audio_format)) {
            format = FILE_AUDIO_FORMAT_WAV;
        }
    }
    return format;
}

/*****************************************************************************/
Status FileAudioWavInfoGet(ConstStrPtr file_path_str_p, FileAudioFormatWav* info_p)
{
OS_FileHd file_hd;
Status s = S_UNDEF;
    if (OS_NULL == info_p) { return s = S_INVALID_REF; }
    IF_OK(s = OS_FileOpen(&file_hd, file_path_str_p,
                          BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) | BIT(OS_FS_FILE_OP_MODE_READ))) {
        IF_OK(s = OS_FileRead(file_hd, info_p, sizeof(FileAudioFormatWav))) {
            IF_OK(s = OS_FileClose(&file_hd)) {
                if (FILE_AUDIO_FORMAT_WAV != AudioFormatGet(info_p)) {
                    s = S_MMPLAY_FORMAT_MISMATCH;
                    OS_LOG_S(D_WARNING, s);
                }
            }
        }
    }
    return s;
}

/******************************************************************************/
void VolumeApply(void* data_out_p, Size size, const OS_AudioBits bit_rate, const OS_AudioVolume volume)
{
    OS_ASSERT_VALUE(OS_NULL != data_out_p);
    const Float volume_pct = (Float)volume / OS_AUDIO_VOLUME_MAX;
    if (16 == bit_rate) {
        S16* data_out_16p = data_out_p;
        size /= sizeof(S16);
        while (size--) {
            *data_out_16p *= volume_pct;
            ++data_out_16p;
        }
    } else if ((24 == bit_rate) ||
               (32 == bit_rate)) {
        S32* data_out_32p = (S32*)data_out_p;
        size /= sizeof(S32);
        while (size--) {
            *data_out_32p *= volume_pct;
            ++data_out_32p;
        }
    } else { OS_LOG_S(D_WARNING, S_INVALID_VALUE); }
}

/******************************************************************************/
void AudioDecode(void* data_out_p, Size size, const FileAudioFormat format)
{
    if (FILE_AUDIO_FORMAT_MP3 == format) {
        DecodeMp3(data_out_p, size);
    }
}

/******************************************************************************/
void DecodeMp3(void* data_out_p, Size size)
{
}