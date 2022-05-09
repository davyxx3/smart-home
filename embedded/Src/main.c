#include "main.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "adxl362.h"

#define EQPTYPE_STR "BU01"		 //设备型号
#define EQPTNAME_STR "DEVBU0007" //设备名称
#define EQPKEY_STR "1234567"	 //访问KEY

#define DATA_UPDATEGAP_S 1800 //数据超时时间
#define DATA_SAMPGAP_H 0	  //两次扫描间睡眠时长
#define DATA_SAMPGAP_M 30	  //两次扫描间睡眠时长
#define DATA_SAMPGAP_S 00	  //两次扫描间睡眠时长

#define DTU_PORT huart4		// M5310A 模块用的串口名称
#define LOG_PORT huart2		//日志输出串口名称
#define RS485_1_PORT huart1 //线路1串口名称
#define RS485_2_PORT huart5 //线路2串口名称

#define CIRCLE_GAP_MS 50 //主循环运行周期

#define MDB_AUTO_TABLESIZE 5 // MODBUS 收发表大小
#define MDB_OVERTIME_MS 1000 // MODBUS 超时时间

// 日志输出缓冲区域
typedef struct __LogTxRingBufTypeDef
{
	uint16_t lastTxCnt;
	uint16_t remainCnt;
	char data[512];
} LogTxRingBufTypeDef;

// 流程和步骤控制
struct SEQSTATE
{
	uint8_t currentSeqNo;	// Current Sequency No
	uint8_t maxSeqNo;		// Max number of sequency
	uint8_t currentStep;	// Current step No ,Edited by Seq_ Function
	uint8_t maxStep;		// Max step of current sequency ,Edited by Seq_ Function
	uint32_t maxOverTimeMs; // Step overtime limiter
	uint32_t passedTimeMs;	// Passed time (ms)
};

// 采集的传感器数据
struct SENSOR_DATA
{
	float pitch; //传感器俯仰角
	float roll;	 //传感器横滚角
	float yaw;	 //传感器偏航角

	int16_t gravity1; //加速度1
	int16_t gravity2; //加速度2
	int16_t gravity3; //加速度3

	uint16_t adcVal1; // ADC sample value
	uint16_t adcVal2; // ADC sample value
	uint16_t adcVal3; // ADC sample value

	float hyetometerVal;	 //雨量计降水量(mm)
	float BatVoltage;		 //电池电压
	float WaterSurfaceRange; //超声波测距获得的水面距离

	float temperture;	 // 温度
	float pH;			 // pH
	float oxgen;		 // 含氧量
	float turbidity;	 // 浊度
	float ammon_ion_con; // Ammonium ion concentration
};

// AT指令队列
struct AT_BUFFER
{
	char txData[256];
	uint16_t txLength;
	char rxData[128];
	uint16_t rxLength;
};
/*JSON 数据缓存区域*/
struct JSON_DATA
{
	char data[384];
	int length;
};

/*MODBUS 读取状态*/
enum MDB_AUTO_STATE
{
	MDB_Lost,
	Mdb_Reading,
	Mdb_Empty
};

/*MODBUS PDU 定义*/
typedef struct __ModbusPduTypeDef
{
	uint8_t enabled_Line;	   // 设备所在线路
	uint8_t slaveID;		   // 设备ID
	enum MDB_AUTO_STATE state; // 设备状态
	uint16_t *destPtr;		   // 数据存放的指针
} ModbusPduTypeDef;

/*MODBUS 收发内容定义*/
struct MDB_AUTO_CTABLE
{
	uint8_t currentLooper;
	uint8_t retryCounter;
	uint16_t timeCounter;
	struct __ModbusPduTypeDef table[MDB_AUTO_TABLESIZE];
};

/*MODBUS 收发队列缓冲区域*/
struct MDB_BUFFER
{
	char txData[64];
	uint16_t txLength;
	char rxData[128];
	uint16_t rxLength;
};

/*用于float和uint8 类型互转的类�??*/
union f32_u8
{
	float f32;
	uint8_t u8[4];
};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART4_UART_Init(void);
static void MX_USART5_UART_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM6_Init(void);
static void MX_CRC_Init(void);

void Init_Log();
int16_t Log(const char *str);
void Driver_Log();
void Proc_Init(void);
void Act_ResetProc(void);
void Proc_SampData(void);
void Proc_MDBread(void);
void Proc_GenerateJson(void);
void Proc_HttpPost(void);
void Seq_EnterPSM(void);
void Proc_SysStop(void);
void Proc_CorrectRtc(void);

void M5310A_RestartRx(void);
void Init_SampData();
void Proc_ConfigPsm();
void MDB_ResetControl();
void Init_MDB_AUTO();

ADC_HandleTypeDef hadc;

CRC_HandleTypeDef hcrc;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart4_rx;
DMA_HandleTypeDef hdma_usart4_tx;
DMA_HandleTypeDef hdma_usart5_rx;

LogTxRingBufTypeDef g_LogTxBuf;
struct SEQSTATE g_ProcState;
struct SENSOR_DATA g_SensorData;
struct AT_BUFFER g_AtBuffer;
struct MDB_AUTO_CTABLE g_Mdb_AutoReg;
struct MDB_BUFFER g_Mdb_Buffer;
struct JSON_DATA g_JsonData;
char PostBuffer[512];

const char AT_CheckStr[] = "AT\r\n";
const char AT_CheckReturnStr[] = "OK\r\n";
const char AT_PingStr[] = "AT+NPING=www.baidu.com\r\n";
const char AT_PintReturnStr[] = "+NPING:";

const char AT_HttpCreateStr[] =
	"AT+HTTPCREATE=\"http://81.69.26.72:8400/\"\r\n";
const char AT_HttpCreateReturnStr[] = "+HTTPCREATE:";
const char AT_HttpHeaderStr[] =
	"AT+HTTPHEADER=0,\"User-Agent: Unknown\\r\\n\"\r\n"; // 39 Bytes
const char AT_HttpHeaderReturnStr[] = "OK\r\n";
const char AT_HttpContentStr[256] = "AT+HTTPCONTENT=0,\"TestConternt\"\r\n";
const char AT_HttpContentReturnStr[] = "OK\r\n";
const char AT_HttpSendStr[] = "AT+HTTPSEND=0,1,\"/upload\"\r\n"; // 19 Bytes
const char AT_HttpSendReturnStr[] = "CONNECT OK";
const char AT_HttpCloseStr[] = "AT+HTTPCLOSE=0\r\n";
const char AT_HttpCloseReturnStr[] = "OK";

/// M5310A PSM mode configuration string.
const char AT_CfgPsmStr[] = "AT+CEREG=4\r\n";
const char AT_CfgPsmReturnStr[] = "OK\r\n";

const char AT_LocalTimeStr[] = "AT+CCLK?\r\n";
const char AT_LocalTimeReturnStr[] = "+CCLK";

// 查询帧报
uint8_t MDB_TX_ARRAY[MDB_AUTO_TABLESIZE][8] = {

	{0x07, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xAC},
	{0x07, 0x03, 0x00, 0x04, 0x00, 0x01, 0xC5, 0xAD},
	{0x07, 0x03, 0x00, 0x04, 0x00, 0x01, 0xC5, 0xAD},
	{0x07, 0x03, 0x00, 0x04, 0x00, 0x01, 0xC5, 0xAD},
	{0x07, 0x03, 0x00, 0x04, 0x00, 0x01, 0xC5, 0xAD}};

// 响应帧报
uint8_t MDB_RX_ARRAY[MDB_AUTO_TABLESIZE][3] = {

	{0x7, 0x3, 0x2},
	{0x7, 0x3, 0x2},
	{0x7, 0x3, 0x2},
	{0x7, 0x3, 0x2},
	{0x7, 0x3, 0x2}};

void Init_Log()
{
	g_LogTxBuf.remainCnt = 0;
	g_LogTxBuf.lastTxCnt = 0;
}

int16_t Log(const char *str)
{
	uint16_t _destCharCnt = 0;
	RTC_TimeTypeDef t_Time;
	RTC_DateTypeDef t_Date;
	HAL_RTC_GetTime(&hrtc, &t_Time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &t_Date, RTC_FORMAT_BIN);
	_destCharCnt = sprintf(g_LogTxBuf.data + g_LogTxBuf.remainCnt,
						   "[%.2d:%.2d:%.2d]:%s\r\n", t_Time.Hours, t_Time.Minutes,
						   t_Time.Seconds, str);

	if ((g_LogTxBuf.remainCnt + _destCharCnt) > 512)
		return -1;

	g_LogTxBuf.remainCnt += _destCharCnt;
	return 0;
}

void Driver_Log()
{
	if (g_LogTxBuf.lastTxCnt != 0)
	{
		if ((HAL_UART_GetState(&LOG_PORT) & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX)
		{
			// Last transmit finished
			memcpy(g_LogTxBuf.data, g_LogTxBuf.data + g_LogTxBuf.lastTxCnt,
				   g_LogTxBuf.remainCnt - g_LogTxBuf.lastTxCnt);
			g_LogTxBuf.remainCnt -= g_LogTxBuf.lastTxCnt;
			g_LogTxBuf.lastTxCnt = 0;
		}
	}
	if (g_LogTxBuf.remainCnt > 0)
	{
		if ((HAL_UART_GetState(&LOG_PORT) & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX)
		{
			HAL_UART_Transmit_DMA(&LOG_PORT, (uint8_t *)g_LogTxBuf.data,
								  g_LogTxBuf.remainCnt);
			g_LogTxBuf.lastTxCnt = g_LogTxBuf.remainCnt;
		}
	}
}

/*�??2位字符数字转换为BCD编码数字*/
uint8_t Convert2CharToBCD(char *str)
{
	uint8_t t_High = *str;
	uint8_t t_Low = *(str + 1);
	uint8_t t_ReturnVal;
	if ((t_High > 57) || (t_High < 48))
		return 0;
	if ((t_Low > 57) || (t_Low < 48))
		return 0;
	t_ReturnVal = ((t_Low - 48) & 0xF) | (((t_High - 48) & 0xF) << 4);
	return t_ReturnVal;
}

/*根据传入的字符串（M5310A应答）设置本地时�??*/
void CorrectRtcTime(char *str)
{
	// Decode data from begin positon
	//+CCLK:19/06/26,12:45:05+32
	// YY:6-7 MM:9-10 DD:12-13
	// hh:15-16  mm:18-19  ss: 21-22
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef DateToUpdate = {0};

	DateToUpdate.Year = Convert2CharToBCD(str + 6);
	DateToUpdate.Month = Convert2CharToBCD(str + 7);
	DateToUpdate.Date = Convert2CharToBCD(str + 12);

	sTime.Hours = Convert2CharToBCD(str + 15);
	sTime.Minutes = Convert2CharToBCD(str + 18);
	sTime.Seconds = Convert2CharToBCD(str + 21);

	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD);
	HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
	// GPIO_InitTypeDef GPIO_InitStruct = {0};
	HAL_RTCEx_DeactivateWakeUpTimer(hrtc);

	// HAL_Delay(100);
	Log("Sys Stop<1>:Exit stop mode.******");
}

/**
 * @brief Hardware service loop (10 ms)
 */
void HardwareLoop10ms()
{
	Driver_Log();
}

/**
 * @brief Hardware service init
 */
void HardwareInit()
{
	//初始化全�??变量
	Init_SampData();
	Proc_Init();
	Act_ResetProc();
	Init_MDB_AUTO();
	MDB_ResetControl();

	HAL_ADC_Stop(&hadc);

	M5310A_RestartRx();
	Log("--------------------------------------");
	Log("System initialized.");
	Log("ADC stop.");
	sprintf(PostBuffer, "Device Type %s  ,Device Name %s",
			EQPTYPE_STR, EQPTNAME_STR);
	HAL_GPIO_WritePin(BRESET_GPIO_Port, BRESET_Pin, GPIO_PIN_SET);
	Log(PostBuffer);
	HAL_Delay(10000);
	Init_ADXL362();
	//网络校时
	while (1)
	{
		Proc_CorrectRtc();
		HAL_Delay(1000);
		if (g_ProcState.currentStep >= g_ProcState.maxStep)
		{
			Log("Finished time correct sequency.");
			break;
		}
		if (g_ProcState.passedTimeMs >= 8000)
		{
			Log("Time correction overtime.");
			break;
		}
		HAL_Delay(1000);
		g_ProcState.passedTimeMs += 2000;
	}
	Act_ResetProc();
	g_ProcState.currentSeqNo = 0;
	//配置NB-IOT模块 M5310A的PSM（节能）模式
	while (1)
	{
		Proc_ConfigPsm();
		HAL_Delay(500);
		if (g_ProcState.currentStep >= g_ProcState.maxStep)
		{
			Log("Finished PSM config sequency.");
			break;
		}
		if (g_ProcState.passedTimeMs >= 8000)
		{
			Log("PSM config overtime.");
			break;
		}
		HAL_Delay(500);
		g_ProcState.passedTimeMs += 1000;
	}
	//关闭水质传感器的两路电源
	HAL_GPIO_WritePin(PWR1_EN_GPIO_Port, PWR1_EN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PWR2_EN_GPIO_Port, PWR2_EN_Pin, GPIO_PIN_RESET);
	Log("Turn off power 1+2.");

	Act_ResetProc();
	g_ProcState.currentSeqNo = 0;
}

/*主要循环*/
void HardwareLowlevelLoop()
{
	char _tmpstr[128];
	HAL_Delay(CIRCLE_GAP_MS / 2);
	// 顺序选择
	switch (g_ProcState.currentSeqNo)
	{
	case 0:
		// 采集电池电压和角度等信息
		Proc_SampData();
		break;
	case 1:
		// 读取传感器采集的数据
		Proc_MDBread();
		break;
	case 2:
		// 生成JSON报文
		Proc_GenerateJson();
		break;
	case 3:
		// 上发数据到服务器
		Proc_HttpPost();
		break;
	case 4:
		// 休眠
		HAL_Delay(100);
		Proc_SysStop();
		break;
	default:
		break;
	}

	HAL_Delay(CIRCLE_GAP_MS / 2);

	// 判断下次循环时应该进行的流程
	if (g_ProcState.currentStep >= g_ProcState.maxStep)
	{
		// 当前流程的所有步骤都已执行完，流程复位
		Act_ResetProc();
		if (g_ProcState.currentSeqNo >= 4)
		{
			// 流程4（休眠）已运行完，从流程0重新开始
			g_ProcState.currentSeqNo = 0;
		}
		else if (g_ProcState.currentSeqNo == 2)
		{
			//从调试口输出传感器数据
			sprintf(_tmpstr, "Sensor Data:\r\n"
							 "Sen\tX\tY\tZ\tRo\tYa\tPi\tHy\tBa\r\n"
							 "Val\t%4d\t%4d\t%4d\t%3.3f\t%3.3f\t%3.3f\t%5.1f\t%3.3f",
					g_SensorData.gravity1, g_SensorData.gravity2,
					g_SensorData.gravity3, g_SensorData.roll,
					g_SensorData.yaw, g_SensorData.pitch,
					g_SensorData.hyetometerVal, g_SensorData.BatVoltage);
			Log(_tmpstr);
			// 进入下一个流程
			g_ProcState.currentSeqNo++;
		}
	}

	// 判断流程是否超时
	if (g_ProcState.passedTimeMs < g_ProcState.maxOverTimeMs)
	{
		// 没有超时，则记录时间
		g_ProcState.passedTimeMs += CIRCLE_GAP_MS;
	}
	else
	{
		// 流程超时，则直接进入Sleep
		HAL_Delay(200);
		Log("Error:Sequency step overtime.");
		Act_ResetProc();
		g_ProcState.currentSeqNo = 4;
	}
}

/*
 * @brief: initialize all process
 * 初始化流�??
 *
 */
void Proc_Init()
{
	g_ProcState.currentStep = 0;
	g_ProcState.maxStep = 0;
	g_ProcState.currentSeqNo = 0;
	g_ProcState.maxSeqNo = 5;
	Act_ResetProc();
}

/*
 * @brief Reset process state
 *  复位流程超时、计�??
 */
void Act_ResetProc()
{
	g_ProcState.currentStep = 0;
	g_ProcState.passedTimeMs = 0;
	g_ProcState.maxStep = 0;
	g_ProcState.passedTimeMs = 0;
	g_ProcState.maxOverTimeMs = 20000;
}

/*ADC采样，以便获得电池电压等信息*/
void Act_SampleAdc()
{
	char tempStr[64];
	ADC_ChannelConfTypeDef sConfig;

	sConfig.Channel = ADC_CHANNEL_5;
	sConfig.Rank = 0;
	HAL_ADC_ConfigChannel(&hadc, &sConfig);
	HAL_ADC_Start(&hadc);
	HAL_Delay(10);
	g_SensorData.adcVal1 = HAL_ADC_GetValue(&hadc);
	HAL_ADC_Stop(&hadc);
	// Convert battery voltage
	// Volt=adcval*3.30/4095*(22/122)=adcval*0.004095f
	g_SensorData.BatVoltage = g_SensorData.adcVal1 * 0.004095f;
	HAL_Delay(1);

	g_SensorData.WaterSurfaceRange = 0;

	sprintf(tempStr, "ADC samp value:%d,%d,%d WaterSurface=%3.2f",
			g_SensorData.adcVal1, g_SensorData.adcVal2, g_SensorData.adcVal3,
			g_SensorData.WaterSurfaceRange);
	Log(tempStr);
}

void Init_SampData()
{
	g_SensorData.adcVal1 = 0;
	g_SensorData.adcVal2 = 0;
	g_SensorData.adcVal3 = 0;
	g_SensorData.BatVoltage = 0;
	g_SensorData.pH = 0;
	g_SensorData.temperture = 0;
	g_SensorData.oxgen = 0;
	g_SensorData.yaw = 0;
	g_SensorData.roll = 0;
	g_SensorData.pitch = 0;
	g_SensorData.WaterSurfaceRange = 0.00f;
}

/*本机数据采样流程*/
void Proc_SampData()
{
	g_ProcState.maxStep = 4;
	g_ProcState.maxOverTimeMs = 1000;
	// [0]打开加速度计电源（因加速度计低功耗，此步忽略，不用专门开启）
	if (g_ProcState.currentStep == 0)
	{
		g_ProcState.currentStep = 1;
		g_ProcState.passedTimeMs = 0;
		return;
	}
	// [1]获取加速度计数值，并计算角度
	if (g_ProcState.currentStep == 1)
	{
		Init_ADXL362();
		HAL_Delay(2);
		Act_ADXL362_ReadData();

		// 获取加速度计数值
		g_SensorData.gravity1 = ADXL362_GetGravity(0);
		g_SensorData.gravity2 = ADXL362_GetGravity(1);
		g_SensorData.gravity3 = ADXL362_GetGravity(2);

		// 计算角度
		g_SensorData.pitch = atan2((float)(g_SensorData.gravity2),
								   (float)(g_SensorData.gravity3)) /
							 3.1415926 * 180.0;
		g_SensorData.roll = atan2((float)(g_SensorData.gravity1),
								  (float)(g_SensorData.gravity3)) /
							3.1415926 * 180.0;
		g_SensorData.yaw = 0;

		g_ProcState.currentStep = 2;
		g_ProcState.passedTimeMs = 0;
		return;
	}
	// [2]关闭加速度计电源（因加速度计低功耗，此步忽略，不用专门开启）并开启ADC
	if (g_ProcState.currentStep == 2)
	{
		HAL_ADC_Start(&hadc);
		g_ProcState.currentStep = 3;
		g_ProcState.passedTimeMs = 0;
		return;
	}
	// [3]读取ADC数据后，关闭ADC
	if (g_ProcState.currentStep == 3)
	{
		Act_SampleAdc();
		HAL_ADC_Stop(&hadc);
		g_ProcState.currentStep = 4;
		g_ProcState.passedTimeMs = 0;
		return;
	}
}

/*生成JSON报文*/
void Proc_GenerateJson()
{
	char tmpStr[40];
	g_ProcState.maxStep = 1;

	if (g_ProcState.currentStep == 0)
	{
		//[0] Generate json string
		g_JsonData.length =
			sprintf((char *)(g_JsonData.data),
					"data={\'Iden\':{\'Prod_Id\':\'%s\',\'Dev_Name\':\'%s\',\'Dev_Key\':\'%s\'},"
					"\'Data\':{\'Bat_Lv\':%3d,\'Bat_Vot\':%2.2f,\'Ati_Roll\':%3.2f,\'Ati_Yaw\':%3.2f,\'Ati_Pitch\':%3.2f,"
					"\'Snr_Dist\':%3.2f,\'Res_Arg\':%4d,\'Crd_Lat\':%3.6f,\'Crd_Lon\':%3.6f,"
					"\'Wqt_Tmpt\':%3.2f,\'Wqt_Oxge\':%3.2f,\'Wqt_Ph\':%3.2f,"
					"\'Wqt_Ammo\':%3.2f,\'Wqt_Turb\':%3.2f}}",
					EQPTYPE_STR, EQPTNAME_STR, EQPKEY_STR, 50,
					g_SensorData.BatVoltage, g_SensorData.roll,
					g_SensorData.yaw, g_SensorData.pitch,
					g_SensorData.WaterSurfaceRange, g_SensorData.adcVal2,
					0.0f, 0.0f, g_SensorData.temperture, g_SensorData.oxgen,
					g_SensorData.pH, g_SensorData.ammon_ion_con,
					g_SensorData.turbidity);

		// Test print
		sprintf(tmpStr, "Generate Json<0>:Total %d bytes.\r\n",
				g_JsonData.length);
		Log(tmpStr);
		//		Log(g_JsonData.data);
		g_ProcState.currentStep = 1;
	}
}

/*Clean M5310A uart rx buffer*/
void M5310A_CleanRx()
{
	memset(g_AtBuffer.rxData, 0, 128);
	g_AtBuffer.rxLength = 0;
}

/*重启到�?�讯模块的接�??*/
void M5310A_RestartRx(void)
{
	HAL_UART_AbortReceive(&DTU_PORT);
	HAL_UART_Receive_DMA(&DTU_PORT, (uint8_t *)(g_AtBuffer.rxData), 128);
}

/*Http 上发报文流程*/
void Proc_HttpPost(void)
{
	g_ProcState.maxStep = 15;
	//[0] 清理�??有的原有HTTP连接
	if (g_ProcState.currentStep == 0)
	{
		g_ProcState.maxOverTimeMs = 1000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_HttpCloseStr,
							  strlen(AT_HttpCloseStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 1;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<0>:Try to clean connection.");
		return;
	}
	//[1-2]用AT指令�??查M5310A模块是否正常
	if (g_ProcState.currentStep == 1)
	{
		g_ProcState.maxOverTimeMs = 10000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_CheckStr,
							  strlen(AT_CheckStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 2;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<1>:Transmit AT check command.");
		return;
	}
	if (g_ProcState.currentStep == 2)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_CheckReturnStr))
		{
			g_ProcState.currentStep = 3;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<2>:AT check command OK.");
		}
		else
		{
			HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_CheckStr,
								  strlen(AT_CheckStr));
			M5310A_CleanRx();
			M5310A_RestartRx();
		}
		return;
	}
	//[3-4]�??查模块到因特网的连接
	if (g_ProcState.currentStep == 3)
	{
		//跳过�??查；直接到步�??5
		g_ProcState.currentStep = 5;
		g_ProcState.passedTimeMs = 0;
		return;
		g_ProcState.maxOverTimeMs = 20000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_PingStr,
							  strlen(AT_PingStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 4;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<3>:Transmit AT Ping command.");
		return;
	}
	if (g_ProcState.currentStep == 4)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_PintReturnStr))
		{
			g_ProcState.currentStep = 5;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<4>:AT Ping command OK.");
		}
		else
		{
			// g_SeqState.currentStep=5;
			// g_SeqState.passedTimeMs=0;
			// Log("Http Post<4>:AT Ping jumped.\r\n");
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_PingStr,strlen(AT_PingStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
	//[5-6]创建HTTP连接
	if (g_ProcState.currentStep == 5)
	{
		g_ProcState.maxOverTimeMs = 2000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_HttpCreateStr,
							  strlen(AT_HttpCreateStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 6;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<5>:AT create Http link.");
		return;
	}
	if (g_ProcState.currentStep == 6)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_HttpCreateReturnStr))
		{
			g_ProcState.currentStep = 7;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<6>:Create Http link successfully.");
		}
		else
		{
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_HttpCreateStr,strlen(AT_HttpCreateStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
	//[7-8]添加HTTP报头
	if (g_ProcState.currentStep == 7)
	{
		g_ProcState.maxOverTimeMs = 2000;
		sprintf(PostBuffer,
				"AT+HTTPHEADER=0,\"Content-Type:application/x-www-form-urlencoded\\r\\n\"\r\n");
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)PostBuffer,
							  strlen(PostBuffer));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 8;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<7>:AT add Http header.");
		return;
	}
	if (g_ProcState.currentStep == 8)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_HttpHeaderReturnStr))
		{
			g_ProcState.currentStep = 9;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<8>:Add Http header successfully.\r\n");
		}
		else
		{
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_HttpHeaderStr,strlen(AT_HttpHeaderStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
	//[9-10]添加HTTP报文
	if (g_ProcState.currentStep == 9)
	{
		g_ProcState.maxOverTimeMs = 5000;
		sprintf(PostBuffer, "AT+HTTPCONTENT=0,\"%s\"\r\n", g_JsonData.data);
		Log(PostBuffer);

		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)PostBuffer,
							  strlen(PostBuffer));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 10;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<9>:AT add Http content.");
		return;
	}
	if (g_ProcState.currentStep == 10)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_HttpContentReturnStr))
		{
			g_ProcState.currentStep = 11;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<10>:Add Http content successfully.");
		}
		else
		{
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_HttpContentStr,strlen(AT_HttpContentStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
	//[11-12]发�?�HTTP请求
	if (g_ProcState.currentStep == 11)
	{
		g_ProcState.maxOverTimeMs = 8000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_HttpSendStr,
							  strlen(AT_HttpSendStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 12;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<11>:Transmit Http post request.");
		return;
	}
	if (g_ProcState.currentStep == 12)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_HttpSendReturnStr))
		{
			g_ProcState.currentStep = 13;
			g_ProcState.passedTimeMs = 0;

			Log("Http Post<12>:Http post successfully.");
		}
		else
		{
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_HttpSendStr,strlen(AT_HttpSendStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
	//[13-14]关闭HTTP连接
	if (g_ProcState.currentStep == 13)
	{
		g_ProcState.maxOverTimeMs = 3000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_HttpCloseStr,
							  strlen(AT_HttpCloseStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 14;
		g_ProcState.passedTimeMs = 0;
		Log("Http Post<13>:Close Http link.");
		return;
	}
	if (g_ProcState.currentStep == 14)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_HttpCloseReturnStr))
		{
			g_ProcState.currentStep = 15;
			g_ProcState.passedTimeMs = 0;
			Log("Http Post<14>:Http link closed successfully.");
		}
		else
		{
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_HttpCloseStr,strlen(AT_HttpCloseStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
}

/*进入睡眠模式*/
void Proc_SysStop()
{
	RTC_TimeTypeDef t_CurrTime;
	uint32_t _sleepSeconds;
	char tmpStr[64];
	HAL_RTC_GetTime(&hrtc, &t_CurrTime, RTC_FORMAT_BIN);
	// Calculate wakeup time
	t_CurrTime.Seconds += DATA_SAMPGAP_S;
	t_CurrTime.Minutes += DATA_SAMPGAP_M;
	t_CurrTime.Hours += DATA_SAMPGAP_H;

	if (t_CurrTime.Seconds >= 60)
	{
		t_CurrTime.Minutes += t_CurrTime.Seconds / 60;
		t_CurrTime.Seconds = t_CurrTime.Seconds % 60;
	}

	if (t_CurrTime.Minutes >= 60)
	{
		t_CurrTime.Hours += t_CurrTime.Minutes / 60;
		t_CurrTime.Minutes = t_CurrTime.Minutes % 60;
	}
	if (t_CurrTime.Hours >= 24)
	{
		t_CurrTime.Hours = t_CurrTime.Hours % 24;
	}

	_sleepSeconds = DATA_SAMPGAP_S + DATA_SAMPGAP_M * 60 + DATA_SAMPGAP_H * 3600;
	if (HAL_OK != HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, _sleepSeconds,
											  RTC_WAKEUPCLOCK_CK_SPRE_16BITS))
	{
		Error_Handler();
	}
	sprintf(tmpStr,
			"System Stop<0>:Enter stop mode(%d hours %d minutes %d seconds.)",
			DATA_SAMPGAP_H, DATA_SAMPGAP_M, DATA_SAMPGAP_S);
	Log(tmpStr);

	HAL_ADC_Stop(&hadc);
	HAL_Delay(200);
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

/*用网络时间矫正本地时�??*/
void Proc_CorrectRtc()
{
	g_ProcState.maxStep = 2;
	//[0-1]从M5310A获得网络时间
	if (g_ProcState.currentStep == 0)
	{
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_LocalTimeStr,
							  strlen(AT_LocalTimeStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 1;
		Log("Try to get Internet time from M5310A.");
		return;
	}
	if (g_ProcState.currentStep == 1)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_LocalTimeReturnStr))
		{
			//解包，并修改本地时间
			CorrectRtcTime(strstr(g_AtBuffer.rxData, AT_LocalTimeReturnStr));
			Log("Get Internet time successfully.");
			g_ProcState.currentStep = 2;
		}
		else
		{
			HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_LocalTimeStr,
								  strlen(AT_LocalTimeStr));
			M5310A_CleanRx();
			M5310A_RestartRx();
		}
		return;
	}
}

/*Sequence for config M5310A PSM mode */
/*配置M5310A的Power Saving Mode(省电休眠模式)*/
void Proc_ConfigPsm()
{
	g_ProcState.maxStep = 2;
	if (g_ProcState.currentStep == 0)
	{
		g_ProcState.maxOverTimeMs = 20000;
		HAL_UART_Transmit_DMA(&DTU_PORT, (uint8_t *)AT_CfgPsmStr,
							  strlen(AT_CfgPsmStr));
		M5310A_CleanRx();
		M5310A_RestartRx();
		g_ProcState.currentStep = 1;
		g_ProcState.passedTimeMs = 0;
		Log("Config PSM<1>:Transmit AT+CEREG command.");
		return;
	}
	if (g_ProcState.currentStep == 1)
	{
		// Check return data
		if (strstr(g_AtBuffer.rxData, AT_CfgPsmReturnStr))
		{
			g_ProcState.currentStep = 2;
			g_ProcState.passedTimeMs = 0;
			Log("Config PSM<2>:PSM config OK.");
		}
		else
		{
			// g_SeqState.currentStep=5;
			// g_SeqState.passedTimeMs=0;
			// Log("Http Post<4>:AT Ping jumped.\r\n");
			// HAL_UART_Transmit_DMA(&DTU_PORT,(uint8_t*)AT_PingStr,strlen(AT_PingStr));
			// CleanRxBuffer();
			// RestartReceive();
		}
		return;
	}
}

/**
 * @brief 重置MODBUS寄存器状态
 *
 */
void MDB_ResetControl()
{
	uint8_t i;
	g_Mdb_AutoReg.currentLooper = 0;
	g_Mdb_AutoReg.retryCounter = 0;
	g_Mdb_AutoReg.timeCounter = 0;
	for (i = 0; i < MDB_AUTO_TABLESIZE; i++)
	{
		g_Mdb_AutoReg.table[i].state = Mdb_Empty;
	}
	memset(g_Mdb_Buffer.rxData, 0, 128);
}

/**
 * @brief 初始化MOSBUS寄存器
 *
 */
void Init_MDB_AUTO()
{
	// Initalize auto reading table
	g_Mdb_AutoReg.table[0].enabled_Line = 1;
	g_Mdb_AutoReg.table[0].slaveID = 1;
	g_Mdb_AutoReg.table[0].destPtr = (uint16_t *)&(g_SensorData.temperture);

	g_Mdb_AutoReg.table[1].enabled_Line = 1;
	g_Mdb_AutoReg.table[1].slaveID = 2;
	g_Mdb_AutoReg.table[1].destPtr = (uint16_t *)&(g_SensorData.oxgen);

	g_Mdb_AutoReg.table[2].enabled_Line = 1;
	g_Mdb_AutoReg.table[2].slaveID = 3;
	g_Mdb_AutoReg.table[2].destPtr = (uint16_t *)&(g_SensorData.pH);

	g_Mdb_AutoReg.table[3].enabled_Line = 2;
	g_Mdb_AutoReg.table[3].slaveID = 4;
	g_Mdb_AutoReg.table[3].destPtr = (uint16_t *)&(g_SensorData.turbidity);

	g_Mdb_AutoReg.table[4].enabled_Line = 2;
	g_Mdb_AutoReg.table[4].slaveID = 5;
	g_Mdb_AutoReg.table[4].destPtr = (uint16_t *)&(g_SensorData.ammon_ion_con);
}

/**
 * @brief 判断两个8位数组的前n项是否相等
 *
 */
uint8_t ArrayEqu(uint8_t *src, uint8_t *dest, uint16_t length)
{
	uint16_t i;
	for (i = 0; i < length; i++)
	{
		if (*(src + i) != *(dest + i))
		{
			return 0;
		}
	}
	return 1;
}

/**
 * @brief 读取传感器数据，进行数据比对和解析
 *
 * @return 是否正常完成解析 1-正常 0-不正常
 */
uint8_t MDB_DecodeFrame()
{
	union f32_u8 t_floatVal;
	t_floatVal.f32 = 0;
	// Try to deal receive buffer
	//通过对比接收到的数据头，判断是否位正确响�??
	//没有CRC校验计算过程
	if (ArrayEqu((uint8_t *)(g_Mdb_Buffer.rxData),
				 (uint8_t *)(MDB_RX_ARRAY[g_Mdb_AutoReg.currentLooper]), 3))
	{

		if ((g_Mdb_Buffer.rxData[5] == 0) && (g_Mdb_Buffer.rxData[6] == 0))
		{
			return 0;
		}

		//解析和存储数据
		if (g_Mdb_Buffer.rxData[0] < 0x7F)
		{
			switch (g_Mdb_AutoReg.currentLooper)
			{
			case 0:
				g_SensorData.temperture = (float)(g_Mdb_Buffer.rxData[3] * 0x100 + g_Mdb_Buffer.rxData[4]) / 100.0f;
				Log("Temperature data updated.");
				break;
			case 1:
				g_SensorData.oxgen = (float)(g_Mdb_Buffer.rxData[3] * 0x100 + g_Mdb_Buffer.rxData[4]) / 100.0f;
				Log("Oxygen data updated.");
				break;
			case 2:
				g_SensorData.pH = (float)(g_Mdb_Buffer.rxData[3] * 0x100 + g_Mdb_Buffer.rxData[4]) / 100.0f;
				Log("PH data updated.");
				break;
			case 3:
				t_floatVal.u8[3] = g_Mdb_Buffer.rxData[3];
				t_floatVal.u8[2] = g_Mdb_Buffer.rxData[4];
				t_floatVal.u8[1] = g_Mdb_Buffer.rxData[5];
				t_floatVal.u8[0] = g_Mdb_Buffer.rxData[6];

				g_SensorData.turbidity = t_floatVal.f32;
				Log("Turbidity data updated.");
			case 4:
				g_SensorData.ammon_ion_con = (float)(g_Mdb_Buffer.rxData[3] * 0x100 + g_Mdb_Buffer.rxData[4]) / 100.0f;
				Log("Ammonium ion concentration data updated.");
			default:
				break;
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * @brief 读取传感器采集的数据
 *
 */
void Proc_MDBread()
{
	RTC_TimeTypeDef t_CurrTime;
	g_ProcState.maxStep = MDB_AUTO_TABLESIZE;
	g_ProcState.maxOverTimeMs = 8000;
	HAL_RTC_GetTime(&hrtc, &t_CurrTime, RTC_FORMAT_BIN);

	// 打开传感器线路1和2的电源，等待30秒后设备启动完成
	if (g_Mdb_AutoReg.currentLooper == 0)
	{
		if (GPIO_PIN_SET != HAL_GPIO_ReadPin(PWR1_EN_GPIO_Port, PWR1_EN_Pin))
		{
			HAL_GPIO_WritePin(PWR1_EN_GPIO_Port, PWR1_EN_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(PWR2_EN_GPIO_Port, PWR2_EN_Pin, GPIO_PIN_SET);
			Log("Turn on power 1+2 and wait 30seconds.");
			HAL_Delay(30000);
		}
	}
	// Ask looper
	g_Mdb_AutoReg.currentLooper = g_ProcState.currentStep;

	// 读取传感器数据
	if (MDB_DecodeFrame() != 0)
	{
		// 接收到数据
		HAL_UART_AbortReceive(&RS485_1_PORT);
		HAL_UART_AbortReceive(&RS485_2_PORT);
		memset(g_Mdb_Buffer.rxData, 0, 128);
		g_Mdb_AutoReg.timeCounter = 0;
		g_ProcState.currentStep++;
		g_ProcState.passedTimeMs = 0;
		g_Mdb_AutoReg.retryCounter = 0;
		Log("Modbus:PDU received.");
		if (g_Mdb_AutoReg.currentLooper == g_ProcState.maxStep)
		{
			// 关闭传感器
			HAL_GPIO_WritePin(PWR1_EN_GPIO_Port, PWR1_EN_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(PWR2_EN_GPIO_Port, PWR2_EN_Pin, GPIO_PIN_RESET);
			Log("Turn off power 1+2 .");
		}
	}
	else
	{
		// 未接收到数据
		if (g_Mdb_AutoReg.timeCounter == 0)
		{
			// 若计时器为0，则可以尝试读取数据
			HAL_UART_AbortReceive(&RS485_1_PORT);
			HAL_UART_AbortReceive(&RS485_2_PORT);
			memset(g_Mdb_Buffer.rxData, 0, 128);
			// 查询数据
			if (g_Mdb_AutoReg.table[g_Mdb_AutoReg.currentLooper].enabled_Line == 1)
			{
				// 查询线路1上的设备数据
				HAL_UART_Transmit(&RS485_1_PORT,
								  MDB_TX_ARRAY[g_Mdb_AutoReg.currentLooper], 8, 1000);
				HAL_UART_Receive_DMA(&RS485_1_PORT,
									 (uint8_t *)(g_Mdb_Buffer.rxData), 127);
			}
			else
			{
				// 查询线路2上的设备数据
				HAL_UART_Transmit(&RS485_2_PORT,
								  MDB_TX_ARRAY[g_Mdb_AutoReg.currentLooper], 8, 1000);
				HAL_UART_Receive_DMA(&RS485_2_PORT,
									 (uint8_t *)(g_Mdb_Buffer.rxData), 127);
			}
			if (g_Mdb_AutoReg.retryCounter == 0)
			{
				Log("Modbus:Send master read request.");
			}
		}

		// 使用计时器来操控重试的时间间隔
		if (g_Mdb_AutoReg.timeCounter < MDB_OVERTIME_MS)
		{
			// 时间间隔未到阈值，则等待
			g_Mdb_AutoReg.timeCounter += CIRCLE_GAP_MS;
		}
		else
		{
			// 时间间隔达到阈值，可以重试了
			if (g_Mdb_AutoReg.retryCounter < 3)
			{
				// 重试次数未达到阈值
				g_Mdb_AutoReg.retryCounter++;
				g_Mdb_AutoReg.timeCounter = 0;
				g_Mdb_AutoReg.table[g_Mdb_AutoReg.currentLooper].state =
					Mdb_Reading;
			}
			else
			{
				// 重试次数超过阈值
				g_Mdb_AutoReg.table[g_Mdb_AutoReg.currentLooper].state =
					MDB_Lost;
				memset(g_Mdb_Buffer.rxData, 0, 128);
				g_Mdb_AutoReg.timeCounter = 0;
				g_ProcState.currentStep++;
				g_ProcState.passedTimeMs = 0;
				g_Mdb_AutoReg.retryCounter = 0;
				Log("Modbus:Slave response timeout.");
			}
		}
	}

	// Clear REG statue
	if (g_ProcState.currentStep == g_ProcState.maxStep)
	{

		MDB_ResetControl();
		memset(g_Mdb_Buffer.rxData, 0, 128);
	}
}

/**
 * @brief 主函数
 *
 * @return 程序是否正常结束 0-正常 非0-不正常
 */
int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_ADC_Init();
	MX_SPI2_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART4_UART_Init();
	MX_USART5_UART_Init();
	MX_RTC_Init();
	MX_TIM6_Init();
	MX_CRC_Init();

	// 初始化全局变量和日志功能
	MDB_ResetControl();
	Act_ResetProc();
	Init_MDB_AUTO();
	Init_Log();
	HAL_TIM_Base_Start_IT(&htim6);

	// 初始化外设
	HardwareInit();

	// 进入主程序流程
	while (1)
	{
		HardwareLowlevelLoop();
	}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief ADC Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC_Init(void)
{

	/* USER CODE BEGIN ADC_Init 0 */

	/* USER CODE END ADC_Init 0 */

	ADC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN ADC_Init 1 */

	/* USER CODE END ADC_Init 1 */
	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc.Instance = ADC1;
	hadc.Init.OversamplingMode = DISABLE;
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
	hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc.Init.LowPowerAutoWait = DISABLE;
	hadc.Init.LowPowerFrequencyMode = DISABLE;
	hadc.Init.LowPowerAutoPowerOff = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure for the selected ADC regular channel to be converted.
	 */
	sConfig.Channel = ADC_CHANNEL_5;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC_Init 2 */

	/* USER CODE END ADC_Init 2 */
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{

	/* USER CODE BEGIN CRC_Init 0 */

	/* USER CODE END CRC_Init 0 */

	/* USER CODE BEGIN CRC_Init 1 */

	/* USER CODE END CRC_Init 1 */
	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_HALFWORDS;
	if (HAL_CRC_Init(&hcrc) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN CRC_Init 2 */

	/* USER CODE END CRC_Init 2 */
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void)
{

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */
	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK)
	{
		Error_Handler();
	}

	/* USER CODE BEGIN Check_RTC_BKUP */

	/* USER CODE END Check_RTC_BKUP */

	/** Initialize RTC and set the Time and Date
	 */
	sTime.Hours = 0x0;
	sTime.Minutes = 0x0;
	sTime.Seconds = 0x0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
	{
		Error_Handler();
	}
	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_JANUARY;
	sDate.Date = 0x1;
	sDate.Year = 0x0;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
	{
		Error_Handler();
	}
	/** Enable the WakeUp
	 */
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 65535,
									RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

	/* USER CODE BEGIN SPI2_Init 0 */

	/* USER CODE END SPI2_Init 0 */

	/* USER CODE BEGIN SPI2_Init 1 */

	/* USER CODE END SPI2_Init 1 */
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 7;
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI2_Init 2 */

	/* USER CODE END SPI2_Init 2 */
}

/**
 * @brief TIM6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM6_Init(void)
{

	/* USER CODE BEGIN TIM6_Init 0 */

	/* USER CODE END TIM6_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM6_Init 1 */

	/* USER CODE END TIM6_Init 1 */
	htim6.Instance = TIM6;
	htim6.Init.Prescaler = 16;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.Period = 50000;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM6_Init 2 */

	/* USER CODE END TIM6_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_RS485Ex_Init(&huart1, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */
}

/**
 * @brief USART4 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART4_UART_Init(void)
{

	/* USER CODE BEGIN USART4_Init 0 */

	/* USER CODE END USART4_Init 0 */

	/* USER CODE BEGIN USART4_Init 1 */

	/* USER CODE END USART4_Init 1 */
	huart4.Instance = USART4;
	huart4.Init.BaudRate = 9600;
	huart4.Init.WordLength = UART_WORDLENGTH_8B;
	huart4.Init.StopBits = UART_STOPBITS_1;
	huart4.Init.Parity = UART_PARITY_NONE;
	huart4.Init.Mode = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart4) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART4_Init 2 */

	/* USER CODE END USART4_Init 2 */
}

/**
 * @brief USART5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART5_UART_Init(void)
{

	/* USER CODE BEGIN USART5_Init 0 */

	/* USER CODE END USART5_Init 0 */

	/* USER CODE BEGIN USART5_Init 1 */

	/* USER CODE END USART5_Init 1 */
	huart5.Instance = USART5;
	huart5.Init.BaudRate = 9600;
	huart5.Init.WordLength = UART_WORDLENGTH_8B;
	huart5.Init.StopBits = UART_STOPBITS_1;
	huart5.Init.Parity = UART_PARITY_NONE;
	huart5.Init.Mode = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_RS485Ex_Init(&huart5, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART5_Init 2 */

	/* USER CODE END USART5_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel2_3_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
	/* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB,
					  VSEN_Pin | SPI2_NCS_Pin | UART5_NRE_Pin | PWR2_EN_Pin | BRESET_Pin,
					  GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, UART1_NRE_Pin | PWR1_EN_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : VSEN_Pin SPI2_NCS_Pin UART5_NRE_Pin PWR2_EN_Pin */
	GPIO_InitStruct.Pin = VSEN_Pin | SPI2_NCS_Pin | UART5_NRE_Pin | PWR2_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : UART1_NRE_Pin PWR1_EN_Pin */
	GPIO_InitStruct.Pin = UART1_NRE_Pin | PWR1_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : BRESET_Pin */
	GPIO_InitStruct.Pin = BRESET_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BRESET_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM2 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM2)
	{
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */
	if (htim->Instance == TIM6)
	{
		HardwareLoop10ms();
	}
	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
