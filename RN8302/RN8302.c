/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-20     LCX       the first version
 */
/****************SPI 采用端口模拟**********************************/
#include "math.h"
#include "RN8302.h"
#include <rtthread.h>
#include <board.h>

#define LOG_TAG "RN8302"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

//---------------------- RN8302配置寄存器地址定义---------------------------------------------------//
#define HFCONST1 0x0100
#define IStart_PS 0x0102
#define IStart_Q 0x0103
#define LostVoltT 0x0104
#define ZXOT 0x0105
#define PRTH1L 0x0106

// #define			PHSIA    0x010F
// #define			PHSIB    0x0110
// #define        PHSIC    0x0111
// #define        PHSIN 	0x0112
#define GSUA 0x0113
#define GSUB 0x0114
#define GSUC 0x0115
#define GSIA 0x0116
#define GSIB 0x0117
#define GSIC 0x0118
#define IA_OS 0x0124
#define IB_OS 0x0125
#define IC_OS 0x0126
#define GPA 0x0128
#define GPB 0x0129
#define GPC 0x012A

// #define        GQA      0x012B
// #define        GQB      0x012C
// #define        GQC      0x012D
// #define        GSA      0x012E
// #define        GSB      0x012F
// #define        GSC      0x0130
#define PA_PHSL 0x0131
#define PB_PHSL 0x0132
#define PC_PHSL 0x0133
#define QA_PHSL 0x0134
#define QB_PHSL 0x0135
#define QC_PHSL 0x0136
// #define        PA_OS       0x0137
// #define        PB_OS       0x0138
// #define        PC_OS       0x0139

#define CFCFG 0x0160
#define EMUCFG 0x0161
#define EMUCON 0x0162
#define WSAVECON 0x0163
#define CHECKSUM1 0x016A

#define WREN 0x0180
#define WMSW 0x0181
#define SOFTRST 0x0182
#define ADCCFG 0x0183
#define MODSEL 0x0186
#define DeviceID 0x018f
//---------------------- RN8302计量寄存器地址定义---------------------------------------------------//
#define UA 0x0007
#define UB 0x0008
#define UC 0x0009
#define USUM 0x000A
#define IA 0x000B
#define IB 0x000C
#define IC 0x000D
#define IN 0x000E

#define PA 0x0014
#define PB 0x0015
#define PC 0x0016
#define PT 0x0017
#define QA 0x0018
#define QB 0x0019
#define QC 0x001A
#define QT 0x001B
#define SA 0x001C
#define SB 0x001D
#define SC 0x001E
#define STA 0x001F
#define PFA 0x0020
#define PFB 0x0021
#define PFC 0x0022
#define PFTA 0x0023
#define EPA 0x0030
#define EPB 0x0031
#define EPC 0x0032
#define EPT 0x0033

#define HFCONST 4494 //HFConst ＝ INT[P*3.6*10 6 *fosc / (32*EC*Un*Ib*2 31 )]

rt_uint32_t Device_ID = 0;
MeterData PowerData[3];

rt_uint16_t Huganqibeilv = 1; //互感器倍率
rt_uint32_t Kv = 83587;       //电压比例系数
rt_uint32_t Ki = 3362964;     //电流比例系数
rt_uint32_t Kp = 33218;       //功率比例系数

rt_uint32_t Sum;

rt_uint32_t data1;
rt_uint32_t data = 0;
rt_uint8_t SetGainData;

void delay_us(rt_uint32_t us)
{
    rt_uint32_t delta;
    /* 获得延时经过的 tick 数 */
    us = us * (SysTick->LOAD / (1000000 / RT_TICK_PER_SECOND));
    /* 获得当前时间 */
    delta = SysTick->VAL;
    /* 循环获得当前时间，直到达到指定的时间后退出循环 */
    while (delta - SysTick->VAL < us)
        ;
}
/*****************SPI GPIO 功能定义******************************/
void SPI_GPIO_Config(void)
{
    rt_pin_mode(SPI_CS_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(SPI_DI_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(SPI_DO_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(SPI_CLK_NUM, PIN_MODE_OUTPUT);
}
/*****************SPI 写单字节**************************/
void Write_SPI_OneByte(rt_uint8_t data)
{
    rt_uint8_t i, bTemp;

    bTemp = 0x80;

    for (i = 0; i < 8; i++) //Write the Address
    {
        SPI_CLK_HIGH();
        delay_us(1);
        if (data & bTemp)
        {
            SPI_DI_HIGH();
        }
        else
            SPI_DI_LOW();

        bTemp >>= 1;
        delay_us(1);
        SPI_CLK_LOW();
        delay_us(1);
    }

    SPI_DI_LOW();
    delay_us(1);
}
/*****************SPI 读单字节**************************/
rt_uint8_t Read_SPI_OneByte(void)
{
    rt_uint8_t i, data;

    data = 0;

    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        SPI_CLK_HIGH();
        delay_us(1);
        SPI_CLK_LOW();
        delay_us(1);

        if (RDSPIData)
            data |= 1;
        else
            ;
        delay_us(1);
    }
    return data;
}
/************************* READ_RN8302*************************************************/
rt_uint32_t READ_SPI(rt_uint16_t Address, rt_uint8_t Data_len)
{
    rt_uint8_t i, DTemp, CheckSumR;
    rt_uint32_t drData;

    SPI_CS_HIGH();
    SPI_CLK_LOW();
    delay_us(2);
    SPI_CS_LOW(); // 开启SPI传输

    DTemp = (rt_uint8_t)(Address & 0x00FF); //
    CheckSumR = DTemp;
    Write_SPI_OneByte(DTemp);
    DTemp = (((rt_uint8_t)(Address >> 4)) & 0xf0);
    CheckSumR += DTemp;
    Write_SPI_OneByte(DTemp);

    delay_us(5);
    drData = 0x00000000; //Read 24bit

    for (i = 0; i < Data_len; i++)
    {
        DTemp = Read_SPI_OneByte();
        drData <<= 8;
        drData += DTemp;
        CheckSumR += DTemp;
    }

    CheckSumR = ~CheckSumR;

    if (CheckSumR != Read_SPI_OneByte()) //比较校验和
    {
        drData = 0xFFFFFFFF;
    }

    SPI_CS_HIGH(); //关闭SPI传输
    delay_us(2);
    return drData;
}
/**************************Write RN8302******************************/

void Write_SPI(rt_uint16_t Address, rt_uint32_t dwData, rt_uint8_t Date_len)
{
    rt_uint8_t i, CheckSumR, DTemp;
    rt_uint8_t dwTemp, data_buff;

    SPI_CS_HIGH();
    SPI_CLK_LOW();
    SPI_CS_LOW(); //开启SPI传输

    DTemp = (rt_uint8_t)(Address & 0x00FF); //
    CheckSumR = DTemp;
    Write_SPI_OneByte(DTemp); //写地址
    DTemp = (((rt_uint8_t)(Address >> 4)) & 0xf0) + 0x80;
    CheckSumR += DTemp;
    Write_SPI_OneByte(DTemp); //写命令

    for (i = 0; i < Date_len; i++)
    {
        DTemp = (rt_uint8_t)(dwData >> (Date_len - 1 - i) * 8);
        Write_SPI_OneByte(DTemp); //写命令
        CheckSumR += DTemp;
    }

    CheckSumR = ~CheckSumR;

    Write_SPI_OneByte(CheckSumR); //写命令

    SPI_CS_HIGH(); //结束传输
    delay_us(2);
}
void EMU_init(void)
{
    rt_uint16_t data;
    Write_SPI(WREN, 0xE5, 1);    //使能写操作
    Write_SPI(SOFTRST, 0xFA, 1); //复位
    delay_us(30);
    Write_SPI(WREN, 0xE5, 1);        //使能写操作
    Write_SPI(WMSW, 0xA2, 1);        //工作模式EMM
    Write_SPI(HFCONST1, HFCONST, 2); //HFCONST1
    //电压增益
    Write_SPI(GSUA, 0xFF3B, 2); //
    Write_SPI(GSUB, 0xff35, 2);
    Write_SPI(GSUC, 0x000F, 2);
    //电流增益
    Write_SPI(GSIA, 0xFFAE, 2); //
    Write_SPI(GSIB, 0xFFA9, 2);
    Write_SPI(GSIC, 0xFFCB, 2);
    //功率增益
    Write_SPI(GPA, 0x00, 2); //
    Write_SPI(GPB, 0x00, 2);
    Write_SPI(GPC, 0x00, 2);
    //电流offset
    //    Write_SPI(IA_OS,0x0D6F,2); //
    //    Write_SPI(IB_OS,0x0DCA,2);
    //    Write_SPI(IC_OS,0x0D4E,2);

    //有功视在启动电流阈值
    Write_SPI(IStart_PS, 0x0236, 2);
    //无功视在启动电流阈值
    Write_SPI(IStart_Q, 0x0236, 2);
    //失压阈值寄存器
    Write_SPI(LostVoltT, 0x0400, 2);
    //过零阈值寄存器
    Write_SPI(ZXOT, 0x002c, 2);
    //CF管脚配置
    Write_SPI(CFCFG, 0x043210, 3);
    //计量单元配置寄存器
    Write_SPI(EMUCFG, 0x400000, 3);
    //写缓存
    Write_SPI(WSAVECON, 0x10, 1);
    //三相四线
    Write_SPI(MODSEL, 0, 1);
    //计量控制位
    Write_SPI(EMUCON, 0x777777, 3);

    Write_SPI(WREN, 0xDC, 1); //关闭写操作
}
void InitAmmeter(void)
{
    //	data0 = ((2.592 * ATT_G * ATT_G*10000*INATTV*INATTI)/(EC*UI*VI));
    //	data1 = 25920000000	/(HFCONST *EC *8388608);//0.0025745//

    SPI_GPIO_Config(); //管脚初始化
    EMU_init();        // 电表初始化
    delay_us(100);
}
void ReadAmmeterData(void)
{
    rt_uint8_t *Pdata;
    rt_uint8_t i;
    //   rt_uint32_t Ua_ADC ,Ub_ADC,Uc_ADC;
    //   rt_uint32_t Ia_ADC ,Ib_ADC,Ic_ADC;
    rt_uint32_t Pa_ADC, Pb_ADC, Pc_ADC, PT_ADC;
    rt_uint32_t Qa_ADC, Qb_ADC, Qc_ADC, QT_ADC;
    //   rt_uint32_t Sa_ADC ,Sb_ADC,Sc_ADC,ST_ADC;
    rt_uint32_t PFa_ADC, PFb_ADC, PFc_ADC, PFT_ADC;

    Device_ID = READ_SPI(DeviceID, 3); //芯片ID
    if (Device_ID != 0x00830200)       //读ID
    {
        InitAmmeter();
        return;
    }

    Sum = READ_SPI(CHECKSUM1, 3); //校表寄存器效验和1
                                  // 	if((Sum != 0x35A2))
                                  // 	{
                                  // 		InitAmmeter();
                                  //        return;
                                  // 	}
    delay_us(10);

    //读取电压值
    PowerData[0].Un = READ_SPI(UA, 4) / Kv;
    PowerData[1].Un = READ_SPI(UB, 4) / Kv;
    PowerData[2].Un = READ_SPI(UC, 4) / Kv;
    //读取电流值 以mA为单位
    PowerData[0].In = READ_SPI(IA, 4) * 1000 / Ki;
    PowerData[1].In = READ_SPI(IB, 4) * 1000 / Ki;
    PowerData[2].In = READ_SPI(IC, 4) * 1000 / Ki;

    //读取有功功率adc值
    Pa_ADC = READ_SPI(PA, 4);
    Pb_ADC = READ_SPI(PB, 4);
    Pc_ADC = READ_SPI(PC, 4);

    if (Pa_ADC > 0x80000000)
        Pa_ADC = Pa_ADC * (-1);
    if (Pb_ADC > 0x80000000)
        Pb_ADC = Pb_ADC * (-1);
    if (Pc_ADC > 0x80000000)
        Pc_ADC = Pc_ADC * (-1);
    //   PT_ADC = READ_SPI(PT,4);

    PowerData[0].Pn = Pa_ADC / Kp;
    PowerData[1].Pn = Pb_ADC / Kp;
    PowerData[2].Pn = Pc_ADC / Kp;

    //读取无功功率adc值
    Qa_ADC = READ_SPI(QA, 4);
    Qb_ADC = READ_SPI(QB, 4);
    Qc_ADC = READ_SPI(QC, 4);
    //   QT_ADC = READ_SPI(QT,4);
    if (Qa_ADC > 0x80000000)
        Qa_ADC = Qa_ADC * (-1);
    if (Qb_ADC > 0x80000000)
        Qb_ADC = Qb_ADC * (-1);
    if (Qc_ADC > 0x80000000)
        Qc_ADC = Qc_ADC * (-1);
    //    if (ST_ADC >0x80000000)
    //       ST_ADC = ST_ADC*(-1);
    PowerData[0].Qn = Qa_ADC / Kp;
    PowerData[1].Qn = Qb_ADC / Kp;
    PowerData[2].Qn = Qc_ADC / Kp;

    //读取视在功率 符号位始终为0
    PowerData[0].Sn = READ_SPI(SA, 4) / Kp;
    PowerData[1].Sn = READ_SPI(SB, 4) / Kp;
    PowerData[2].Sn = READ_SPI(SC, 4) / Kp;

    //读取功率因数adc值
    PFa_ADC = READ_SPI(PFA, 3);
    PFb_ADC = READ_SPI(PFB, 3);
    PFc_ADC = READ_SPI(PFC, 3);
    PFT_ADC = READ_SPI(PFTA, 3);
    //
    if (PFa_ADC >= 0x800000)
    {
        PFa_ADC <<= 8;
        PFa_ADC = PFa_ADC * (-1);
        PFa_ADC >>= 8;
    }

    if (PFb_ADC >= 0x800000)
    {
        PFb_ADC <<= 8;
        PFb_ADC = PFb_ADC * (-1);
        PFb_ADC >>= 8;
    }

    if (PFc_ADC >= 0x800000)
    {
        PFc_ADC <<= 8;
        PFc_ADC = PFc_ADC * (-1);
        PFc_ADC >>= 8;
    }

    if (PFT_ADC >= 0x800000)
    {
        PFT_ADC <<= 8;
        PFT_ADC = PFT_ADC * (-1);
        PFT_ADC >>= 8;
    }

    PowerData[0].Pfn = PFa_ADC * 100 / 8388608;
    PowerData[1].Pfn = PFb_ADC * 100 / 8388608;
    PowerData[2].Pfn = PFc_ADC * 100 / 8388608;

    // Pdata = (rt_uint8_t *)&PowerData;

    // for (i = 0; i < (sizeof(PowerData)); i++)
    // {
    //     UsartSend_Bit3(Pdata[i], 1);
    // }
}

static void MeterData_task(void *param)
{
    while (1)
    {
        ReadAmmeterData();
        LOG_I("\nUa:%d,Ia:%d,Pa:%d\nUb:%d,Ib:%d,Pb:%d\nUc:%d,Ic:%d,Pc:%d\n",
              PowerData[0].Un,
              PowerData[0].In,
              PowerData[0].Pn,
              PowerData[1].Un,
              PowerData[1].In,
              PowerData[1].Pn,
              PowerData[2].Un,
              PowerData[2].In,
              PowerData[2].Pn);
        rt_thread_mdelay(1000);
    }
}

int Init_Read_RN8302(void)
{
    static rt_thread_t tid1 = RT_NULL;

    tid1 = rt_thread_create("adc_task",
                            MeterData_task, RT_NULL,
                            8192,
                            4, 20);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    return 1;
}
