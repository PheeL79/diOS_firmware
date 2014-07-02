/***************************************************************************//**
* @file    task_b_ko.c
* @brief   B-ko task.
* @author  A. Filyanov
*******************************************************************************/
#ifndef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal.h"
#include "osal.h"
#include "os_message.h"
#include "os_debug.h"
#include "os_timer.h"
#include "os_event.h"
#include "os_driver.h"
#include "os_memory.h"
#include "os_signal.h"
#include "os_settings.h"
#include "os_environment.h"
#include "task_b_ko.h"

//-----------------------------------------------------------------------------
//Task arguments
typedef struct {
    OS_QueueHd  stdin_qhd;
    OS_DriverHd drv_led_user;
    TimeMs      blink_rate;
} TaskArgs;

static TaskArgs task_args = {
    .stdin_qhd      = OS_NULL,
    .drv_led_user   = OS_NULL,
    .blink_rate     = OS_BLOCK
};

//------------------------------------------------------------------------------
const OS_TaskConfig task_b_ko_cfg = {
    .name       = "B-ko",
    .func_main  = OS_TaskMain,
    .func_power = OS_TaskPower,
    .args_p     = (void*)&task_args,
    .attrs      = BIT(OS_TASK_ATTR_RECREATE),
    .timeout    = 3,
    .prio_init  = OS_TASK_PRIO_BELOW_NORMAL,
    .prio_power = OS_PWR_PRIO_DEFAULT + 3,
    .stack_size = OS_STACK_SIZE_MIN * 2,
    .stdin_len  = OS_STDIO_LEN,
    .stdout_len = OS_STDIO_LEN
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
Status s;
    const OS_DriverConfig drv_cfg = {
        .name       = "LED_USR",
        .itf_p      = drv_led_v[DRV_ID_LED_USER],
        .prio_power = OS_PWR_PRIO_DEFAULT
    };
    IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&task_args_p->drv_led_user)) { return s; }
    return s;
}

const S8 volume = 1;
/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
State led_state = OFF;
ConstStr* task_name_server = "A-ko";
const OS_QueueHd stdio_in_qhd = OS_TaskStdIoGet(OS_THIS_TASK, OS_STDIO_IN);
OS_TaskHd  a_ko_thd = OS_TaskByNameGet(task_name_server);
OS_QueueHd a_ko_qhd = OS_TaskStdIoGet(a_ko_thd, OS_STDIO_IN);
OS_Message* msg_p;// = OS_MessageCreate(OS_MSG_APP, sizeof(task_args), OS_BLOCK, &task_args);
volatile int debug = 0;
const OS_Signal signal = OS_SIGNAL_CREATE(OS_SIG_APP, 0);

//    OS_SIGNAL_EMIT(a_ko_qhd, signal, OS_MSG_PRIO_HIGH);
//    OS_MessageSend(a_ko_qhd, msg_p, 100, OS_MSG_PRIO_NORMAL);
    {
    static ConstStrPtr hello_msg_p = "Hello, world!";
    Status s;
    OS_EventItem* ev_item_p;
    IF_STATUS(s = OS_EventItemCreate(hello_msg_p, strlen(hello_msg_p), &ev_item_p)) {
        OS_LOG_S(D_CRITICAL, s);
    }
    static ConstStrPtr tim_name_p = "EventT";
    const OS_TimerConfig tim_cfg = {
        .name_p = tim_name_p,
        .slot   = a_ko_qhd,
        .id     = 18,
        .period = 10000,
        .options= (OS_TimerOptions)(BIT(OS_TIM_OPT_PERIODIC) | BIT(OS_TIM_OPT_EVENT))
    };
    const OS_EventConfig cfg = {
        .timer_cfg_p= &tim_cfg,
        .item_p     = ev_item_p,
        .state      = OS_EVENT_STATE_UNDEF
    };
    OS_EventHd ehd;
        IF_STATUS(s = OS_EventCreate(&cfg, &ehd)) {
            OS_LOG_S(D_WARNING, s);
        }
    }

U8 debug_count = 6;
	for(;;) {
        IF_STATUS(OS_MessageReceive(stdio_in_qhd, &msg_p, task_args_p->blink_rate)) {
            //OS_LOG_S(D_WARNING, S_UNDEF_MSG);
        } else {
            if (OS_SIGNAL_IS(msg_p)) {
                switch (OS_SIGNAL_ID_GET(msg_p)) {
                    case OS_SIG_APP:
                        debug = OS_SIGNAL_DATA_GET(msg_p);
                        break;
                    default:
                        OS_LOG_S(D_DEBUG, S_UNDEF_SIG);
                        break;
                }
            } else {
                switch (msg_p->id) {
                    case OS_MSG_APP:
                        debug = 2;
                        break;
                    default:
                        OS_LOG_S(D_DEBUG, S_UNDEF_MSG);
                        break;
                }
                OS_MessageDelete(msg_p); // free message allocated memory
            }
        }
        led_state = (ON == led_state) ? OFF : ON;
        OS_DriverWrite(task_args_p->drv_led_user, (void*)&led_state, 1, OS_NULL);
//        if (!--debug_count) {
//            while(1) {};
//        }
    }
}

/******************************************************************************/
Status OS_TaskPower(OS_TaskArgs* args_p, const OS_PowerState state)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
Status s = S_OK;


//    if (PWR_STOP == state) {
//        return S_APP_MODULE; //DEBUG!!!
//    }


    switch (state) {
        case PWR_STARTUP:
            IF_STATUS(s = OS_TaskInit(args_p)) {
            }
            break;
        case PWR_OFF:
        case PWR_STOP:
        case PWR_SHUTDOWN: {
            IF_STATUS(s = OS_FileSystemMediaDeInit(volume)) {
                OS_LOG_S(D_WARNING, s);
            }
            IF_STATUS_OK(s = OS_DriverClose(task_args.drv_led_user)) {
                IF_STATUS(s = OS_DriverDeInit(task_args.drv_led_user)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            }
            break;
        case PWR_ON: {
            IF_STATUS_OK(s = OS_DriverInit(task_args.drv_led_user)) {
                IF_STATUS(s = OS_DriverOpen(task_args.drv_led_user, OS_NULL)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            IF_STATUS_OK(OS_FileSystemMediaInit(volume)) {
                IF_STATUS_OK(OS_FileSystemMount(volume)) {
                    ConstStrPtr config_path_p = OS_EnvVariableGet("config_file");
                    Str value[OS_SETTINGS_VALUE_LEN];
                    OS_SettingsRead(config_path_p, "Second", "blink_rate", &value[0]);
                    task_args_p->blink_rate = (TimeMs)strtoul((const char*)&value[0], OS_NULL, 10);
        //            OS_SettingsRead(config_path_p, "First", "Val", &value[0]);
                    //OS_SettingsDelete(config_path_p, "Second", OS_NULL);
                    //OS_SettingsDelete(config_path_p, "Second", "String");
                    //OS_SettingsRead(config_path_p, "Second", "String", &value[0]);
                    //OS_SettingsWrite(config_path_p, "Third", "Test", "String test of settings");
                    // File write test
        //            {
        //                OS_FileHd test_file;
        //                OS_FileOpen(&test_file, "1:/test_write.bin",
        //                            (OS_FileOpenMode)(BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) |\
        //                                              BIT(OS_FS_FILE_OP_MODE_OPEN_NEW) |\
        //                                              BIT(OS_FS_FILE_OP_MODE_WRITE)));
        //                OS_FileWrite(test_file, (U8*)MEM_EXT_SRAM_BASE_ADDRESS, MEM_EXT_SRAM_SIZE);
        //                OS_FileClose(&test_file);
        //            }
                }
            }
            }
            break;
        default:
            break;
    }
    return s;
}

#endif // NDEBUG