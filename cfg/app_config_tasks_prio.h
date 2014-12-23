/**************************************************************************//**
* @file    app_config_tasks_prio.h
* @brief   Config header file for the application tasks priorities.
* @author  A. Filyanov
******************************************************************************/
#ifndef _APP_CONFIG_TASKS_PRIO_H_
#define _APP_CONFIG_TASKS_PRIO_H_

//------------------------------------------------------------------------------
// runtime (initial) priority
#define APP_TASK_PRIO_A_KO                   (OS_TASK_PRIO_NORMAL)
#define APP_TASK_PRIO_B_KO                   (OS_TASK_PRIO_NORMAL)
#define APP_TASK_PRIO_MMPLAY                 (OS_TASK_PRIO_HIGH)

// power priority
#define APP_TASK_PRIO_PWR_A_KO               (OS_PWR_PRIO_DEFAULT + 5)
#define APP_TASK_PRIO_PWR_B_KO               (OS_PWR_PRIO_DEFAULT + 3)
#define APP_TASK_PRIO_PWR_MMPLAY             (OS_PWR_PRIO_DEFAULT + 7)

#endif // _APP_CONFIG_TASKS_PRIO_H_