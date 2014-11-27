/***************************************************************************//**
* @file    task_b_ko.c
* @brief   B-ko task.
* @author  A. Filyanov
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drv_usb.h"
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
    OS_DriverHd             drv_led_user;
    OS_EventHd              ehd;
    OS_QueueHd              a_ko_qhd;
    TimeMs                  blink_rate;
} TaskArgs;

static TaskArgs task_args = {
    .drv_led_user   = OS_NULL,
    .ehd            = OS_NULL,
    .a_ko_qhd       = OS_NULL,
    .blink_rate     = OS_BLOCK,
};

//------------------------------------------------------------------------------
const OS_TaskConfig task_b_ko_cfg = {
    .name       = "B-ko",
    .func_main  = OS_TaskMain,
    .func_power = OS_TaskPower,
    .args_p     = (void*)&task_args,
    .attrs      = BIT(OS_TASK_ATTR_RECREATE),
    .timeout    = 4,
    .prio_init  = OS_TASK_PRIO_NORMAL,
    .prio_power = OS_PWR_PRIO_DEFAULT + 3,
    .stack_size = OS_STACK_SIZE_MIN,
    .stdin_len  = OS_STDIN_LEN
};

//------------------------------------------------------------------------------
static Status       EventCreate(OS_QueueHd a_ko_qhd, OS_EventHd* ehd_p);

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
Status s;
    {
        const OS_DriverConfig drv_cfg = {
            .name       = "LED_USR",
            .itf_p      = drv_led_v[DRV_ID_LED_USER],
            .prio_power = OS_PWR_PRIO_DEFAULT
        };
        IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&task_args_p->drv_led_user)) { return s; }
    }
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
State led_state = OFF;
const OS_QueueHd stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
OS_Message* msg_p;// = OS_MessageCreate(OS_MSG_APP, sizeof(task_args), OS_BLOCK, &task_args);
int debug = 0;
const OS_Signal signal = OS_SignalCreate(OS_SIG_APP, 0);

//    OS_SIGNAL_EMIT(a_ko_qhd, signal, OS_MSG_PRIO_HIGH);
//    OS_MessageSend(a_ko_qhd, msg_p, 100, OS_MSG_PRIO_NORMAL);

U8 debug_count = (rand() % 6) + 1;
task_args_p->blink_rate = 1;
	for(;;) {
        IF_STATUS(OS_MessageReceive(stdin_qhd, &msg_p, task_args_p->blink_rate)) {
            //OS_LOG_S(D_WARNING, S_UNDEF_MSG);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_APP:
                        debug = OS_SignalDataGet(msg_p);
                        break;
                    default:
                        OS_LOG_S(D_DEBUG, S_UNDEF_SIG);
                        break;
                }
            } else {
                switch (msg_p->id) {
                    case OS_MSG_USB_HID_MOUSE: {
                        const OS_UsbHidMouseData* mouse_p = (OS_UsbHidMouseData*)&(msg_p->data);
                        const U8 l = BIT_TEST(mouse_p->buttons_bm, BIT(OS_USB_HID_MOUSE_BUTTON_LEFT));
                        const U8 r = BIT_TEST(mouse_p->buttons_bm, BIT(OS_USB_HID_MOUSE_BUTTON_RIGHT));
                        const U8 m = BIT_TEST(mouse_p->buttons_bm, BIT(OS_USB_HID_MOUSE_BUTTON_MIDDLE));
                        OS_LOG(D_DEBUG, "X:%d, Y:%d, L:%d, M:%d, R:%d",
                                        mouse_p->x, mouse_p->y, l, m, r);
                        }
                        break;
//                    case OS_MSG_APP:
//                        debug = 2;
//                        break;
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
            IF_OK(s = OS_DriverInit(task_args.drv_led_user, OS_NULL)) {
                IF_STATUS(s = OS_DriverOpen(task_args.drv_led_user, OS_NULL)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            break;
        case PWR_OFF:
        case PWR_STOP:
        case PWR_SHUTDOWN: {
//            IF_STATUS(s = OS_EventDelete(task_args_p->ehd, OS_TIMEOUT_DEFAULT)) {
//                OS_LOG_S(D_WARNING, s);
//            }
//            IF_OK(s = OS_DriverClose(task_args.drv_led_user)) {
//                IF_STATUS(s = OS_DriverDeInit(task_args.drv_led_user)) {
//                }
//            } else {
//                s = (S_INIT == s) ? S_OK : s;
//            }
            }
            break;
        case PWR_ON: {
            ConstStrPtr config_path_p = OS_EnvVariableGet("config_file");
            Str value[OS_SETTINGS_VALUE_LEN];
            OS_SettingsRead(config_path_p, "Second", "blink_rate", &value[0]);
            task_args_p->blink_rate = (TimeMs)OS_StrToUL((const char*)&value[0], OS_NULL, 10);

//                    OS_SettingsRead(config_path_p, "First", "Val", &value[0]);
//                    OS_SettingsDelete(config_path_p, "Second", OS_NULL);
//                    OS_SettingsDelete(config_path_p, "Second", "String");
//                    OS_SettingsRead(config_path_p, "Second", "String", &value[0]);
//                    OS_SettingsWrite(config_path_p, "Third", "Test", "String test of settings");
//                     File write test
//                    {
//                        OS_FileHd test_file;
//                        OS_FileOpen(&test_file, "1:/test_write.bin",
//                                    (OS_FileOpenMode)(BIT(OS_FS_FILE_OP_MODE_OPEN_EXISTS) |\
//                                                      BIT(OS_FS_FILE_OP_MODE_OPEN_NEW) |\
//                                                      BIT(OS_FS_FILE_OP_MODE_WRITE)));
//                        OS_FileWrite(test_file, (U8*)MEM_EXT_SRAM_BASE_ADDRESS, MEM_EXT_SRAM_SIZE);
//                        OS_FileClose(&test_file);
//                    }
//            ConstStr* task_name_server = "A-ko";
//            const OS_TaskHd a_ko_thd = OS_TaskByNameGet(task_name_server);
//            task_args_p->a_ko_qhd = OS_TaskStdIoGet(a_ko_thd, OS_STDIO_IN);
//            IF_STATUS(EventCreate(task_args_p->a_ko_qhd, &task_args_p->ehd)) {
//                OS_ASSERT(OS_FALSE);
//            }
            }
            break;
        default:
            break;
    }
    return s;
}

/******************************************************************************/
Status EventCreate(OS_QueueHd a_ko_qhd, OS_EventHd* ehd_p)
{
ConstStrPtr hello_msg = "Hello, world!";
const U8 hello_msg_len = strlen(hello_msg) + 1;
StrPtr hello_msg_p = OS_Malloc(hello_msg_len);
OS_EventItem* ev_item_p;
Status s;

    *ehd_p = OS_NULL;
    OS_StrCpy(hello_msg_p, hello_msg);
    IF_OK(s = OS_EventItemCreate(hello_msg_p, hello_msg_len, &ev_item_p)) {
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
        IF_STATUS(s = OS_EventCreate(&cfg, ehd_p)) {
            OS_LOG_S(D_WARNING, s);
        }
    } else {
        OS_LOG_S(D_WARNING, s);
    }
    return s;
}