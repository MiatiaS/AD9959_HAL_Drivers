
AD9959 DDS驱动库使用说明
概述
本驱动库为AD9959四通道直接数字频率合成器(DDS)提供完整的驱动支持，适用于STM32H7系列微控制器。AD9959是一款高性能的四通道DDS芯片，能够生成高精度的正弦波信号，并支持频率、相位、幅度的独立控制以及各种扫描功能。
硬件连接
引脚连接对照表
STM32引脚
AD9959引脚
功能说明
CubeMX标签名
PA9
CS
片选信号(低电平有效)
AD9959_CS
PA8
I/O UPDATE
数据更新信号
AD9959_UD
PC9
SCLK
SPI时钟信号
AD9959_CLK
PC8
SDIO_0
串行数据输入线0
AD9959_SD0
PC7
SDIO_1
串行数据输入线1
AD9959_SD1
PC6
SDIO_2
串行数据输入线2
AD9959_SD2
PD15
SDIO_3
串行数据输入线3
AD9959_SD3
PD14
RESET
硬件复位信号(低电平有效)
AD9959_RST
PD13
PWR DWN CTL
功率下降控制(低电平有效)
AD9959_PDC
硬件注意事项
GPIO配置: 所有引脚都应配置为推挽输出模式
电源连接: AD9959的VCC连接到3.3V电源
信号完整性: 建议在每个信号线上串联22Ω电阻以减少信号反射
接地要求: AD9959的AGND和DGND需要良好接地
时钟设置: 外部晶振25MHz，通过内部PLL倍频到500MHz系统时钟
软件配置
1. 文件包含
#include "myad9959.h"
2. 基本初始化
// 在main函数中初始化
int main(void)
{
    // ... HAL库初始化和时钟配置 ...
    
    MX_GPIO_Init();     // GPIO初始化
    ad9959_init();      // AD9959芯片初始化
    
    // ... 其他代码 ...
}
API函数说明
1. 初始化函数
void ad9959_init(void);
功能: 初始化AD9959芯片
参数: 无
返回: 无
说明: 必须在使用其他功能前调用
2. 通道选择函数
void ad9959_channel_sel_enable(uint8_t ch);
功能: 选择并使能指定通道
参数: ch - 通道号(0-3)
返回: 无
说明: 可选调用，高级函数会自动选择通道
3. 固定信号输出
void ad9959_set_signal_out(uint8_t ch, double fre, uint16_t phase, uint16_t amp);
功能: 设置指定通道输出固定参数的正弦波信号
参数:
ch: 通道号(0-3)
fre: 输出频率，单位Hz (0 ~ 250MHz)
phase: 输出相位，单位度 (0-360)
amp: 输出幅度 (1-1023)
返回: 无
4. 扫频功能
void ad9959_sweep_frequency(uint8_t ch, double fre1, double fre2, double rdw, double fdw, uint16_t phase, uint16_t amp);
功能: 设置线性扫频输出
参数:
ch: 通道号(0-3)
fre1: 起始频率，单位Hz
fre2: 终止频率，单位Hz
rdw: 上升步进频率，单位Hz/步
fdw: 下降步进频率，单位Hz/步
phase: 固定相位，单位度
amp: 固定幅度 (1-1023)
5. 扫相功能
void ad9959_sweep_phase(uint8_t ch, double fre, uint16_t phase1, uint16_t phase2, uint16_t rdw, uint16_t fdw, uint16_t amp);
功能: 设置线性扫相输出
参数:
ch: 通道号(0-3)
fre: 固定频率，单位Hz
phase1: 起始相位，单位度
phase2: 终止相位，单位度
rdw: 上升步进相位，单位度/步
fdw: 下降步进相位，单位度/步
amp: 固定幅度 (1-1023)
6. 扫幅功能
void ad9959_sweep_amplitude(uint8_t ch, double fre, uint16_t phase, uint16_t amp1, uint16_t amp2, uint16_t rdw, uint16_t fdw);
功能: 设置线性扫幅输出
参数:
ch: 通道号(0-3)
fre: 固定频率，单位Hz
phase: 固定相位，单位度
amp1: 起始幅度 (1-1023)
amp2: 终止幅度 (1-1023)
rdw: 上升步进幅度，单位LSB/步
fdw: 下降步进幅度，单位LSB/步
使用示例
示例1: 四通道固定频率输出
int main(void)
{
    // 系统初始化...
    MX_GPIO_Init();
    ad9959_init();
    
    // 配置4个通道输出不同频率的信号
    ad9959_set_signal_out(0, 1000000.0, 0, 512);      // 通道0: 1MHz, 0°, 中等幅度
    HAL_Delay(10);
    
    ad9959_set_signal_out(1, 2000000.0, 90, 768);     // 通道1: 2MHz, 90°, 较高幅度
    HAL_Delay(10);
    
    ad9959_set_signal_out(2, 5000000.0, 180, 256);    // 通道2: 5MHz, 180°, 较低幅度
    HAL_Delay(10);
    
    ad9959_set_signal_out(3, 10000000.0, 270, 1023);  // 通道3: 10MHz, 270°, 最大幅度
    HAL_Delay(10);
    
    while(1)
    {
        HAL_Delay(1000);  // 主循环
    }
}
示例2: 扫频功能
// 通道0进行1MHz到10MHz的扫频
ad9959_sweep_frequency(0, 1000000.0, 10000000.0, 1000.0, 1000.0, 0, 512);
示例3: 扫相功能
// 通道1进行0度到360度的扫相，频率固定在5MHz
ad9959_sweep_phase(1, 5000000.0, 0, 360, 1, 1, 512);
示例4: 扫幅功能
// 通道2进行幅度扫描，从最小到最大幅度
ad9959_sweep_amplitude(2, 1000000.0, 0, 100, 1000, 1, 1);
技术参数
系统时钟: 500MHz (25MHz外部晶振 × 20倍频)
频率范围: 0 ~ 250MHz
频率分辨率: 约0.116Hz
相位范围: 0 ~ 360度
相位分辨率: 约0.022度
幅度范围: 1 ~ 1023 (10位精度)
通道数: 4个独立通道
常见问题
Q1: 输出频率不准确
A: 检查外部晶振频率是否为25MHz，确认AD9959_System_Clk宏定义为500000000。
Q2: 无信号输出
A:
检查硬件连接是否正确
确认GPIO配置为推挽输出模式
检查电源和接地连接
确认调用了ad9959_init()函数
Q3: 扫频不工作
A:
检查起始和终止频率设置是否合理
确认步进频率不为0
检查SRR寄存器配置
Q4: 编译警告
A: 文件中的编译警告不影响功能，可以正常使用。如需消除警告，可以检查头文件包含和函数参数类型匹配。
移植说明
如需移植到其他STM32系列或修改引脚定义：
修改myad9959.h中的引脚宏定义
在CubeMX中重新配置GPIO引脚
根据需要调整AD9959_DELAY_LOOP_COUNT延时参数
确认系统时钟频率并更新AD9959_System_Clk宏定义
版本信息
版本: V2.0
日期: 2025-06-30
作者: 科一电子 & 20614
适用芯片: STM32H7系列
测试平台: STM32H743VIT6
