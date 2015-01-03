/**************************************************************************//**
* @file    main.c
* @brief   Main entry point.
* @author  A. Filyanov
******************************************************************************/
#include "hal.h"
#include "osal.h"
#include "os_task.h"
#include "os_supervise.h"
#include "os_startup.h"
#include "version.h"
#include "os_shell_commands_app.h"
#if (1 == OS_TEST_ENABLED)
#include "test_main.h"
#endif // OS_TEST_ENABLED

//-----------------------------------------------------------------------------
#define MDL_NAME    "main"

//-----------------------------------------------------------------------------
/// @brief          Init the device.
/// @return         #Status.
static Status       Init(void);

/// @brief          Init the device applications.
/// @return         #Status.
static Status       APP_Init(void);

/******************************************************************************/
void main(void)
{
    // Init the device and it's applications.
    IF_STATUS(Init()) { HAL_ASSERT(OS_FALSE); }
    HAL_LOG(D_INFO, "OS scheduler start...");
    OS_SchedulerStart();
    HAL_ASSERT(OS_FALSE);
}

/******************************************************************************/
Status Init(void)
{
Status s;
    // Hardware init.
    IF_STATUS(s = HAL_Init_())  { return s; }
    // OS init.
    IF_STATUS(s = OSAL_Init())  { return s; }
    // Application init.
    IF_STATUS(s = APP_Init())   { return s; }
    return s;
}

/******************************************************************************/
Status APP_Init(void)
{
extern const OS_TaskConfig task_a_ko_cfg, task_b_ko_cfg;
extern Status AudioCodecInit_(void);
Status s = S_UNDEF;
#if (OS_AUDIO_ENABLED)
    IF_STATUS(s = AudioCodecInit_()) { return s; }
#endif //(OS_AUDIO_ENABLED)
    IF_STATUS(s = OS_ShellCommandsAppInit()) { return s; }
    // Add application tasks to the system startup.
//    IF_STATUS(s = OS_StartupTaskAdd(&task_a_ko_cfg)) { return s; }
//    IF_STATUS(s = OS_StartupTaskAdd(&task_b_ko_cfg)) { return s; }

    HAL_LOG(D_INFO, "Application init...");
    HAL_LOG(D_INFO, "-------------------------------");
    HAL_LOG(D_INFO, "Firmware: v%d.%d.%d%s-%s",
                    version.maj,
                    version.min,
                    version.bld,
                    ver_lbl[version.lbl],
                    version.rev);
    HAL_LOG(D_INFO, "Built on: %s, %s", __DATE__, __TIME__);
    HAL_LOG(D_INFO, "-------------------------------");
#if (1 == OS_TEST_ENABLED)
    // Tests.
    HAL_LOG(D_INFO, "Tests run...\n");
    TestsRun();
    HAL_LOG(D_INFO, "-------------------------------");
#endif // OS_TEST_ENABLED
    return s;
}
