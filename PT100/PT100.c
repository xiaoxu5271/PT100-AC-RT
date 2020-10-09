/*
 * 程序清单： ADC 设备使用例程
 * 例程导出了 adc_sample 命令到控制终端
 * 命令调用格式：adc_sample
 * 程序功能：通过 ADC 设备采样电压值并转换为数值。
 *           示例代码参考电压为3.3V,转换位数为12位。
*/

#include <rtthread.h>
#include <rtdevice.h>
#include <math.h>

#define LOG_TAG "PT100"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#include "PT100.h"

#define ADC_DEV_NAME "adc1"    /* ADC 设备名称 */
#define REFER_VOLTAGE 330      /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define CONVERT_BITS (1 << 12) /* 转换位数为12位 */

#define PT100_CHANNEL_1 0      /* ADC 通道 */
#define PT100_CHANNEL_2 4      /* ADC 通道 */
#define PT100_CHANNEL_3 1      /* ADC 通道 */
#define PT100_CHANNEL_4 6      /* ADC 通道 */
#define PT100_CHANNEL_5 9      /* ADC 通道 */
#define PT100_CHANNEL_6 14     /* ADC 通道 */
#define PT100_CHANNEL_7 15     /* ADC 通道 */
#define PT100_CHANNEL_8 8      /* ADC 通道 */
#define REFER_VOLTAG 330       /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define CONVERT_BITS (1 << 12) /* 转换位数为12位 */

static float Get_Pt100_Temp(rt_uint32_t adc_value);

static void read_adc_task(void *param)
{
    rt_adc_device_t adc_dev;
    rt_uint32_t value[8] = {0};

    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    while (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
    }
    /* 使能设备 */
    rt_adc_enable(adc_dev, PT100_CHANNEL_1);
    rt_adc_enable(adc_dev, PT100_CHANNEL_2);
    rt_adc_enable(adc_dev, PT100_CHANNEL_3);
    rt_adc_enable(adc_dev, PT100_CHANNEL_4);
    rt_adc_enable(adc_dev, PT100_CHANNEL_5);
    rt_adc_enable(adc_dev, PT100_CHANNEL_6);
    rt_adc_enable(adc_dev, PT100_CHANNEL_7);
    rt_adc_enable(adc_dev, PT100_CHANNEL_8);

    while (1)
    {
        /* 读取采样值 */
        value[0] = rt_adc_read(adc_dev, PT100_CHANNEL_1);
        value[1] = rt_adc_read(adc_dev, PT100_CHANNEL_2);
        value[2] = rt_adc_read(adc_dev, PT100_CHANNEL_3);
        value[3] = rt_adc_read(adc_dev, PT100_CHANNEL_4);
        value[4] = rt_adc_read(adc_dev, PT100_CHANNEL_5);
        value[5] = rt_adc_read(adc_dev, PT100_CHANNEL_6);
        value[6] = rt_adc_read(adc_dev, PT100_CHANNEL_7);
        value[7] = rt_adc_read(adc_dev, PT100_CHANNEL_8);

        for (rt_uint8_t i = 0; i < 8; i++)
        {
            rt_kprintf("value[%d]=%d ", i + 1, value[i]);
        }
        rt_kprintf("\n");
        Get_Pt100_Temp(value[0]);

        /* 转换为对应电压值 */
        // vol = value * REFER_VOLTAGE / CONVERT_BITS;
        // rt_kprintf("the voltage is :%d.%02d \n", vol / 100, vol % 100);
        rt_thread_mdelay(1000);
    }

    /* 关闭通道 */
    // ret = rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
}

int Init_Read_PT100(void)
{
    static rt_thread_t tid1 = RT_NULL;

    tid1 = rt_thread_create("adc_task",
                            read_adc_task, RT_NULL,
                            8192,
                            4, 20);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    return 1;
}

static float Get_Pt100_Temp(rt_uint32_t adc_value)
{
    double F_T;

    if (adc_value >= 0 && adc_value < 1810)
    {
        F_T = (3e-9) * pow(adc_value, 3) - (6e-6) * pow(adc_value, 2) + 0.0692 * adc_value + 4.7117;
    }

    if (adc_value >= 1875)
    {
        // F_T = -1 * (4e-6) * pow(adc_value, 3) + 0.024 * pow(adc_value, 2) - 48.485 * adc_value + 32447;
        F_T = 0.001 * pow(adc_value, 2) - 3.5727 * adc_value + 3259.2;
    }
    // rt_kprintf("temp=%.3f\n", F_T);
    LOG_I("temp=%.3f\n", F_T);
    return F_T;
}
