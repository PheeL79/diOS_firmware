/***************************************************************************//**
* @file    task_mmplay.c
* @brief   Multimedia player task.
* @author  A. Filyanov
*******************************************************************************/
#include "drv_audio.h"
#include "os_supervise.h"
#include "os_audio.h"
#include "os_environment.h"
#include "os_task_audio.h"
#include "app_common.h"
#include "task_mmplay.h"

#if (OS_AUDIO_ENABLED)
//-----------------------------------------------------------------------------
#define MDL_NAME                "task_mmplay"
#undef  MDL_STATUS_ITEMS
#define MDL_STATUS_ITEMS        &status_mmplay_v[0]

#define AUDIO_BUF_IN_MEMORY     OS_MEM_RAM_EXT_SRAM
#define AUDIO_BUF_OUT_MEMORY    OS_MEM_RAM_EXT_SRAM
#define AUDIO_DMA_SIZE_MAX      U16_MAX

//------------------------------------------------------------------------------
enum {
    S_MMPLAY_UNDEF = S_AUDIO_CODEC_LAST,
    S_MMPLAY_FORMAT_UNSUPPORTED,
    S_MMPLAY_LAST
};

const StatusItem status_mmplay_v[] = {
    {"Undefined status"},
    {"Unsupported format"},
};

typedef enum {
    MMPLAY_STATE_UNDEF,
    MMPLAY_STATE_PLAY,
    MMPLAY_STATE_PAUSE,
    MMPLAY_STATE_STOP,
    MMPLAY_STATE_LAST
} MMPlayState;

//Task arguments
typedef struct {
    OS_FileHd           file_hd;
    OS_QueueHd          stdin_qhd;
    OS_AudioDeviceHd    audio_dev_hd;
    OS_AudioDmaMode     audio_dev_dma_mode;
    AudioCodecHd        audio_codec_hd;
    U8*                 audio_buf_in_p;
    Size                audio_buf_in_size;
    U8*                 audio_buf_out_p;
    Size                audio_buf_out_size;
    Int                 audio_buf_out_size_curr;
    Bool                audio_buf_idx;
    AudioFormatInfo     audio_format_info;
    AudioFrameInfo      audio_frame_info;
    MMPlayState         state;
} TaskStorage;

//------------------------------------------------------------------------------
static Status   Play(TaskStorage* tstor_p);
//static void AudioBitRateConvert(U8* data_in_p, U8* data_out_p, Size size,
//                                const OS_AudioBits bit_rate_in, const OS_AudioBits bit_rate_out);
static void     VolumeApply(U8* data_out_p, Size size, const OS_AudioBits bit_rate, const OS_AudioVolume volume);
static Status   FrameReadDecode(TaskStorage* tstor_p, U8* audio_buf_out_p);
static void     ISR_DrvAudioDeviceCallback(OS_AudioDeviceCallbackArgs* args_p);

//------------------------------------------------------------------------------
ConstStrP mmplay_file_path_str_p;

//------------------------------------------------------------------------------
OS_TaskConfig task_mmplay_cfg = {
    .name           = APP_TASK_NAME_MMPLAY,
    .func_main      = OS_TaskMain,
    .func_power     = OS_TaskPower,
    .args_p         = OS_NULL,
    .attrs          = BIT(OS_TASK_ATTR_SINGLE),
    .timeout        = 4,
    .prio_init      = APP_PRIO_TASK_MMPLAY,
    .prio_power     = APP_PRIO_PWR_TASK_MMPLAY,
    .storage_size   = sizeof(TaskStorage),
    .stack_size     = OS_STACK_SIZE_MIN,
    .stdin_len      = OS_STDIN_LEN
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
ConstStrP file_path_str_p = args_p->args_p;
AudioFormatInfo* audio_format_info_p = &(tstor_p->audio_format_info);
Status s = S_UNDEF;

    tstor_p->state = MMPLAY_STATE_UNDEF;
    //Check file format.
    IF_OK(s = AudioFileFormatInfoGet(file_path_str_p, audio_format_info_p)) {
        if (AUDIO_FORMAT_UNDEF != audio_format_info_p->format) {
            if (AUDIO_FORMAT_MP3 == audio_format_info_p->format) {
                tstor_p->audio_buf_out_size = 0x2400;
                tstor_p->audio_buf_in_size  = tstor_p->audio_buf_out_size;
                tstor_p->audio_dev_dma_mode = OS_AUDIO_DMA_MODE_CIRCULAR; //OS_AUDIO_DMA_MODE_NORMAL;
            } else if (AUDIO_FORMAT_WAV == audio_format_info_p->format) {
                tstor_p->audio_buf_out_size = 0x2000;
                tstor_p->audio_buf_in_size  = (tstor_p->audio_buf_out_size / 2);
                tstor_p->audio_dev_dma_mode = OS_AUDIO_DMA_MODE_CIRCULAR;
            } else { return s = S_MMPLAY_FORMAT_UNSUPPORTED; }
            tstor_p->audio_codec_hd  = AudioCodecGet(audio_format_info_p->format);
            tstor_p->audio_dev_hd    = OS_AudioDeviceDefaultGet(DIR_OUT);
            if ((OS_NULL != tstor_p->audio_dev_hd) &&
                (OS_NULL != tstor_p->audio_codec_hd)) {
                const OS_AudioDeviceIoSetupArgs io_args = {
                    .info       = tstor_p->audio_format_info.audio_info,
                    .dma_mode   = tstor_p->audio_dev_dma_mode,
                    .volume     = OS_VolumeGet(),
                };
                IF_OK(s = OS_AudioDeviceIoSetup(tstor_p->audio_dev_hd, &io_args, DIR_OUT)) {
                    //Allocate audio stream buffers.
                    tstor_p->audio_buf_in_p = OS_MallocEx(tstor_p->audio_buf_in_size,  AUDIO_BUF_IN_MEMORY);
                    tstor_p->audio_buf_out_p= OS_MallocEx(tstor_p->audio_buf_out_size, AUDIO_BUF_OUT_MEMORY);
                    if ((OS_NULL == tstor_p->audio_buf_in_p) ||
                        (OS_NULL == tstor_p->audio_buf_out_p)) { return s = S_OUT_OF_MEMORY; }
                    IF_OK(s = OS_FileOpen(&tstor_p->file_hd, file_path_str_p,
                                          BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) | BIT(OS_FS_FILE_OP_MODE_READ))) {
                        const OS_AudioDeviceArgsOpen audio_dev_open_args = {
                            .slot_qhd           = OS_TaskStdInGet(OS_THIS_TASK),
                            .isr_callback_func  = ISR_DrvAudioDeviceCallback
                        };
                        IF_OK(s = OS_AudioDeviceOpen(tstor_p->audio_dev_hd, (void*)&audio_dev_open_args)) {
                            IF_OK(s = AudioCodecOpen(tstor_p->audio_codec_hd, OS_NULL)) {
                                tstor_p->audio_frame_info.buf_in_offset = 0;
                                tstor_p->audio_frame_info.buf_out_size  = 0;
                                tstor_p->audio_buf_out_size /= 2; //Double buffer.
                                tstor_p->audio_buf_idx       = 0; //First one.
                                tstor_p->state = MMPLAY_STATE_STOP;
                                const OS_Signal signal = OS_SignalCreate(OS_SIG_MMPLAY_PLAY, 0);
                                IF_OK(s = OS_SignalSend(audio_dev_open_args.slot_qhd, signal, OS_MSG_PRIO_NORMAL)) {
                                }
                                IF_STATUS(s) {
                                    IF_STATUS(s = AudioCodecClose(tstor_p->audio_codec_hd, OS_NULL)) {
                                    }
                                }
                            }
                            IF_STATUS(s) {
                                IF_OK(s = OS_AudioDeviceClose(tstor_p->audio_dev_hd)) {}
                            }
                        }
                        IF_STATUS(s) {
                            IF_OK(s = OS_FileClose(&tstor_p->file_hd)) {}
                        }
                    }
                }
                IF_STATUS(s) {
                    OS_FreeEx(tstor_p->audio_buf_in_p,  AUDIO_BUF_IN_MEMORY);
                    OS_FreeEx(tstor_p->audio_buf_out_p, AUDIO_BUF_OUT_MEMORY);
                }
            } else { s = S_INVALID_PTR; }
        } else { s = S_MMPLAY_FORMAT_UNSUPPORTED; }
    }
    IF_STATUS(s) { OS_LOG_S(L_WARNING, s); }
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
OS_Message* msg_p;
U8* decode_audio_buf_out_p;
U8* play_audio_buf_out_p;
Status s = S_UNDEF;

    tstor_p->stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
	for(;;) {
        IF_STATUS(OS_MessageReceive(tstor_p->stdin_qhd, &msg_p, OS_BLOCK)) {
            //OS_LOG_S(L_WARNING, S_UNDEF_MSG);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_AUDIO_TX_COMPLETE:
#if (OS_DEBUG_ENABLED)
{
    HAL_GPIO_DEBUG_1_TOGGLE();
}
#endif // (OS_DEBUG_ENABLED)
                        decode_audio_buf_out_p  = tstor_p->audio_buf_out_p;
                        play_audio_buf_out_p    = decode_audio_buf_out_p;
                        if (tstor_p->audio_buf_idx) {
                            decode_audio_buf_out_p  += tstor_p->audio_buf_out_size;
                        } else {
                            play_audio_buf_out_p    += tstor_p->audio_buf_out_size;
                        }
                        if (OS_AUDIO_DMA_MODE_NORMAL == tstor_p->audio_dev_dma_mode) {
                            IF_OK(s = OS_AudioPlay(tstor_p->audio_dev_hd, play_audio_buf_out_p, tstor_p->audio_buf_out_size_curr)) {
                                s = FrameReadDecode(tstor_p, decode_audio_buf_out_p);
                                VolumeApply(decode_audio_buf_out_p, tstor_p->audio_buf_out_size_curr,
                                            tstor_p->audio_format_info.audio_info.sample_bits, OS_VolumeGet());
                            }
                        } else if (OS_AUDIO_DMA_MODE_CIRCULAR == tstor_p->audio_dev_dma_mode) {
                            s = FrameReadDecode(tstor_p, decode_audio_buf_out_p);
                            VolumeApply(decode_audio_buf_out_p, tstor_p->audio_buf_out_size_curr,
                                        tstor_p->audio_format_info.audio_info.sample_bits, OS_VolumeGet());
                        } else {
                            OS_LOG_S(L_CRITICAL, S_HARDWARE_ERROR);
                            OS_TaskDelete(OS_THIS_TASK);
                        }
                        tstor_p->audio_buf_idx ^= 1; // Switch output buffer.
#if (OS_DEBUG_ENABLED)
{
    HAL_GPIO_DEBUG_1_TOGGLE();
}
#endif // (OS_DEBUG_ENABLED)
                        break;
                    case OS_SIG_AUDIO_TX_COMPLETE_HALF:
                        break;
                    case OS_SIG_AUDIO_ERROR:
                        OS_LOG_S(L_DEBUG_1, S_HARDWARE_ERROR);
                        break;
                    case OS_SIG_MMPLAY_PLAY:
                        if (MMPLAY_STATE_STOP == tstor_p->state) {
                            IF_OK(s = Play(tstor_p)) {
                                tstor_p->state = MMPLAY_STATE_PLAY;
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_PAUSE:
                        if (MMPLAY_STATE_PLAY == tstor_p->state) {
                            IF_OK(s = OS_AudioPause(tstor_p->audio_dev_hd)) {
                                tstor_p->state = MMPLAY_STATE_PAUSE;
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_RESUME:
                        if (MMPLAY_STATE_PAUSE == tstor_p->state) {
                            IF_OK(s = OS_AudioResume(tstor_p->audio_dev_hd)) {
                                tstor_p->state = MMPLAY_STATE_PLAY;
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    case OS_SIG_MMPLAY_STOP:
                        if ((MMPLAY_STATE_PLAY  == tstor_p->state) ||
                            (MMPLAY_STATE_PAUSE == tstor_p->state)) {
                            IF_OK(s = OS_AudioStop(tstor_p->audio_dev_hd)) {
                                IF_OK(s = OS_QueueClear(tstor_p->stdin_qhd)) {
                                    IF_OK(s = OS_FileLSeek(tstor_p->file_hd, 0)) {
                                        tstor_p->state = MMPLAY_STATE_STOP;
                                    }
                                }
                            }
                        } else { s = S_INVALID_STATE; }
                        break;
                    default:
                        s = S_INVALID_SIGNAL;
                        break;
                }
                IF_STATUS(s) {
                    OS_LOG_S(L_WARNING, s);
                }
            } else {
                switch (msg_p->id) {
                    default:
                        OS_LOG_S(L_DEBUG_1, S_INVALID_MESSAGE);
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
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
Status s = S_UNDEF;

    switch (state) {
        case PWR_STARTUP:
            s = S_OK;
            break;
        case PWR_OFF:
        case PWR_STOP:
            break;
        case PWR_SHUTDOWN:
            IF_OK(s = OS_AudioStop(tstor_p->audio_dev_hd)) {
                IF_OK(s = AudioCodecClose(tstor_p->audio_codec_hd, OS_NULL)) {
                    IF_OK(s = OS_QueueClear(OS_TaskStdInGet(OS_THIS_TASK))) {
                        IF_OK(s = OS_FileClose(&tstor_p->file_hd)) {
                            IF_OK(s = OS_AudioDeviceClose(tstor_p->audio_dev_hd)) {
                            }
                        }
                    }
                }
            }
            OS_FreeEx(tstor_p->audio_buf_in_p,  AUDIO_BUF_IN_MEMORY);
            OS_FreeEx(tstor_p->audio_buf_out_p, AUDIO_BUF_OUT_MEMORY);
            break;
        case PWR_ON:
            IF_STATUS(s = OS_TaskInit(args_p)) {}
            break;
        default:
            s = S_OK;
            break;
    }
//error:
    IF_STATUS(s) { OS_LOG_S(L_WARNING, s); }
    return s;
}

/*****************************************************************************/
Status Play(TaskStorage* tstor_p)
{
Status s = S_UNDEF;

    IF_OK(s = OS_FileLSeek(tstor_p->file_hd, tstor_p->audio_format_info.header_size)) {
        IF_OK(s = FrameReadDecode(tstor_p, tstor_p->audio_buf_out_p)) {
            VolumeApply(tstor_p->audio_buf_out_p, tstor_p->audio_buf_out_size_curr,
                        tstor_p->audio_format_info.audio_info.sample_bits, OS_VolumeGet());
            IF_OK(s = OS_AudioPlay(tstor_p->audio_dev_hd,
                                   tstor_p->audio_buf_out_p, tstor_p->audio_buf_out_size_curr)) {
                IF_OK(s = FrameReadDecode(tstor_p, (tstor_p->audio_buf_out_p + tstor_p->audio_buf_out_size))) {
                    VolumeApply((tstor_p->audio_buf_out_p + tstor_p->audio_buf_out_size), tstor_p->audio_buf_out_size_curr,
                                tstor_p->audio_format_info.audio_info.sample_bits, OS_VolumeGet());
                }
            }
        }
    }
    return s;
}

/******************************************************************************/
void VolumeApply(U8* data_out_p, Size size, const OS_AudioBits bit_rate, const OS_AudioVolume volume)
{
    OS_ASSERT_DEBUG(OS_NULL != data_out_p);
    const Float volume_pct = (Float)volume / OS_AUDIO_VOLUME_MAX;
    if (16 == bit_rate) {
        S16* data_out_16p = (S16*)data_out_p;
        size /= sizeof(S16);
        while (size--) {
            *data_out_16p++ *= volume_pct;
        }
    } else if ((24 == bit_rate) ||
               (32 == bit_rate)) {
        S32* data_out_32p = (S32*)data_out_p;
        size /= sizeof(S32);
        while (size--) {
            *data_out_32p++ *= volume_pct;
        }
    } else { OS_LOG_S(L_WARNING, S_INVALID_VALUE); }
}

/******************************************************************************/
Status FrameReadDecode(TaskStorage* tstor_p, U8* audio_buf_out_p)
{
Int audio_buf_out_size = tstor_p->audio_buf_out_size;
Status s = S_UNDEF;

    while ((0 < audio_buf_out_size) && (s != S_AUDIO_CODEC_OUTPUT_BUFFER_FULL)) {
        IF_OK(s = OS_FileRead(tstor_p->file_hd,
                              tstor_p->audio_buf_in_p    + tstor_p->audio_frame_info.buf_in_offset,
                              tstor_p->audio_buf_in_size - tstor_p->audio_frame_info.buf_in_offset)) {
            IF_OK(s = AudioCodecDecode(tstor_p->audio_codec_hd,
                                       tstor_p->audio_buf_in_p, tstor_p->audio_buf_in_size,
                                       audio_buf_out_p, audio_buf_out_size,
                                       &tstor_p->audio_frame_info)) {
            }
            audio_buf_out_p     += tstor_p->audio_frame_info.buf_out_size;
            audio_buf_out_size  -= tstor_p->audio_frame_info.buf_out_size;
        } else {
            if ((S_FS_EOF == s) || (S_INVALID_SIZE == s)) {
                OS_LOG(L_DEBUG_1, "End of file");
                OS_TaskDelete(OS_THIS_TASK);
            }
        }
    }
    if (audio_buf_out_size) {
        //Void remaining output audio buffer space.
        OS_MemSet(audio_buf_out_p, 0, audio_buf_out_size);
    }
    tstor_p->audio_buf_out_size_curr = (tstor_p->audio_buf_out_size - audio_buf_out_size);
    s = S_OK; //Status force clear!
    return s;
}

/******************************************************************************/
void ISR_DrvAudioDeviceCallback(OS_AudioDeviceCallbackArgs* args_p)
{
const OS_Signal signal = OS_ISR_SignalCreate(OS_SIG_DRV, args_p->signal_id, 0);
    if (1 == OS_ISR_SignalSend(args_p->slot_qhd, signal, OS_MSG_PRIO_NORMAL)) {
        OS_ContextSwitchForce();
    }
}

#endif //(OS_AUDIO_ENABLED)