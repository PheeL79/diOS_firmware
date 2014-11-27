/***************************************************************************//**
* @file    task_a_ko.c
* @brief   A-ko task.
* @author  A. Filyanov
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_supervise.h"
#include "os_message.h"
#include "os_debug.h"
#include "os_power.h"
#include "os_driver.h"
#include "os_memory.h"
#include "os_signal.h"
#include "os_timer.h"
#include "os_event.h"
#include "os_environment.h"
#include "drv_rtc.h"
#include "task_a_ko.h"

//------------------------------------------------------------------------------
enum {
    OS_EVENT_TAMPER,
    OS_EVENT_WAKEUP,
};

//-----------------------------------------------------------------------------
//Task arguments
typedef struct {
    OS_QueueHd  stdin_qhd;
    OS_QueueHd  stdout_qhd;
    OS_DriverHd drv_rtc;
    OS_DriverHd drv_button_tamper;
    OS_DriverHd drv_button_wakeup;
    OS_TimerHd  timer_power;
    Bool        button_wakeup_state;
} TaskArgs;

static TaskArgs task_args = {
    .stdin_qhd          = OS_NULL,
    .stdout_qhd         = OS_NULL,
    .drv_rtc            = OS_NULL,
    .drv_button_tamper  = OS_NULL,
    .drv_button_tamper  = OS_NULL,
    .timer_power        = OS_NULL,
    .button_wakeup_state= OS_FALSE
};

//-----------------------------------------------------------------------------
static void     TimerPowerHandler(TaskArgs* task_args_p);
static void     ButtonTamperHandler(TaskArgs* task_args_p);
static void     ISR_ButtonTamperHandler(void);
static void     ButtonWakeupHandler(TaskArgs* task_args_p);
static void     ISR_ButtonWakeupHandler(void);

//------------------------------------------------------------------------------
const OS_TaskConfig task_a_ko_cfg = {
    .name       = "A-ko",
    .func_main  = OS_TaskMain,
    .func_power = OS_TaskPower,
    .args_p     = (void*)&task_args,
    .attrs      = BIT(OS_TASK_ATTR_RECREATE),
    .timeout    = 1,
    .prio_init  = OS_TASK_PRIO_NORMAL,
    .prio_power = OS_PWR_PRIO_DEFAULT + 5,
    .stack_size = OS_STACK_SIZE_MIN,
    .stdin_len  = OS_STDIN_LEN
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
Status s;
    {
        const OS_DriverConfig drv_cfg = {
            .name       = "BTAMPER",
            .itf_p      = drv_button_v[DRV_ID_BUTTON_TAMPER],
            .prio_power = OS_PWR_PRIO_DEFAULT
        };
        IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&task_args_p->drv_button_tamper)) { return s; }
    }
    {
        const OS_DriverConfig drv_cfg = {
            .name       = "BWAKEUP",
            .itf_p      = drv_button_v[DRV_ID_BUTTON_WAKEUP],
            .prio_power = OS_PWR_PRIO_DEFAULT
        };
        IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&task_args_p->drv_button_wakeup)) { return s; }
    }
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskArgs* task_args_p = (TaskArgs*)args_p;
OS_Message* msg_p;
volatile int debug = 0;
Status s;

    task_args_p->drv_rtc = OS_DriverRtcGet();

U8 debug_count = (rand() % 17) + 1;
	for(;;) {
        IF_STATUS(s = OS_MessageReceive(task_args_p->stdin_qhd, &msg_p, OS_BLOCK)) {
            //OS_LOG_S(D_WARNING, s);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_DRV:
                        switch (OS_SignalDataGet(msg_p)) {
                            case OS_EVENT_TAMPER:
                                ButtonTamperHandler(task_args_p);
                                break;
                            case OS_EVENT_WAKEUP:
                                ButtonWakeupHandler(task_args_p);
                                break;
                            default:
                                OS_LOG_S(D_DEBUG, S_UNDEF_SIG);
                                break;
                        }
                        break;
                    case OS_SIG_TIMER: {
                        const OS_TimerId timer_id = OS_TimerIdGet(task_args_p->timer_power);
                        if (timer_id == (OS_TimerId)OS_SignalDataGet(msg_p)) {
                            TimerPowerHandler(task_args_p);
                        }
                        }
                        break;
                    case OS_SIG_EVENT: {
                        const OS_TimerId timer_id = (OS_TimerId)OS_SignalDataGet(msg_p);
                            OS_EventItem* item_p = OS_EventItemByTimerIdGet(timer_id);
                            if (OS_NULL != item_p) {
                                IF_OK(s = OS_EventItemLock(item_p, OS_TIMEOUT_MUTEX_LOCK)) {
                                    OS_LOG(D_INFO, item_p->data_p);
                                    IF_STATUS(s = OS_EventItemUnlock(item_p)) {
                                        OS_LOG_S(D_WARNING, s);
                                    }
                                } else {
                                    OS_LOG_S(D_WARNING, s);
                                }
                            }
                        }
                        break;
                    case OS_SIG_APP:
                        debug = OS_SignalDataGet(msg_p);
                        OS_LOG_S(D_DEBUG, S_OK);
                        break;
                    case OS_SIG_PWR_ACK:
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
        case PWR_SHUTDOWN: {
            IF_STATUS(s = OS_TimerDelete(task_args_p->timer_power, OS_TIMEOUT_DEFAULT)) {
            }
            task_args_p->timer_power = OS_NULL;
            IF_OK(s = OS_DriverClose(task_args.drv_button_tamper, OS_NULL)) {
                IF_STATUS(s = OS_DriverDeInit(task_args.drv_button_tamper, OS_NULL)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            IF_OK(s = OS_DriverClose(task_args.drv_button_wakeup, OS_NULL)) {
                IF_STATUS(s = OS_DriverDeInit(task_args.drv_button_wakeup, OS_NULL)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            }
            break;
        case PWR_ON: {
            if (OS_NULL == task_args_p->timer_power) {
                task_args_p->stdin_qhd = OS_TaskStdInGet(OS_TaskByNameGet(task_a_ko_cfg.name));
                static ConstStrPtr tim_name_p = "PowerOff";
                const OS_TimerConfig tim_cfg = {
                    .name_p = tim_name_p,
                    .slot   = task_args_p->stdin_qhd,
                    .id     = 16,
                    .period = 4000,
                    .options= OS_TIM_OPT_UNDEF
                };
                IF_STATUS(s = OS_TimerCreate(&tim_cfg, &task_args_p->timer_power)) {
                    return s;
                }
            }
            IF_OK(s = OS_DriverInit(task_args.drv_button_tamper, OS_NULL)) {
                IF_STATUS(s = OS_DriverOpen(task_args.drv_button_tamper, (void*)ISR_ButtonTamperHandler)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            IF_OK(s = OS_DriverInit(task_args.drv_button_wakeup, OS_NULL)) {
                IF_STATUS(s = OS_DriverOpen(task_args.drv_button_wakeup, (void*)ISR_ButtonWakeupHandler)) {
                }
            } else {
                s = (S_INIT == s) ? S_OK : s;
            }
            }
            break;
        default:
            break;
    }
    return s;
}

/******************************************************************************/
void TimerPowerHandler(TaskArgs* task_args_p)
{
const OS_Signal signal = OS_SignalCreate(OS_SIG_SHUTDOWN, 0);
    OS_SignalSend(OS_TaskSvStdInGet(), signal, OS_MSG_PRIO_HIGH);
}

/******************************************************************************/
void ButtonTamperHandler(TaskArgs* task_args_p)
{
    OS_LOG(D_WARNING, "Tamper event detected!");
    IF_STATUS(OS_DriverIoCtl(task_args_p->drv_rtc, DRV_REQ_BUTTON_TAMPER_DISABLE, OS_NULL)) { OS_ASSERT(OS_FALSE); }
    //Wait for jitter to calm.
    OS_TaskDelay(100);
    //Remove jitter signals from the queue.
    OS_QueueClear(task_args_p->stdin_qhd);
//#ifndef NDEBUG
//U32 size = 0x1000;
//U8* data_p = OS_MallocEx(size, OS_MEM_RAM_INT_CCM);
//HAL_RTC_BackupRegWrite io_args = {
//    .reg = HAL_RTC_BKUP_REG_USER,
//    .val = (U32)0xDEADBEEFUL
//};
//    OS_DriverIoCtl(task_args.drv_rtc, DRV_REQ_RTC_BKUP_REG_WRITE, &io_args);
//    if (OS_NULL != data_p) {
//        memset(data_p, (rand() % 0xFF), size);
//        OS_DriverWrite(task_args.drv_rtc, data_p, size, OS_NULL);
//    }
//    OS_FreeEx(data_p, OS_MEM_RAM_INT_CCM);
//#endif // NDEBUG
    IF_STATUS(OS_DriverIoCtl(task_args_p->drv_rtc, DRV_REQ_BUTTON_TAMPER_ENABLE, OS_NULL)) { OS_ASSERT(OS_FALSE); }
}

/******************************************************************************/
void ISR_ButtonTamperHandler(void)
{
    if (1 == OS_ISR_SignalSend(task_args.stdin_qhd,
                               OS_SignalCreate(OS_SIG_DRV, OS_EVENT_TAMPER),
                               OS_MSG_PRIO_HIGH)) {
        OS_ContextSwitchForce();
    }
}

/******************************************************************************/
void ButtonWakeupHandler(TaskArgs* task_args_p)
{
    task_args_p->button_wakeup_state ^= OS_TRUE;
    if (OS_TRUE == task_args_p->button_wakeup_state) {
        OS_LOG(D_INFO, "Wakeup button pressed");
    } else {
        OS_LOG(D_INFO, "Wakeup button released");
    }
    OS_TimerStart(task_args_p->timer_power, 4000);
    //Wait for debounce.
    OS_TaskDelay(10);
    //Remove jitter signals from the queue.
    OS_QueueClear(task_args_p->stdin_qhd);
    // Is button released?
    if (OS_FALSE == task_args_p->button_wakeup_state) {
        static OS_PowerState state_prev = PWR_UNDEF;
        const OS_PowerState state = OS_PowerStateGet();
        IF_STATUS(OS_TimerStop(task_args_p->timer_power, OS_TIMEOUT_DEFAULT)) {
            return;
        }
        if (PWR_STOP != state) {
            OS_PowerStateSet(PWR_STOP);
        } else {
            OS_PowerStateSet(state_prev);
        }
        state_prev = state;
    }
}

/******************************************************************************/
void ISR_ButtonWakeupHandler(void)
{
    if (1 == OS_ISR_SignalSend(task_args.stdin_qhd,
                               OS_ISR_SignalCreate(DRV_ID_BUTTON_WAKEUP, OS_SIG_DRV, OS_EVENT_WAKEUP),
                               OS_MSG_PRIO_HIGH)) {
        OS_ContextSwitchForce();
    }
}
