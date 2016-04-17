/***************************************************************************//**
* @file    task_b_ko.c
* @brief   B-ko task.
* @author  A. Filyanov
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drv_usb.h"
#include "os_timer.h"
#include "os_trigger.h"
#include "os_driver.h"
#include "os_memory.h"
#include "os_settings.h"
#include "os_environment.h"
#include "app_common.h"
#include "task_b_ko.h"

//-----------------------------------------------------------------------------
//Task arguments
typedef struct {
    OS_DriverHd     drv_gpio;
    OS_TriggerHd    trigger_hd;
    OS_TimerHd      led_tim_hd;
    OS_QueueHd      a_ko_qhd;
    OS_TimeMs       blink_rate;
    OS_TimeMs       period;
    S32             pwm_step;
    S32             pwm_pulse;
} TaskStorage;

//------------------------------------------------------------------------------
const OS_TaskConfig task_b_ko_cfg = {
    .name           = "B-ko",
    .func_main      = OS_TaskMain,
    .func_power     = OS_TaskPower,
    .args_p         = OS_NULL,
    .attrs          = BIT(OS_TASK_ATTR_RECREATE),
    .timeout        = 4,
    .prio_init      = APP_PRIO_TASK_B_KO,
    .prio_power     = APP_PRIO_PWR_TASK_B_KO,
    .storage_size   = sizeof(TaskStorage),
    .stack_size     = OS_STACK_SIZE_MIN,
    .stdin_len      = OS_STDIN_LEN
};

//------------------------------------------------------------------------------
static Status       TriggerCreate(OS_QueueHd a_ko_qhd, OS_TriggerHd* trigger_hd_p);

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
Status s = S_OK;
    tstor_p->blink_rate = OS_BLOCK;
    tstor_p->drv_gpio = OS_DriverGpioGet();
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
//State led_state = OFF;
const OS_QueueHd stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
OS_Message* msg_p;// = OS_MessageCreate(OS_MSG_APP, sizeof(task_args), OS_BLOCK, &task_args);
int debug = 0;
//const OS_Signal signal = OS_SignalCreate(OS_SIG_APP, 0);
Status s;

//    OS_SIGNAL_EMIT(a_ko_qhd, signal, OS_MSG_PRIO_HIGH);
//    OS_MessageSend(a_ko_qhd, msg_p, 100, OS_MSG_PRIO_NORMAL);

//U8 debug_count = (rand() % 6) + 1;
//tstor_p->blink_rate = 1;
	for(;;) {
        IF_STATUS(OS_MessageReceive(stdin_qhd, &msg_p, OS_BLOCK)) {
            //OS_LOG_S(L_WARNING, S_UNDEF_MSG);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_APP:
                        debug = OS_SignalDataGet(msg_p);
                        break;
                    case OS_SIG_TIMER: {
                        const OS_TimerId timer_id = OS_TimerIdGet(tstor_p->led_tim_hd);
                        if (timer_id == (OS_TimerId)OS_SignalDataGet(msg_p)) {
                            const S32 pwm_pulse = (tstor_p->pwm_pulse + tstor_p->pwm_step);
                            if (U16_MAX < pwm_pulse) {
                                tstor_p->pwm_pulse = U16_MAX;
                                tstor_p->pwm_step *= -1;
                            } else if (0 > pwm_pulse) {
                                tstor_p->pwm_pulse = 0;
                                tstor_p->pwm_step *= -1;
                            } else {
                                tstor_p->pwm_pulse = pwm_pulse;
                            }
                            const DrvGpioArgsIoCtlPwm gpio_pwm_args = {
                                .pwm_pulse  = (U16)tstor_p->pwm_pulse,
                                .gpio       = GPIO_LED_USER
                            };
                            IF_OK(s = OS_DriverIoCtl(tstor_p->drv_gpio, DRV_REQ_GPIO_PWM_SET, (void*)&gpio_pwm_args)) {
                            }
                        }
                        }
                        break;
                    default:
                        OS_LOG_S(L_DEBUG_1, S_INVALID_SIGNAL);
                        break;
                }
            } else {
                switch (msg_p->id) {
                    case OS_MSG_USB_HID_MOUSE: {
                        const OS_UsbHidMouseData* mouse_p = (OS_UsbHidMouseData*)&(msg_p->data);
                        const U8 l = BIT_TEST(mouse_p->buttons_bf, BIT(OS_USB_HID_MOUSE_BUTTON_LEFT));
                        const U8 r = BIT_TEST(mouse_p->buttons_bf, BIT(OS_USB_HID_MOUSE_BUTTON_RIGHT));
                        const U8 m = BIT_TEST(mouse_p->buttons_bf, BIT(OS_USB_HID_MOUSE_BUTTON_MIDDLE));
                        OS_LOG(L_DEBUG_1, "X:%d, Y:%d, L:%d, M:%d, R:%d",
                                        mouse_p->x, mouse_p->y, l, m, r);
                        }
                        break;
//                    case OS_MSG_APP:
//                        debug = 2;
//                        break;
                    default:
                        OS_LOG_S(L_DEBUG_1, S_INVALID_MESSAGE);
                        break;
                }
                OS_MessageDelete(msg_p); // free message allocated memory
            }
        }
//        led_state = (ON == led_state) ? OFF : ON;
//        OS_DriverWrite(tstor_p->drv_gpio, (void*)GPIO_LED_USER, 0, (void*)led_state);

//        if (!--debug_count) {
//            while(1) {};
//        }
    }
}

/******************************************************************************/
Status OS_TaskPower(OS_TaskArgs* args_p, const OS_PowerState state)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
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
            IF_STATUS(s = OS_TimerDelete(tstor_p->led_tim_hd, OS_TIMEOUT_DEFAULT)) {
            }
//            IF_STATUS(s = OS_TriggerDelete(tstor_p->trigger_hd, OS_TIMEOUT_DEFAULT)) {
//                OS_LOG_S(L_WARNING, s);
//            }
//            IF_OK(s = OS_DriverClose(task_args.drv_led_user)) {
//                IF_STATUS(s = OS_DriverDeInit(task_args.drv_led_user)) {
//                }
//            } else {
//                s = (S_INITED == s) ? S_OK : s;
//            }
            }
            break;
        case PWR_ON: {
            ConstStrP config_path_p = OS_EnvVariableGet("config_file");
            Str value[OS_SETTINGS_VALUE_LEN];
            OS_SettingsRead(config_path_p, "Second", "blink_rate", &value[0]);
            tstor_p->blink_rate = OS_StrToUL((const char*)&value[0], OS_NULL, 10);
            OS_SettingsRead(config_path_p, "Second", "period", &value[0]);
            tstor_p->period     = OS_StrToUL((const char*)&value[0], OS_NULL, 10);
            tstor_p->pwm_step = (U16_MAX / (tstor_p->blink_rate / tstor_p->period));

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
//            tstor_p->a_ko_qhd = OS_TaskStdIoGet(a_ko_thd, OS_STDIO_IN);
//            IF_STATUS(TriggerCreate(tstor_p->a_ko_qhd, &tstor_p->trigger_hd)) {
//                OS_ASSERT(OS_FALSE);
//            }
            const DrvGpioArgsOpen gpio_open_args = {
                .gpio = GPIO_LED_USER
            };
            IF_OK(s = OS_DriverOpen(tstor_p->drv_gpio, (void*)&gpio_open_args)) {
                if (OS_NULL == tstor_p->led_tim_hd) {
                    static ConstStrP tim_name_p = "LED";
                    const OS_TimerConfig tim_cfg = {
                        .name_p = tim_name_p,
                        .slot   = OS_TaskStdInGet(OS_TaskByNameGet(task_b_ko_cfg.name)),
                        .id     = 13,
                        .period = tstor_p->period,
                        .options= (OS_TimerOptions)(BIT(OS_TIM_OPT_PERIODIC))
                    };
                    IF_OK(s = OS_TimerCreate(&tim_cfg, &tstor_p->led_tim_hd)) {
                        s = OS_TimerStart(tstor_p->led_tim_hd, tim_cfg.period);
                    }
                }
            }
            }
            break;
        default:
            break;
    }
    return s;
}

/******************************************************************************/
Status TriggerCreate(OS_QueueHd a_ko_qhd, OS_TriggerHd* trigger_hd_p)
{
ConstStrP hello_msg = "Hello, world!";
const U8 hello_msg_len = strlen(hello_msg) + 1;
StrP hello_msg_p = OS_Malloc(hello_msg_len);
OS_TriggerItem* trig_item_p;
Status s;

    *trigger_hd_p = OS_NULL;
    OS_StrCpy(hello_msg_p, hello_msg);
    IF_OK(s = OS_TriggerItemCreate(hello_msg_p, hello_msg_len, &trig_item_p)) {
        static ConstStrP tim_name_p = "TriggerT";
        const OS_TimerConfig tim_cfg = {
            .name_p = tim_name_p,
            .slot   = a_ko_qhd,
            .id     = 18,
            .period = 10000,
            .options= (OS_TimerOptions)(BIT(OS_TIM_OPT_PERIODIC) | BIT(OS_TIM_OPT_TRIGGER))
        };
        const OS_TriggerConfig cfg = {
            .timer_cfg_p= &tim_cfg,
            .item_p     = trig_item_p,
            .state      = OS_TRIGGER_STATE_UNDEF
        };
        IF_STATUS(s = OS_TriggerCreate(&cfg, trigger_hd_p)) {
            OS_LOG_S(L_WARNING, s);
        }
    } else {
        OS_LOG_S(L_WARNING, s);
    }
    return s;
}