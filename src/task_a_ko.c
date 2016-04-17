/***************************************************************************//**
* @file    task_a_ko.c
* @brief   A-ko task.
* @author  A. Filyanov
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_supervise.h"
#include "os_driver.h"
#include "os_memory.h"
#include "os_timer.h"
#include "os_trigger.h"
#include "os_environment.h"
#include "drv_rtc.h"
#include "app_common.h"
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
    OS_DriverHd drv_rtc;
    OS_DriverHd drv_button_tamper;
    OS_DriverHd drv_button_wakeup;
    OS_TimerHd  timer_power;
    Bool        button_wakeup_state;
} TaskStorage;

//-----------------------------------------------------------------------------
static void     TimerPowerHandler(TaskStorage* tstor_p);
static void     ButtonTamperHandler(TaskStorage* tstor_p);
static void     ISR_ButtonTamperHandler(void* args_p);
static void     ButtonWakeupHandler(TaskStorage* tstor_p);

//------------------------------------------------------------------------------
const OS_TaskConfig task_a_ko_cfg = {
    .name           = "A-ko",
    .func_main      = OS_TaskMain,
    .func_power     = OS_TaskPower,
    .args_p         = OS_NULL,
    .attrs          = BIT(OS_TASK_ATTR_RECREATE),
    .timeout        = 1,
    .prio_init      = APP_PRIO_TASK_A_KO,
    .prio_power     = APP_PRIO_PWR_TASK_A_KO,
    .storage_size   = sizeof(TaskStorage),
    .stack_size     = OS_STACK_SIZE_MIN,
    .stdin_len      = OS_STDIN_LEN
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
Status s;

    tstor_p->button_wakeup_state = OS_FALSE;
    {
        const OS_DriverConfig drv_cfg = {
            .name       = "BTAMPER",
            .itf_p      = drv_button_v[DRV_ID_BUTTON_TAMPER],
            .prio_power = OS_PWR_PRIO_DEFAULT
        };
        IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&tstor_p->drv_button_tamper)) { return s; }
    }
    {
        const OS_DriverConfig drv_cfg = {
            .name       = "BWAKEUP",
            .itf_p      = drv_button_v[DRV_ID_BUTTON_WAKEUP],
            .prio_power = OS_PWR_PRIO_DEFAULT
        };
        IF_STATUS(s = OS_DriverCreate(&drv_cfg, (OS_DriverHd*)&tstor_p->drv_button_wakeup)) { return s; }
    }
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
OS_Message* msg_p;
const OS_QueueHd stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
volatile int debug = 0;
Status s;

    tstor_p->drv_rtc = OS_DriverRtcGet();

U8 debug_count = (rand() % 17) + 1;
	for(;;) {
        IF_STATUS(s = OS_MessageReceive(stdin_qhd, &msg_p, OS_BLOCK)) {
            //OS_LOG_S(L_WARNING, s);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    case OS_SIG_DRV:
                        if (DRV_ID_GPIO == OS_SignalSrcGet(msg_p)) {
                            switch (OS_SignalDataGet(msg_p)) {
                                case OS_EVENT_TAMPER:
                                    ButtonTamperHandler(tstor_p);
                                    break;
                                case OS_EVENT_WAKEUP:
                                    ButtonWakeupHandler(tstor_p);
                                    break;
                                default:
                                    OS_LOG_S(L_DEBUG_1, S_INVALID_SIGNAL);
                                    break;
                            }
                        }
                        break;
                    case OS_SIG_TIMER: {
                        const OS_TimerId timer_id = OS_TimerIdGet(tstor_p->timer_power);
                        if (timer_id == (OS_TimerId)OS_SignalDataGet(msg_p)) {
                            TimerPowerHandler(tstor_p);
                        }
                        }
                        break;
                    case OS_SIG_TRIGGER: {
                        const OS_TimerId timer_id = (OS_TimerId)OS_SignalDataGet(msg_p);
                            OS_TriggerItem* item_p = OS_TriggerItemByTimerIdGet(timer_id);
                            if (OS_NULL != item_p) {
                                IF_OK(s = OS_TriggerItemLock(item_p, OS_TIMEOUT_MUTEX_LOCK)) {
                                    OS_LOG(L_INFO, item_p->data_p);
                                    IF_STATUS(s = OS_TriggerItemUnlock(item_p)) {
                                        OS_LOG_S(L_WARNING, s);
                                    }
                                } else {
                                    OS_LOG_S(L_WARNING, s);
                                }
                            }
                        }
                        break;
                    case OS_SIG_APP:
                        debug = OS_SignalDataGet(msg_p);
                        OS_LOG_S(L_DEBUG_1, S_OK);
                        break;
                    case OS_SIG_PWR_ACK:
                        break;
                    default:
                        OS_LOG_S(L_DEBUG_1, S_INVALID_SIGNAL);
                        break;
                }
            } else {
                switch (msg_p->id) {
                    case OS_MSG_APP:
                        debug = 2;
                        break;
                    default:
                        OS_LOG_S(L_DEBUG_1, S_INVALID_MESSAGE);
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
        case PWR_SHUTDOWN: {
            IF_STATUS(s = OS_TimerDelete(tstor_p->timer_power, OS_TIMEOUT_DEFAULT)) {
            }
            tstor_p->timer_power = OS_NULL;
//            IF_OK(s = OS_DriverClose(tstor_p->drv_button_tamper, OS_NULL)) {
//                IF_STATUS(s = OS_DriverDeInit(tstor_p->drv_button_tamper, OS_NULL)) {
//                }
//            } else {
//                s = (S_INITED == s) ? S_OK : s;
//            }
            IF_OK(s = OS_DriverClose(tstor_p->drv_button_wakeup, OS_NULL)) {
                IF_STATUS(s = OS_DriverDeInit(tstor_p->drv_button_wakeup, OS_NULL)) {
                }
            } else {
                s = (S_INITED == s) ? S_OK : s;
            }
            }
            break;
        case PWR_ON: {
            if (OS_NULL == tstor_p->timer_power) {
                tstor_p->stdin_qhd = OS_TaskStdInGet(OS_TaskByNameGet(task_a_ko_cfg.name));
                static ConstStrP tim_name_p = "PowerOff";
                const OS_TimerConfig tim_cfg = {
                    .name_p = tim_name_p,
                    .slot   = tstor_p->stdin_qhd,
                    .id     = 16,
                    .period = 4000,
                    .options= (OS_TimerOptions)0
                };
                IF_STATUS(s = OS_TimerCreate(&tim_cfg, &tstor_p->timer_power)) {
                    return s;
                }
            }
            IF_OK(s = OS_DriverInit(OS_DriverGpioGet(), (void*)GPIO_BUTTON_WAKEUP)) {
                const DrvGpioArgsOpen args = {
                    .gpio               = GPIO_BUTTON_WAKEUP,
                    .signal_id          = OS_SIG_DRV,
                    .signal_data        = OS_EVENT_WAKEUP,
                    .slot_qhd           = tstor_p->stdin_qhd
                };
                IF_STATUS(s = OS_DriverOpen(OS_DriverGpioGet(), (void*)&args)) {
                }
            } else {
                s = (S_INITED == s) ? S_OK : s;
            }
//            IF_OK(s = OS_DriverInit(tstor_p->drv_button_tamper, OS_NULL)) {
//                IF_STATUS(s = OS_DriverOpen(tstor_p->drv_button_tamper, (void*)ISR_ButtonTamperHandler)) {
//                }
//            } else {
//                s = (S_INITED == s) ? S_OK : s;
//            }
            IF_OK(s = OS_DriverInit(tstor_p->drv_button_wakeup, OS_NULL)) {
                IF_STATUS(s = OS_DriverOpen(tstor_p->drv_button_wakeup, OS_NULL)) {
                }
            } else {
                s = (S_INITED == s) ? S_OK : s;
            }
            }
            break;
        default:
            break;
    }
    return s;
}

/******************************************************************************/
void TimerPowerHandler(TaskStorage* tstor_p)
{
const OS_Signal signal = OS_SignalCreate(OS_SIG_SHUTDOWN, 0);
    OS_SignalSend(OS_TaskSvStdInGet(), signal, OS_MSG_PRIO_HIGH);
}

/******************************************************************************/
void ButtonTamperHandler(TaskStorage* tstor_p)
{
    OS_LOG(L_WARNING, "Tamper event detected!");
    IF_STATUS(OS_DriverIoCtl(tstor_p->drv_rtc, DRV_REQ_BUTTON_TAMPER_DISABLE, OS_NULL)) { OS_ASSERT(OS_FALSE); }
    //Wait for jitter to calm.
    OS_TaskDelay(100);
    //Remove jitter signals from the queue.
    OS_QueueClear(tstor_p->stdin_qhd);
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
    IF_STATUS(OS_DriverIoCtl(tstor_p->drv_rtc, DRV_REQ_BUTTON_TAMPER_ENABLE, OS_NULL)) { OS_ASSERT(OS_FALSE); }
}

/******************************************************************************/
void ISR_ButtonTamperHandler(void* args_p)
{
//    if (1 == OS_ISR_SignalSend(task_args.stdin_qhd,
//                               OS_SignalCreate(OS_SIG_DRV, OS_EVENT_TAMPER),
//                               OS_MSG_PRIO_HIGH)) {
//        OS_ContextSwitchForce();
//    }
}

/******************************************************************************/
void ButtonWakeupHandler(TaskStorage* tstor_p)
{
    tstor_p->button_wakeup_state ^= OS_TRUE;
    if (OS_TRUE == tstor_p->button_wakeup_state) {
        OS_LOG(L_INFO, "Wakeup button pressed");
    } else {
        OS_LOG(L_INFO, "Wakeup button released");
    }
    OS_TimerStart(tstor_p->timer_power, 4000);
    //Wait for debounce.
    OS_TaskDelay(10);
    //Remove jitter signals from the queue.
    OS_QueueClear(tstor_p->stdin_qhd);
    // Is button released?
    if (OS_FALSE == tstor_p->button_wakeup_state) {
        static OS_PowerState state_prev = PWR_UNDEF;
        const OS_PowerState state = OS_PowerStateGet();
        IF_STATUS(OS_TimerStop(tstor_p->timer_power, OS_TIMEOUT_DEFAULT)) {
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
