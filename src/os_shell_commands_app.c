/**************************************************************************//**
* @file    os_shell_commands_app.c
* @brief   OS shell application commands.
* @author  A. Filyanov
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "hal.h"
#include "osal.h"
#include "os_shell_commands_app.h"
#include "os_shell.h"

//------------------------------------------------------------------------------
static ConstStr cmd_app[]      = "net";
static ConstStr cmd_help_app[] = "Display the message.";
/******************************************************************************/
static Status OS_ShellCmdAppHandler(const U32 argc, ConstStrPtr argv[]);
Status OS_ShellCmdAppHandler(const U32 argc, ConstStrPtr argv[])
{
register U32 argc_count = 0;
    while (argc > argc_count) {
        printf("\n%s", argv[argc_count]);
        ++argc_count;
    }
    return S_OK;
}

/******************************************************************************/
Status OS_ShellCommandsAppInit(void)
{
ConstStr empty_str[] = "";
OS_ShellCommandConfig cmd_cfg;
    //Create and register network shell commands
    cmd_cfg.command     = cmd_app;
#ifdef OS_SHELL_HELP_ENABLED
    cmd_cfg.help_brief  = cmd_help_app;
    cmd_cfg.help_detail = empty_str;
#endif // OS_SHELL_HELP_ENABLED
    cmd_cfg.handler     = OS_ShellCmdAppHandler;
    cmd_cfg.options     = OS_SHELL_OPT_UNDEF;
    IF_STATUS(OS_ShellCommandCreate(&cmd_cfg)) {
        OS_ASSERT(OS_FALSE);
    }
    return S_OK;
}
