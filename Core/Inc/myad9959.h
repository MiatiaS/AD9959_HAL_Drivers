//
// Created by 20614 on 25-6-30.
//

#ifndef MYAD9959_H
#define MYAD9959_H

#include "main.h"



#define Sweep_Fre		0	// 扫频
#define Sweep_Phase		1	// 扫相
#define Sweep_Amp		2	// 扫幅

/***************************通道寄存器地址宏定义**************************************/
#define CSR 	0x00		/* 通道选择寄存器 */
#define FR1 	0x01		/* 功能寄存器1 */
#define FR2 	0x02		/* 功能寄存器2 */
#define CFR 	0x03		/* 通道功能寄存器 */
#define CFTW0 	0x04		/* 32位通道频率转换字寄存器 */
#define CPOW0 	0x05		/* 14位通道相位转换字寄存器 */
#define ACR 	0x06		/* 幅度控制寄存器 */
#define SRR 	0x07		/* 线性扫描定时器 */
#define RDW 	0x08		/* 线性向上扫描定时器 */
#define FDW 	0x09		/* 线性向下扫描定时器 */

/*********************************引脚连接说明*********************************************/
/*
 * STM32H7与AD9959引脚连接关系说明：
 *
 * STM32引脚        AD9959引脚       功能说明                    CubeMX中的标签名
 * ---------------------------------------------------------------------------------
 * PA9         <->  CS             片选信号(低电平有效)         AD9959_CS
 * PA8         <->  I/O UPDATE     数据更新信号                 AD9959_UD
 * PC9         <->  SCLK           SPI时钟信号                  AD9959_CLK
 * PC8         <->  SDIO_0         串行数据输入线0              AD9959_SD0
 * PC7         <->  SDIO_1         串行数据输入线1              AD9959_SD1
 * PC6         <->  SDIO_2         串行数据输入线2              AD9959_SD2
 * PD15        <->  SDIO_3         串行数据输入线3              AD9959_SD3
 * PD14        <->  RESET          硬件复位信号(低电平有效)     AD9959_RST
 * PD13        <->  PWR DWN CTL    功率下降控制(低电平有效)     AD9959_PDC
 *
 * 注意事项：
 * 1. 所有GPIO引脚都配置为推挽输出模式
 * 2. AD9959的VCC应连接到3.3V电源
 * 3. 建议在每个信号线上串联22Ω电阻以减少信号反射
 * 4. AD9959的AGND和DGND应该良好接地
 * 5. 外部晶振频率为25MHz，通过内部PLL倍频到500MHz系统时钟
 */

/*********************************引脚控制宏定义*********************************************/
#define AD9959_CS(x)      	HAL_GPIO_WritePin(AD9959_CS_GPIO_Port, AD9959_CS_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_UD(x)      	HAL_GPIO_WritePin(AD9959_UD_GPIO_Port, AD9959_UD_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_CLK(x)     	HAL_GPIO_WritePin(AD9959_CLK_GPIO_Port, AD9959_CLK_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_SD0(x)     	HAL_GPIO_WritePin(AD9959_SD0_GPIO_Port, AD9959_SD0_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_SD1(x)     	HAL_GPIO_WritePin(AD9959_SD1_GPIO_Port, AD9959_SD1_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_SD2(x)     	HAL_GPIO_WritePin(AD9959_SD2_GPIO_Port, AD9959_SD2_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_SD3(x)     	HAL_GPIO_WritePin(AD9959_SD3_GPIO_Port, AD9959_SD3_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_RST(x)     	HAL_GPIO_WritePin(AD9959_RST_GPIO_Port, AD9959_RST_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define AD9959_PDC(x)     	HAL_GPIO_WritePin(AD9959_PDC_GPIO_Port, AD9959_PDC_Pin, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/*******************************外部函数声明*******************************************/

/**
 * @brief       AD9959芯片初始化函数
 * @param       无
 * @retval      无
 * @note        执行AD9959芯片的硬件复位和基本初始化
 *              设置所有GPIO引脚的初始状态，完成芯片复位时序
 *              必须在使用其他AD9959功能之前调用此函数
 */
extern void ad9959_init(void);

/**
 * @brief       选择并使能AD9959指定通道
 * @param       ch: 通道号 (0-3)
 *              0=通道0, 1=通道1, 2=通道2, 3=通道3
 * @retval      无
 * @note        AD9959有4个独立的DDS通道，通过CSR寄存器选择要操作的通道
 *              在配置任何通道参数之前必须先调用此函数选择目标通道
 *              每次只能选择一个通道进行操作
 */
extern void ad9959_channel_sel_enable(uint8_t ch);

/**
 * @brief       设置AD9959指定通道输出固定参数信号
 * @param       ch: 输出通道 (0-3)
 * @param       fre: 输出频率，单位Hz (0 ~ 250MHz)
 * @param       phase: 输出相位，单位度 (0-360度)
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置指定通道输出固定频率、相位和幅度的正弦波信号
 *              该函数会自动选择通道、配置寄存器并更新输出
 *              适用于产生稳定的单频信号输出
 */
extern void ad9959_set_signal_out(uint8_t ch, double fre, uint16_t phase, uint16_t amp);

/**
 * @brief       设置AD9959指定通道线性扫频输出
 * @param       ch: 输出通道 (0-3)
 * @param       fre1: 起始频率，单位Hz
 * @param       fre2: 终止频率，单位Hz
 * @param       rdw: 上升步进频率，单位Hz/步
 * @param       fdw: 下降步进频率，单位Hz/步
 * @param       phase: 输出相位，单位度 (0-360度)
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置线性扫频模式，频率在fre1和fre2之间往复扫描
 *              扫频步进速度由rdw和fdw控制，相位和幅度保持固定
 *              扫频完成后会自动反向扫描，实现连续的三角波扫频
 */
extern void ad9959_sweep_frequency(uint8_t ch, double fre1, double fre2, double rdw, double fdw, uint16_t phase, uint16_t amp);

/**
 * @brief       设置AD9959指定通道线性扫相输出
 * @param       ch: 输出通道 (0-3)
 * @param       fre: 输出频率，单位Hz (固定频率)
 * @param       phase1: 起始相位，单位度
 * @param       phase2: 终止相位，单位度
 * @param       rdw: 上升步进相位，单位度/步
 * @param       fdw: 下降步进相位，单位度/步
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置线性扫相模式，相位在phase1和phase2之间往复扫描
 *              频率和幅度保持固定，只改变信号相位
 *              适用于相位调制和相位扫描应用
 */
extern void ad9959_sweep_phase(uint8_t ch, double fre, uint16_t phase1, uint16_t phase2, uint16_t rdw, uint16_t fdw, uint16_t amp);

/**
 * @brief       设置AD9959指定通道线性扫幅输出
 * @param       ch: 输出通道 (0-3)
 * @param       fre: 输出频率，单位Hz (固定频率)
 * @param       phase: 输出相位，单位度 (固定相位)
 * @param       amp1: 起始幅度 (1-1023)
 * @param       amp2: 终止幅度 (1-1023)
 * @param       rdw: 上升步进幅度，单位LSB/步
 * @param       fdw: 下降步进幅度，单位LSB/步
 * @retval      无
 * @note        配置线性扫幅模式，幅度在amp1和amp2之间往复扫描
 *              频率和相位保持固定，只改变信号幅度
 *              适用于幅度调制和功率扫描应用
 */
extern void ad9959_sweep_amplitude(uint8_t ch, double fre, uint16_t phase, uint16_t amp1, uint16_t amp2, uint16_t rdw, uint16_t fdw);



#endif //MYAD9959_H
