/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-20     LCX       the first version
 */
#ifndef RN8302_RN8302_H_
#define RN8302_RN8302_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>

#define SPI_CS_NUM GET_PIN(C, 0)
#define SPI_DI_NUM GET_PIN(C, 1)
#define SPI_DO_NUM GET_PIN(C, 2)
#define SPI_CLK_NUM GET_PIN(C, 3)

#define SPI_CS_HIGH() rt_pin_write(SPI_CS_NUM, PIN_HIGH)
#define SPI_DI_HIGH() rt_pin_write(SPI_DI_NUM, PIN_HIGH)
#define SPI_DO_HIGH() rt_pin_write(SPI_DO_NUM, PIN_HIGH)
#define SPI_CLK_HIGH() rt_pin_write(SPI_CLK_NUM, PIN_HIGH)

#define SPI_CS_LOW() rt_pin_write(SPI_CS_NUM, PIN_LOW)
#define SPI_DI_LOW() rt_pin_write(SPI_DI_NUM, PIN_LOW)
#define SPI_DO_LOW() rt_pin_write(SPI_DO_NUM, PIN_LOW)
#define SPI_CLK_LOW() rt_pin_write(SPI_CLK_NUM, PIN_LOW)

#define RDSPIData rt_pin_read(SPI_DO_NUM) //读数据
#define ATT_G 1.163
#define INATTV 72 // 模拟扩大8倍
#define INATTI 5  //互感器感应电压 扩大1000倍
#define EC 3200   //电表表常数
#define UI 220    //额定电压
#define VI 0.0478
#define UI_K 10.38 //

#define Exterl_meter_start_number 2
//#define  HFCONST ((2.592*ATT_G*ATT_G*10000*INATTV*INATTI)/(EC*UI*VI))  =76
//#define P_K   1.829E-4//1000000*UI*VI/(ATT_G*ATT_G*INATTV*INATTI*8388608)	//	  2^23 = 8388608

extern rt_uint32_t CurrentElectric; //总电量

extern rt_uint8_t SetGainData; //设置增益数据

typedef struct
{
    rt_uint16_t In;
    rt_uint16_t Un;
    rt_uint16_t Pn;
    rt_uint16_t Qn;
    rt_uint16_t Sn;
    rt_uint8_t Pfn;
    rt_uint32_t EPn; // dian liang
} MeterData;

extern MeterData PowerData[3];

extern rt_uint16_t Huganqibeilv; //互感器倍率

void SPI_GPIO_Config(void);
rt_uint32_t READ_Att70xx(rt_uint8_t Address);
void Write_Att70xx(rt_uint8_t address, rt_uint32_t write_data, rt_uint8_t Date_len);
void EMU_init(void);
void InitAmmeter(void);
void ReadAmmeterData(void);
int Init_Read_RN8302(void);

#endif /* RN8302_RN8302_H_ */
