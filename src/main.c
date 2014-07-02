/**************************************************************************//**
* @file    main.c
* @brief   Main entry point.
* @author  A. Filyanov
******************************************************************************/
#include "hal.h"
#include "osal.h"
#include "os_task.h"
#include "os_supervise.h"
#include "crc8.h"
#ifdef TEST
#include "test_main.h"
#endif // TEST

//-----------------------------------------------------------------------------
#define MDL_NAME    "main"

//-----------------------------------------------------------------------------
extern const OS_TaskConfig task_a_ko_cfg, task_b_ko_cfg;
// Startup vector.
const OS_TaskConfig* os_startup_v[] = {
    &task_a_ko_cfg,
    &task_b_ko_cfg,
    OS_NULL
};

//-----------------------------------------------------------------------------
/// @brief      Init the device.
/// @return     #Status.
static Status   Init(void);

/// @brief      Init the device applications.
/// @return     #Status.
static Status   APP_Init(void);

static Status FirmwareVersionGet(void);

/******************************************************************************/
void main(void)
{
    // Init the device and it's applications.
    IF_STATUS(Init()) { D_ASSERT(OS_FALSE); }
    D_LOG(D_INFO, "OS scheduler start...");
    OS_SchedulerStart();
    D_ASSERT(OS_FALSE);
}

/******************************************************************************/
Status Init(void)
{
Status s;
    // Hardware init.
    IF_STATUS(s = HAL_Init()) { return s; }
    // OS init.
    IF_STATUS(s = OSAL_Init()){ return s; }
    // Application init.
    IF_STATUS(s = APP_Init()) { return s; }
    return s;
}

/******************************************************************************/
Status APP_Init(void)
{
Status s = S_OK;
    D_LOG(D_INFO, "Application init...");
    D_LOG(D_INFO, "-------------------------------");
    D_LOG(D_INFO, "Firmware: v%d.%d.%d.%d%s",
                   ver_p->maj,
                   ver_p->min,
                   ver_p->bld,
                   ver_p->rev,
                   ver_lbl[ver_p->lbl]);
    D_LOG(D_INFO, "Built on: %s, %s", __DATE__, __TIME__);
    D_LOG(D_INFO, "-------------------------------");
#ifdef OS_TEST
    // Tests.
    D_LOG(D_INFO, "Tests run...\n");
    TestsRun();
    D_LOG(D_INFO, "-------------------------------");
#endif // OS_TEST
    return s;
}

/******************************************************************************/
Status FirmwareVersionGet(void)
{
VersionApp* ver_p = &device_state.description.device_description.version;

    memset((void*)&device_state, 0x0, sizeof(device_state));
    // Static info description.
    ver_p->maj = VERSION_MAJ;
    ver_p->min = VERSION_MIN;
    ver_p->bld = VERSION_BLD;
    ver_p->rev = VERSION_REV;
    ver_p->lbl = VERSION_LBL;
    return S_OK;
}
