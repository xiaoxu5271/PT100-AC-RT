/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-19     RT-Thread    first version
 */

#include <rtthread.h>
#include "PT100.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    Init_Read_PT100();
    Init_Read_RN8302();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
