/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 * @author
 * @copyright
 ******************************************************************************
 */

/****************************************************************************************************************
 * Includes
 ****************************************************************************************************************/


#include "test.h"
#include "basic.h"

/****************************************************************************************************************
 * Private typedef
 ****************************************************************************************************************/

/****************************************************************************************************************
 * Private define
 ****************************************************************************************************************/
#define HW_TEST

/****************************************************************************************************************
 * Private macro
 ****************************************************************************************************************/

/****************************************************************************************************************
 * Private variables
 ****************************************************************************************************************/

/* 全域變數定義 */
volatile uint8_t rx_data = 0;     // 儲存接收到的藍芽字元
uint16_t current_speed = 700;     // 馬達預設 PWM 佔空比 (範圍: 0 ~ 999)

/****************************************************************************************************************
 * Private function prototypes
 ****************************************************************************************************************/
void Init_all(void);

/* 函式宣告 */
void GPIO_Configuration(void);
void TIM_PWM_Configuration(void);
void USART1_Configuration(void);
void Motor_SetSpeed(uint16_t left_speed, uint16_t right_speed);
void Motor_Forward(void);
void Motor_Backward(void);
void Motor_TurnLeft(void);
void Motor_TurnRight(void);
void Motor_Stop(void);

/****************************************************************************************************************
 * Private functions
 ****************************************************************************************************************/

/**
 * @brief  The application entry point.
 * @retval int
 */

/* 主程式 */
int main(void)
{
  
  /* Init */
  Init_all();

#ifdef HW_TEST
  
#endif  
  
  // 1. 硬體周邊初始化
    GPIO_Configuration();
    TIM_PWM_Configuration();
    USART1_Configuration();


    // 2. 設定預設車速並停止馬達
    Motor_SetSpeed(current_speed, current_speed);
    Motor_Stop();


    // 3. 主迴圈 (保持低負載，由 USART1 中斷接收藍芽指令)
    while (1)
    {
        // 可在此處加入其他非阻塞邏輯
    }
}


void Init_all(void)
{
  
}


/* ---------------------------------------------------------------------------*/
/* USART1 藍芽中斷服務函式 (Interrupt Service Routine)                         */
/* ---------------------------------------------------------------------------*/
void USART1_IRQHandler(void)
{
    // 確認是否為接收暫存器非空中斷 (RXNE)
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 讀取接收到的資料
        rx_data = USART_ReceiveData(USART1);


        // 根據藍芽指令切換遙控車動作
        switch (rx_data)
        {
            case 'F': case 'f': // 前進
                Motor_Forward();
                break;


            case 'B': case 'b': // 後退
                Motor_Backward();
                break;


            case 'L': case 'l': // 原地左轉
                Motor_TurnLeft();
                break;


            case 'R': case 'r': // 原地右轉
                Motor_TurnRight();
                break;


            case 'S': case 's': // 煞車 / 停止
                Motor_Stop();
                break;


            /* 數字 1, 5, 9 調整車速 */
            case '1': current_speed = 300; Motor_SetSpeed(current_speed, current_speed); break;
            case '5': current_speed = 600; Motor_SetSpeed(current_speed, current_speed); break;
            case '9': current_speed = 999; Motor_SetSpeed(current_speed, current_speed); break;


            default:
                break;
        }


        // 清除中斷標誌位
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}


/* ---------------------------------------------------------------------------*/
/* 馬達底層控制函式                                                           */
/* ---------------------------------------------------------------------------*/


// 設定左/右馬達 PWM 速度 (範圍: 0 ~ 999)
void Motor_SetSpeed(uint16_t left_speed, uint16_t right_speed)
{
    TIM_SetCompare1(TIM2, left_speed);  // PA0 (ENA)
    TIM_SetCompare1(TIM3, right_speed); // PA6 (ENB)
}


// 前進
void Motor_Forward(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_1);   // IN1 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); // IN2 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_3);   // IN3 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_4); // IN4 = 0
}


// 後退
void Motor_Backward(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_1); // IN1 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_2);   // IN2 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_3); // IN3 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_4);   // IN4 = 1
}


// 原地左轉
void Motor_TurnLeft(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_1); // IN1 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_2);   // IN2 = 1
    GPIO_SetBits(GPIOA, GPIO_Pin_3);   // IN3 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_4); // IN4 = 0
}


// 原地右轉
void Motor_TurnRight(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_1);   // IN1 = 1
    GPIO_ResetBits(GPIOA, GPIO_Pin_2); // IN2 = 0
    GPIO_ResetBits(GPIOA, GPIO_Pin_3); // IN3 = 0
    GPIO_SetBits(GPIOA, GPIO_Pin_4);   // IN4 = 1
}


// 停止
void Motor_Stop(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4);
}


/* ---------------------------------------------------------------------------*/
/* 周邊配置與暫存器初始化                                                     */
/* ---------------------------------------------------------------------------*/


// GPIO 腳位初始化 (L298N 控制腳與 PWM 腳)
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;


    // 開啟 GPIOA 時脈
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);


    // PA1, PA2, PA3, PA4 配置為通用推挽輸出 (IN1 ~ IN4)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    // PA0 (TIM2_CH1) 與 PA6 (TIM3_CH1) 配置為複用推挽輸出 (PWM 輸出腳)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}


// 定時器 PWM 初始化 (TIM2 & TIM3)
void TIM_PWM_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;


    // 開啟 TIM2 與 TIM3 時脈
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE);


    // 時脈基底設定：72MHz / (71+1) = 1MHz，計數 1000 次 (0~999) => 1kHz PWM
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 999;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);


    // TIM2_CH1 (PA0) PWM 模式設定
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);


    // TIM3_CH1 (PA6) PWM 模式設定
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);


    // 開啟計數器
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}


// USART1 藍芽模組初始化 (PA9=TX, PA10=RX, BaudRate=9600)
void USART1_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    // 開啟 USART1 與 GPIOA 時脈
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);


    // PA9 (USART1_TX) 配置為複用推挽輸出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    // PA10 (USART1_RX) 配置為浮空輸入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    // USART1 設定 (波特率 9600, 8 位元資料, 1 停止位, 無校驗)
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);


    // 啟動 USART1 接收中斷 (RXNE)
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);


    // NVIC 中斷優先級設定
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);


    // 啟動 USART1
    USART_Cmd(USART1, ENABLE);
}

/****************************************************************************************************************
 * End of file
 ****************************************************************************************************************/