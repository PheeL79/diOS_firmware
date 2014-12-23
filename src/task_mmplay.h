/***************************************************************************//**
* @file    task_mmplay.h
* @brief   Multimedia player task.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _TASK_MMPLAY_H_
#define _TASK_MMPLAY_H_

#include "os_file_system.h"
#include "os_audio.h"
#include "audio_codec.h"

//-----------------------------------------------------------------------------
#define APP_TASK_NAME_MMPLAY    "MMPlay"

enum {
    OS_SIG_MMPLAY_UNDEF = OS_SIG_AUDIO_LAST,
    OS_SIG_MMPLAY_PLAY,
    OS_SIG_MMPLAY_PAUSE,
    OS_SIG_MMPLAY_RESUME,
    OS_SIG_MMPLAY_STOP,
    OS_SIG_MMPLAY_SEEK,
    OS_SIG_MMPLAY_LAST
};

extern OS_TaskConfig task_mmplay_cfg;
extern ConstStrP mmplay_file_path_str_p;

#endif // _TASK_MMPLAY_H_