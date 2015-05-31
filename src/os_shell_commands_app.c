/**************************************************************************//**
* @file    os_shell_commands_app.c
* @brief   OS shell application commands.
* @author  A. Filyanov
******************************************************************************/
#include "osal.h"
#include "os_shell_commands_app.h"
#include "os_shell.h"
#include "task_mmplay.h"

//-----------------------------------------------------------------------------
static OS_TaskHd mmplay_thd;

#if (OS_AUDIO_ENABLED)
//------------------------------------------------------------------------------
static ConstStr cmd_mmplay[]            = "mmplay";
static ConstStr cmd_help_brief_mmplay[] = "Play a multimedia file.";
/******************************************************************************/
static Status OS_ShellCmdMMPlayHandler(const U32 argc, ConstStrP argv[]);
Status OS_ShellCmdMMPlayHandler(const U32 argc, ConstStrP argv[])
{
static OS_QueueHd mmplay_stdin_qhd;
const char* file_path_str_p = (char*)argv[0];
OS_SignalId signal_id = OS_SIG_UNDEF;
Status s = S_UNDEF;
    if (0 == OS_StrCmp("play", file_path_str_p)) {
        signal_id = OS_SIG_MMPLAY_PLAY;
    } else if (!OS_StrCmp("pause", file_path_str_p)) {
        signal_id = OS_SIG_MMPLAY_PAUSE;
    } else if (!OS_StrCmp("resume",file_path_str_p)) {
        signal_id = OS_SIG_MMPLAY_RESUME;
    } else if (!OS_StrCmp("stop", file_path_str_p)) {
        signal_id = OS_SIG_MMPLAY_STOP;
    } else if (!OS_StrCmp("seek", file_path_str_p)) {
        signal_id = OS_SIG_MMPLAY_SEEK;
    } else {
        IF_OK(s = OS_TaskCreate(file_path_str_p, &task_mmplay_cfg, &mmplay_thd)) {
            mmplay_stdin_qhd = OS_TaskStdInGet(mmplay_thd);
            if (OS_NULL == mmplay_stdin_qhd) {
                OS_TaskDelete(mmplay_thd);
                s = S_INVALID_QUEUE;
            }
        }
    }
    if (OS_SIG_UNDEF != signal_id) {
        const OS_Signal signal = OS_SignalCreate(signal_id, 0);
        IF_STATUS(s = OS_SignalSend(mmplay_stdin_qhd, signal, OS_MSG_PRIO_NORMAL)) {}
    }
    return s;
}
#endif //(OS_AUDIO_ENABLED)

//------------------------------------------------------------------------------
static ConstStr empty_str[] = "";
static const OS_ShellCommandConfig cmd_cfg_app[] = {
#if (OS_AUDIO_ENABLED)
    { cmd_mmplay,   cmd_help_brief_mmplay,  empty_str,              OS_ShellCmdMMPlayHandler,       1,    2,      OS_SHELL_OPT_UNDEF  },
#endif //(OS_AUDIO_ENABLED)
    OS_NULL
};

/******************************************************************************/
Status OS_ShellCommandsAppInit(void)
{
    //Create and register file system shell commands
    for (Size i = 0; i < ITEMS_COUNT_GET(cmd_cfg_app, OS_ShellCommandConfig); ++i) {
        const OS_ShellCommandConfig* cmd_cfg_p = &cmd_cfg_app[i];
        if (OS_NULL != cmd_cfg_p->command) {
            IF_STATUS(OS_ShellCommandCreate(cmd_cfg_p)) {
                OS_ASSERT(OS_FALSE);
            }
        }
    }
    return S_OK;
}
