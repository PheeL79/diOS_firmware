/***************************************************************************//**
* @file    task_mmplay.h
* @brief   Multimedia player task.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _TASK_MMPLAY_H_
#define _TASK_MMPLAY_H_

#include "os_file_system.h"
#include "os_audio.h"

//-----------------------------------------------------------------------------
#define OS_TASK_NAME_MMPLAY     "MMPlay"

typedef enum {
    MMPLAY_STATE_UNDEF,
    MMPLAY_STATE_PLAY,
    MMPLAY_STATE_PAUSE,
    MMPLAY_STATE_STOP,
    MMPLAY_STATE_LAST
} MMPlayState;

enum {
    S_MMPLAY_UNDEF = S_MODULE,
    S_MMPLAY_FORMAT_UNSUPPORTED,
    S_MMPLAY_FORMAT_MISMATCH
};

enum {
    OS_SIG_MMPLAY_UNDEF = OS_SIG_AUDIO_LAST,
    OS_SIG_MMPLAY_PLAY,
    OS_SIG_MMPLAY_PAUSE,
    OS_SIG_MMPLAY_RESUME,
    OS_SIG_MMPLAY_STOP,
    OS_SIG_MMPLAY_SEEK,
    OS_SIG_MMPLAY_LAST
};

typedef enum {
    FILE_AUDIO_FORMAT_UNDEF,
    FILE_AUDIO_FORMAT_WAV,
    FILE_AUDIO_FORMAT_MP3,
    FILE_AUDIO_FORMAT_LAST
} FileAudioFormat;

//Task arguments
typedef struct {
    ConstStrPtr         file_path_str_p;
    OS_FileHd           file_hd;
    OS_AudioDeviceHd    audio_dev_hd;
    void*               audio_buf_p;
    Size                audio_buf_size;
    FileAudioFormat     audio_format;
    OS_AudioInfo        audio_info;
    MMPlayState         state;
} TaskArgs;

extern OS_TaskConfig task_mmplay_cfg;
extern TaskArgs task_mmplay_args;

#endif // _TASK_MMPLAY_H_