//
// Created by 20614 on 25-6-30.
//

#include "myad9959.h"

#include "spi.h"

/**
 ****************************************************************************************************
 * @file        myad9959.c
 * @author      正点原子 & 20614
 * @version     V2.0
 * @date        2025-06-30
 * @brief       AD9959 DDS芯片驱动程序
 *              本驱动支持AD9959四通道直接数字频率合成器的基本功能
 *              包括频率输出、相位控制、幅度控制以及扫频/扫相/扫幅功能
 ****************************************************************************************************
 */

/* AD9959系统时钟频率设定为500MHz，影响频率分辨率和最大输出频率 */
#define AD9959_System_Clk 500000000

/* AD9959延时宏定义 - 便于移植时修改 */
#define AD9959_DELAY(x)   ad9959_delay(x)

/* 延时循环次数 - 根据系统主频调整以获得合适的延时
 * STM32H743 at 480MHz, a value around 50 gives a small delay for SPI.
 * 降低此值会缩短延时，增加此值会延长延时。
 */
#define AD9959_DELAY_LOOP_COUNT 50

/**
 * @brief       AD9959软件延时函数
 * @param       nns: 延时参数，数值越大延时越长
 * @retval      无
 * @note        使用for循环实现延时，便于根据系统主频调整。
 *              通过修改 AD9959_DELAY_LOOP_COUNT 来调整延时长度。
 */
void ad9959_delay(uint32_t nns)
{
	for(; nns != 0; nns--)
	{
		for(volatile uint32_t i = 0; i < AD9959_DELAY_LOOP_COUNT; i++)
		{
			/* This loop creates a delay. The volatile keyword prevents the compiler
			 * from optimizing this loop away. */
			__NOP();
		}
	}
}

/**
 * @brief       AD9959芯片复位和基本初始化
 * @param       无
 * @retval      无
 * @note        设置所有控制信号初始状态，执行硬件复位时序
 */
void ad9959_init(void)
{
	/* 设置SPI控制信号初始状态 */
	AD9959_CS(1);		// 片选信号拉高，SPI未选中状态
	AD9959_CLK(0);		// 时钟信号初始为低电平
	AD9959_UD(0);		// 更新信号初始为低电平

	/* 设置所有串行数据线为低电平 */
	AD9959_SD0(0);		// 串行数据线0
	AD9959_SD1(0);		// 串行数据线1
	AD9959_SD2(0);		// 串行数据线2
	AD9959_SD3(0);		// 串行数据线3

	/* 设置PDC为低电平，关闭功率下降模式 */
	AD9959_PDC(0);

	/* 执行AD9959硬件复位时序 */
 	AD9959_RST(0);		// 复位信号拉低
 	AD9959_DELAY(3);	// 保持复位状态至少3个时钟周期
 	AD9959_RST(1);		// 复位信号拉高，开始复位过程
 	AD9959_DELAY(500);	// 等待复位完成，确保内部电路稳定
 	AD9959_RST(0);		// 复位信号拉低，完成复位时序
}

/**
 * @brief       AD9959数据更新函数
 * @param       无
 * @retval      无
 * @note        向AD9959发送更新脉冲，使之前写入的寄存器数据生效
 *              必须在写入频率、相位、幅度等参数后调用此函数
 */
void IO_update(void)
{
	AD9959_UD(0);		// 确保更新信号为低电平
	AD9959_DELAY(6);	// 延时确保信号稳定
	AD9959_UD(1);		// 更新信号拉高，产生上升沿
	AD9959_DELAY(12);	// 保持高电平，满足更新脉冲宽度要求
	AD9959_UD(0);		// 更新信号拉低，完成更新脉冲
}

/**
 * @brief       向AD9959写入数据
 * @param       reg: 寄存器地址 (0x00-0x09)
 * @param       DataNumber: 要写入的数据字节数
 * @param       Data: 指向要写入数据的指针
 * @retval      无
 * @note        使用SPI协议向AD9959指定寄存器写入数据
 *              先发送8位寄存器地址，再发送指定字节数的数据
 */
void AD9959_WriteData(uint8_t reg, uint8_t DataNumber, uint8_t *Data)
{
	uint8_t	CMD, Value, cnt, i;
	CMD = reg;

	/* 开始SPI通信：时钟拉低，片选拉低 */
	AD9959_CLK(0);
	AD9959_CS(0);		// 选中AD9959芯片

	/* 发送8位寄存器地址 */
	for(i=0; i<8; i++)
	{
		AD9959_CLK(0);						// 时钟拉低，准备数据
		if(0x80 == (CMD & 0x80))			// 检查最高位
			AD9959_SD0(1);					// 发送1
		else
			AD9959_SD0(0);					// 发送0
		AD9959_CLK(1);						// 时钟拉高，AD9959采样数据
		CMD <<= 1;							// 左移1位，准备下一位数据
	}
	AD9959_CLK(0);

	/* 发送数据字节 */
	for (cnt=0; cnt<DataNumber; cnt++)
	{
		Value = Data[cnt];					// 获取当前要发送的字节
		for (i=0; i<8; i++)
		{
			AD9959_CLK(0);					// 时钟拉低，准备数据
			if(0x80 == (Value & 0x80))		// 检查最高位
				AD9959_SD0(1);				// 发送1
			else
				AD9959_SD0(0);				// 发送0
			AD9959_CLK(1);					// 时钟拉高，AD9959采样数据
			Value <<= 1;					// 左移1位，准备下一位数据
		}
		AD9959_CLK(0);
	}

	/* 结束SPI通信：片选拉高 */
	AD9959_CS(1);
}

/**
 * @brief       AD9959 SPI数据写入函数
 * @param       reg: 寄存器地址 (0x00-0x09)
 * @param       DataNumber: 要写入的数据字节数
 * @param       Data: 指向要写入数据的指针
 * @retval      无
 * @note        该函数目前未实现，保留以便未来扩展SPI数据写入功能
 *              目前所有数据写入通过AD9959_WriteData函数完成
 */
void AD9959_WriteData_SPI(uint8_t reg, uint8_t DataNumber, uint8_t *Data)
{
	/* 开始SPI通信：片选拉低 */
	AD9959_CS(0);		// 选中AD9959芯片

	/* 发送8位寄存器地址 */
	HAL_SPI_Transmit(&AD9959_SPI_HANDLE, &reg, 1, HAL_MAX_DELAY);

	/* 发送数据字节 */
	if(DataNumber > 0 && Data != NULL)
	{
		HAL_SPI_Transmit(&AD9959_SPI_HANDLE, Data, DataNumber, HAL_MAX_DELAY);
	}

	/* 结束SPI通信：片选拉高 */
	AD9959_CS(1);
}

/**
 * @brief       AD9959统一数据写入函数
 * @param       reg: 寄存器地址 (0x00-0x09)
 * @param       DataNumber: 要写入的数据字节数
 * @param       Data: 指向要写入数据的指针
 * @retval      无
 * @note        根据编译时宏定义自动选择使用软件SPI或硬件SPI
 *              定义AD9959_USE_HARDWARE_SPI宏则使用硬件SPI，否则使用软件SPI
 *              该函数是对底层SPI通信的统一封装，用户只需调用此函数即可
 */
void AD9959_WriteData_Unified(uint8_t reg, uint8_t DataNumber, uint8_t *Data)
{
#ifdef AD9959_USE_HARDWARE_SPI
	/* 使用硬件SPI模式 */
	AD9959_WriteData_SPI(reg, DataNumber, Data);
#else
	/* 使用软件SPI模式（默认） */
	AD9959_WriteData(reg, DataNumber, Data);
#endif
}

/**
 * @brief       计算频率控制字CFTW0
 * @param       fre: 目标输出频率 (Hz)
 * @param       CFTW0_Data: 指向4字节频率控制字数组的指针
 * @retval      无
 * @note        根据公式 CFTW0 = fre * 2^32 / System_Clock 计算32位频率控制字
 *              System_Clock = 500MHz�����频率分辨率约为0.116Hz
 */
void AD9959_Get_CFTW0_Data(double fre, uint8_t *CFTW0_Data)
{
	double buff;
	uint32_t Value;

	/* 计算频率控制字：CFTW0 = fre * 2^32 / System_Clock */
	buff = 4294967296.0 / AD9959_System_Clk;	// 2^32 / 500MHz
	buff = buff * fre;							// 乘以目标频率
	Value = (uint32_t)buff;						// 转换为32位整数

	/* 将32位数据拆分为4个字节，高字节在前 */
	CFTW0_Data[0] = (uint8_t)(Value>>24);		// 最高字节
	CFTW0_Data[1] = (uint8_t)(Value>>16);		// 次高字节
	CFTW0_Data[2] = (uint8_t)(Value>>8);		// 次低字节
	CFTW0_Data[3] = (uint8_t)Value;				// 最低字节
}

/**
 * @brief       计算相位控制字CPOW0
 * @param       phase: 目标相位角度 (0-360度)
 * @param       CPOW0_Data: 指向2字节相��控制字数组的指针
 * @retval      无
 * @note        根据公式 CPOW0 = phase * 2^14 / 360 计算14位相位控制字
 *              相位分辨率为360/2^14 ≈ 0.022度
 */
void AD9959_Get_CPOW0_Data(int phase, uint8_t *CPOW0_Data)
{
	double buff;
	uint32_t Value;

	/* 计算相位控制字：CPOW0 = phase * 2^14 / 360 */
	buff = 16384.0/360;						// 2^14 / 360度
	buff = phase * buff;					// 乘以目标相位
	Value = (uint32_t)buff;					// 转换为整数

	/* 将14位数据拆分为2个字节，高字节在前 */
	CPOW0_Data[0] = (uint8_t)(Value>>8);	// 高字节(包含高6位)
	CPOW0_Data[1] = (uint8_t)(Value>>0);	// 低字节
}

/**
 * @brief       计算幅度控制字ACR
 * @param       amp: 目标幅度值 (1-1023)
 * @param       ACR_Data: 指向3字节幅度控制字数组的指针
 * @retval      无
 * @note        设置DAC输出幅度，1023�����应最大输出���度
 *              幅度控制为10位，分辨率为1/1024
 */
void AD9959_Get_ACR_Data(uint16_t amp, uint8_t *ACR_Data)
{
	uint16_t Value;

	Value = amp & 0x03FF;					// 限制为10位数据(0-1023)

	/* 幅度控制字格式：���持原有设置，更新幅度位 */
	ACR_Data[0] = ACR_Data[0];				// ���持第一字节不变
	ACR_Data[1] = (ACR_Data[1] | (uint8_t)(Value>>8));	// 更新高2位
	ACR_Data[2] = (uint8_t)(Value>>0);		// 设置低8位
}

/**
 * @brief       选择并使能AD9959通道
 * @param       ch: 通道号 (0-3)
 * @retval      无
 * @note        AD9959有4个独立的DDS通道，通过CSR寄存器选择要操作的通道
 *              0x10=通道0, 0x20=通道1, 0x40=通道2, 0x80=通道3
 */
void ad9959_channel_sel_enable(uint8_t ch)
{
	uint8_t ch_sel[4] = {0x10, 0x20, 0x40, 0x80};	// 各通道选择码
	AD9959_WriteData_Unified(CSR, 1, &ch_sel[ch]);			// 写入通道选择寄存器
}

/**
 * @brief       设置AD9959指定通道输出固定频率信号
 * @param       ch: 输出通道 (0-3)
 * @param       fre: 输出频率 (Hz)
 * @param       phase: 输出相位 (0-360度)
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置指定通道输出固定参数的正弦波信号
 *              该函数会依次配置功能寄存器、幅度、相位、频率，最后更新输出
 */
void ad9959_set_signal_out(uint8_t ch, double fre, uint16_t phase, uint16_t amp)
{
	uint8_t CFTW0_Data[4];					// 频率控制字缓存
	uint8_t CPOW0_Data[2];					// 相位控制字缓存
	uint8_t ACR_Data[3] = {0x00,0x10,0x00};	// 幅度控制寄存器配置
	uint8_t CFR_Data[3] = {0x00,0x23,0x35};	// 通道功能寄存器配置
	uint8_t FR1_Data[3] = {0xD0,0x00,0x00};	// 功能寄存器1配置

	/* ����要操作的通道 */
	ad9959_channel_sel_enable(ch);

	/* 配置功能寄存器，启用单频模式 */
	AD9959_WriteData_Unified(FR1, 3, FR1_Data);		// 配置FR1：启用PLL等
	AD9959_WriteData_Unified(CFR, 3, CFR_Data);		// 配置CFR：单频模式

	/* 设置输出幅度 */
	AD9959_Get_ACR_Data(amp, ACR_Data);		// 计算幅度控制字
	AD9959_WriteData_Unified(ACR, 3, ACR_Data);		// 写入幅度控制寄存器

	/* 设置输出相位 */
	AD9959_Get_CPOW0_Data(phase, CPOW0_Data);	// 计算相位控制字
	AD9959_WriteData_Unified(CPOW0, 2, CPOW0_Data);		// 写入相位控制寄存器

	/* 设置输出频率 */
	AD9959_Get_CFTW0_Data(fre, CFTW0_Data);		// 计算频率控制字
	AD9959_WriteData_Unified(CFTW0, 4, CFTW0_Data);		// 写入频率控制寄存器

	/* 更新输出，使所有设置生效 */
	IO_update();
}

/**
 * @brief       AD9959��性扫频功能
 * @param       ch: 输出通道 (0-3)
 * @param       fre1: 起始频率 (Hz)
 * @param       fre2: 终止频率 (Hz)
 * @param       rdw: 上升步进频率 (Hz/步)
 * @param       fdw: 下降步进频率 (Hz/步)
 * @param       phase: 输出相位 (0-360度)
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置线性扫频模式，频率在fre1和fre2之间往复扫描
 *              扫频速度由rdw和fdw控制，SRR寄存器控制扫频斜率
 */
void ad9959_sweep_frequency(uint8_t ch, double fre1, double fre2, double rdw, double fdw, uint16_t phase, uint16_t amp)
{
	uint8_t RDW_Data[4];						// 上升步进频���控制字缓存
	uint8_t FDW_Data[4];						// 下降步进频率控制字缓存
	uint8_t CFTW0_Data[4];						// 频率控制字缓存
	uint8_t CPOW0_Data[2];						// 相位控制字缓存
	uint8_t SRR_Data[2] = {0xFF,0xFF};			// 扫描���率寄存器：最快扫描速度
	uint8_t ACR_Data[3] = {0x00,0x10,0x00};		// 幅度控制寄存器配置
	uint8_t CFR_Data[3] = {0x82, 0x43, 0x30};   // 通道功能寄存器：启用线性扫频模式  //科一电子为{0x80,0x43,0x20} 此处进行了修改;
	uint8_t FR1_Data[3] = {0xD0,0x00,0x00};		// 功能寄存器1配置

	/* 选择要操作的通道 */
	ad9959_channel_sel_enable(ch);

	/* 配置功能寄存器，启用扫频模式 */
	AD9959_WriteData_Unified(FR1, 3, FR1_Data);			// 配置FR1寄存器
	AD9959_WriteData_Unified(CFR, 3, CFR_Data);			// 配置CFR：启用线性扫频

	/* 设置起始频率 */
	AD9959_Get_CFTW0_Data(fre1, CFTW0_Data);	// 计算起始频率控制字
	AD9959_WriteData_Unified(CFTW0, 4, CFTW0_Data);		// 写入CFTW0寄存器

	/* 设置终止频率 */
	AD9959_Get_CFTW0_Data(fre2, CFTW0_Data);	// 计算终止频率控制字
	AD9959_WriteData_Unified(0x0A, 4, CFTW0_Data);		// 写入CFTW1寄存器(0x0A)

	/* 设置上升步进频率 */
	AD9959_Get_CFTW0_Data(rdw, RDW_Data);		// 计算上升步进控制字
	AD9959_WriteData_Unified(RDW, 4, RDW_Data);			// 写���RDW寄存器

	/* 设置下降步进频率 */
	AD9959_Get_CFTW0_Data(fdw, FDW_Data);		// 计���下降步进控制字
	AD9959_WriteData_Unified(FDW, 4, FDW_Data);			// 写入FDW寄存器

	/* 设置扫描斜率时间 */
	AD9959_WriteData_Unified(SRR, 2, SRR_Data);			// 写入扫描斜率寄存器

	/* 设置输出幅度 */
	AD9959_Get_ACR_Data(amp, ACR_Data);			// 计算幅度控制字
	AD9959_WriteData_Unified(ACR, 3, ACR_Data);			// 写入幅度控制寄存器

	/* 设置输出相位 */
	AD9959_Get_CPOW0_Data(phase, CPOW0_Data);	// 计算相位控制字
	AD9959_WriteData_Unified(CPOW0, 2, CPOW0_Data);		// 写入相位控制寄存器

	/* 更新输出，启动扫频 */
	IO_update();
}

/**
 * @brief       AD9959线性扫相功能
 * @param       ch: 输出��道 (0-3)
 * @param       fre: 输出频率 (Hz)
 * @param       phase1: 起始相位 (0-360度)
 * @param       phase2: 终止相�� (0-360度)
 * @param       rdw: 上升步进相��� (度/步)
 * @param       fdw: 下降步进相位 (度/步)
 * @param       amp: 输出幅度 (1-1023)
 * @retval      无
 * @note        配置线性扫相模式，相位在phase1和phase2之间往复扫描
 *              频率保持固定，只改变信号相位
 */
void ad9959_sweep_phase(uint8_t ch, double fre, uint16_t phase1, uint16_t phase2, uint16_t rdw, uint16_t fdw, uint16_t amp)
{
	uint8_t RDW_Data[4] = {0x00,0x00,0x00,0x00};	// 上升步进相位控制字缓存
	uint8_t FDW_Data[4] = {0x00,0x00,0x00,0x00};	// 下降步进相位控制字��存
	uint8_t CFTW0_Data[4] = {0x00,0x00,0x00,0x00};	// 频率控制字缓存
	uint8_t CPOW0_Data[4] = {0x00,0x00,0x00,0x00};	// 相位控制字缓存
	uint8_t SRR_Data[2] = {0xFF,0xFF};				// 扫描斜率寄存器
	uint8_t ACR_Data[3] = {0x00,0x10,0x00};			// 幅度控制寄存器配置
	uint8_t CFR_Data[3] = {0xc0,0xC3,0x30};			// 通道功能寄存器：启用线性扫相模式
	uint8_t FR1_Data[3] = {0xD0,0x00,0x00};			// 功能寄存器1配置

	/* 选择要操作的通道 */
	ad9959_channel_sel_enable(ch);

	/* 配置功能寄存器，启用扫相模��� */
	AD9959_WriteData_Unified(FR1, 3, FR1_Data);				// 配置FR1寄存器
	AD9959_WriteData_Unified(CFR, 3, CFR_Data);				// 配置CFR：启用线性扫相

	/* 设置固定输出频率 */
	AD9959_Get_CFTW0_Data(fre, CFTW0_Data);			// 计算频率控制字
	AD9959_WriteData_Unified(CFTW0, 4, CFTW0_Data);			// 写入频率寄存器

	/* 设置起始相位 */
	AD9959_Get_CPOW0_Data(phase1, CPOW0_Data);		// 计算起始相位控制字
	AD9959_WriteData_Unified(CPOW0, 2, CFTW0_Data);			// 写入CPOW0寄存器

	/* 设置终止相位 */
	AD9959_Get_CPOW0_Data(phase2, CPOW0_Data);		// 计算终止相位控制字
	AD9959_WriteData_Unified(0x0A, 4, CFTW0_Data);			// 写入CPOW1寄存器

	/* 设置上升步进相位 */
	AD9959_Get_CPOW0_Data(rdw, RDW_Data);			// 计算上升步进控制字
	AD9959_WriteData_Unified(RDW, 4, RDW_Data);				// 写入RDW寄存器

	/* 设置下降步进相位 */
	AD9959_Get_CPOW0_Data(fdw, FDW_Data);			// 计���下降步进控制字
	AD9959_WriteData_Unified(FDW, 4, FDW_Data);				// 写入FDW寄存器

	/* ��置扫描斜率时间 */
	AD9959_WriteData_Unified(SRR, 2, SRR_Data);				// 写入扫描斜率������器

	/* 设置输出幅度 */
	AD9959_Get_ACR_Data(amp, ACR_Data);				// 计算幅度控制字
	AD9959_WriteData_Unified(ACR, 3, ACR_Data);				// 写入幅度控制寄存器

	/* 更新输出，启�����相 */
	IO_update();
}

/**
 * @brief       计算幅度扫描控制字
 * @param       amp: 目标幅度值 (1-1023)
 * @param       Amp_Data: ���向4字节幅度扫描控制字数组的指针
 * @retval      无
 * @note        用于扫幅功能的幅度控制字计算，格式与ACR寄存器不同
 *              将10位幅度值转换为扫描模式专用的数据格式
 */
void AD9959_Get_Amp_Data(uint16_t amp, uint8_t *Amp_Data)
{
	uint16_t Value;

	Value = amp & 0x03FF;					// 限制为10位数据(0-1023)

	/* 扫幅模式专�����的幅度控制字格式 */
	Amp_Data[0] = (uint8_t)(Value >> 2);	// 高8位数据
	Amp_Data[1] = (uint8_t)(Value << 6);	// 低2位数据左移到高位
	Amp_Data[2] = 0x00;						// 填充0
	Amp_Data[3] = 0x00;						// 填充0
}

/**
 * @brief       AD9959线性扫幅功能
 * @param       ch: 输出通道 (0-3)
 * @param       fre: 输出频率 (Hz)
 * @param       phase: 输出相位 (0-360度)
 * @param       amp1: 起始幅度 (1-1023)
 * @param       amp2: 终止幅度 (1-1023)
 * @param       rdw: 上升步进幅度 (LSB/步)
 * @param       fdw: 下降步进幅度 (LSB/��)
 * @retval      无
 * @note        配置线性扫���模式，幅度在amp1和amp2之间往复扫描
 *              频率和相位保持固定，只改变信号幅度
 */
void ad9959_sweep_amplitude(uint8_t ch, double fre, uint16_t phase, uint16_t amp1, uint16_t amp2, uint16_t rdw, uint16_t fdw)
{
	uint8_t Amp_Data[4];						// 幅度控制字缓存
	uint8_t RDW_Data[4];						// 上升步进幅度控制字缓存
	uint8_t FDW_Data[4];						// 下降步进幅度控制字缓存
	uint8_t CFTW0_Data[4];						// 频率控制字缓存
	uint8_t CPOW0_Data[2];						// 相位控制字缓存
	uint8_t SRR_Data[2] = {0xFF,0xFF};			// 扫描斜率寄存器
	uint8_t ACR_Data[3] = {0x00,0x00,0x00};		// 幅度控制寄存器配置
	uint8_t CFR_Data[3] = {0x40,0x43,0x20};		// 通道功能��存器��启用��性��幅模式
	uint8_t FR1_Data[3] = {0xD0,0x00,0x00};		// 功能寄存器1配置

	/* 选择要操作的通道 */
	ad9959_channel_sel_enable(ch);

	/* 配置功能寄存器，启用扫幅模式 */
	AD9959_WriteData_Unified(FR1, 3, FR1_Data);			// 配置FR1寄存器
	AD9959_WriteData_Unified(CFR, 3, CFR_Data);			// 配置CFR：启用线性扫幅

	/* 设置起始幅度 */
	AD9959_Get_ACR_Data(amp1, ACR_Data);		// 计算起始幅度控制字
	AD9959_WriteData_Unified(ACR, 3, ACR_Data);			// 写入ACR寄存器

	/* 设置终止幅度 */
	AD9959_Get_Amp_Data(amp2, Amp_Data);		// 计算终止幅���控制字
	AD9959_WriteData_Unified(0x0A, 4, Amp_Data);		// 写入ASF1寄存器

	/* 设置上升步进幅度 */
	AD9959_Get_Amp_Data(rdw, RDW_Data);			// 计算上升步进控制字
	AD9959_WriteData_Unified(RDW, 4, RDW_Data);			// 写入RDW寄存器

	/* 设置下降步进幅度 */
	AD9959_Get_Amp_Data(fdw, FDW_Data);			// 计算下降步进控制字
	AD9959_WriteData_Unified(FDW, 4, FDW_Data);			// 写入FDW寄存器

	/* 设置扫描斜率时间 */
	AD9959_WriteData_Unified(SRR, 2, SRR_Data);			// 写入扫描斜率寄存器

	/* 设置固定输出相位 */
	AD9959_Get_CPOW0_Data(phase, CPOW0_Data);	// 计算相位控制字
	AD9959_WriteData_Unified(CPOW0, 2, CFTW0_Data);		// 写入相位寄存器

	/* 设置固定��出频率 */
	AD9959_Get_CFTW0_Data(fre, CFTW0_Data);		// 计算频率控制字
	AD9959_WriteData_Unified(CFTW0, 4, CFTW0_Data);		// 写入频���寄存器

	/* 更新输出，启动扫幅 */
	IO_update();
}
