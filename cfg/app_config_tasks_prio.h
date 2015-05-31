/**************************************************************************//**
* @file    app_config_tasks_prio.h
* @brief   Config header file for the application tasks priorities.
* @author  A. Filyanov
******************************************************************************/
#ifndef _APP_CONFIG_TASKS_PRIO_H_
#define _APP_CONFIG_TASKS_PRIO_H_

//------------------------------------------------------------------------------
// runtime (initial) priority
#define APP_PRIO_TASK_A_KO                   (80)
#define APP_PRIO_TASK_B_KO                   (80)
#define APP_PRIO_TASK_MMPLAY                 (100)
#define APP_PRIO_TASK_NETSERV                (110)

// power priority
#define APP_PRIO_PWR_TASK_A_KO               (OS_PWR_PRIO_DEFAULT + 5)
#define APP_PRIO_PWR_TASK_B_KO               (OS_PWR_PRIO_DEFAULT + 3)
#define APP_PRIO_PWR_TASK_MMPLAY             (OS_PWR_PRIO_DEFAULT + 7)
#define APP_PRIO_PWR_TASK_NETSERV            (OS_PWR_PRIO_DEFAULT + 7)

#endif // _APP_CONFIG_TASKS_PRIO_H_