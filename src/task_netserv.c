/***************************************************************************//**
* @file    task_netserv.c
* @brief
* @author
*******************************************************************************/
#include "app_common.h"
#include "task_netserv.h"

//-----------------------------------------------------------------------------
#define MDL_NAME            "task_netserv"

//-----------------------------------------------------------------------------
//Task arguments
typedef struct {
    void* args_p;
} TaskStorage;

//------------------------------------------------------------------------------
const OS_TaskConfig task_netserv_cfg = {
    .name           = "NetServ",
    .func_main      = OS_TaskMain,
    .func_power     = OS_TaskPower,
    .args_p         = OS_NULL,
    .attrs          = 0,
    .timeout        = 1,
    .prio_init      = APP_PRIO_TASK_NETSERV,
    .prio_power     = APP_PRIO_PWR_TASK_NETSERV,
    .storage_size   = sizeof(TaskStorage),
    .stack_size     = OS_STACK_SIZE_MIN,
    .stdin_len      = 1
};

/******************************************************************************/
Status OS_TaskInit(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
Status s = S_UNDEF;
    OS_LOG(D_INFO, "Init");
s = S_OK;
    return s;
}

/******************************************************************************/
void OS_TaskMain(OS_TaskArgs* args_p)
{
TaskStorage* tstor_p = (TaskStorage*)args_p->stor_p;
OS_Message* msg_p;
const OS_QueueHd stdin_qhd = OS_TaskStdInGet(OS_THIS_TASK);
Status s = S_UNDEF;

	for(;;) {
        IF_STATUS(OS_MessageReceive(stdin_qhd, &msg_p, OS_BLOCK)) {
            OS_LOG_S(D_WARNING, S_INVALID_MESSAGE);
        } else {
            if (OS_SignalIs(msg_p)) {
                switch (OS_SignalIdGet(msg_p)) {
                    default:
                        OS_LOG_S(D_DEBUG, S_INVALID_SIGNAL);
                        break;
                }
            } else {
                switch (msg_p->id) {
                    default:
                        OS_LOG_S(D_DEBUG, S_INVALID_MESSAGE);
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
Status s = S_UNDEF;
    switch (state) {
        case PWR_STARTUP:
            IF_STATUS(s = OS_TaskInit(args_p)) {
            }
            break;
        case PWR_ON:
            s = S_OK;
            break;
        case PWR_STOP:
            s = S_OK;
            break;
        case PWR_SHUTDOWN:
            s = S_OK;
            break;
        default:
            break;
    }
    return s;
}
