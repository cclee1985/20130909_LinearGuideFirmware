/**
*****************************************************************************
**
**	Author: Lee Chee Cheong. Latest Update 9 September 2013, 10:29p.m.
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Keil MDK ARM(R)
**                STMicroelectronics STM32F4xx Standard Peripherals Library
**
**  Distribution: The file is distributed æ?³s is,ï¿½without any warranty
**                of any kind.
**
**  (c)Copyright CRSST
**  You may use this file as-is or modify it according to the needs of your
**  project.
**
**
*****************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>

/** @addtogroup STM32F4_Discovery_Peripheral_Examples
  * @{
  */

/** @addtogroup IO_Toggle
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define HEAD_SENSOR GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5)
#define TAIL_SENSOR GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)

#define MOTOR_COIL_ON GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define MOTOR_COIL_OFF GPIO_ResetBits(GPIOB, GPIO_Pin_12)

#define MOTOR_FORWARD 	0
#define MOTOR_BACKWARD	1
#define MOTOR_STOP			2

#define TIM12_PRESCALER 3

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
int32_t qeiFeedback = 0;
uint32_t vextaFeedback = 0;

/* Private function prototypes -----------------------------------------------*/
void Usart2_init(uint32_t baudrate);
void GPIO_Configuration(void);
void PWM_init(void);
void StepperFeedback_init (void);
void QEI1_init (void);

void TIM12_CH1_PWM_OUT (uint16_t frequency, uint8_t dutyCycle);
void TIM12_CH2_PWM_OUT (uint16_t frequency, uint8_t dutyCycle);
void runStepper (uint8_t direction, uint16_t speed);

void Delaynus(vu32 nus);
void delay_ms(uint32_t miliseconds);

void USART_puts(char *string);

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	/*!< At this stage the microcontroller clock setting is already configured, 
		this is done through SystemInit() function which is called from startup
		file (startup_stm32f4xx.s) before to branch to application main.
		To reconfigure the default setting of SystemInit() function, refer to
		system_stm32f4xx.c file
	*/
	uint16_t frequency;
	
	Usart2_init(115200);
	GPIO_Configuration ();
	PWM_init();
	QEI1_init ();
	StepperFeedback_init();
	
	while (1){
		USART_puts ("Hello world\r\n");
	}
	
	while (1);
	
	// ON motor coil
	MOTOR_COIL_ON;
	
	frequency = 700;
	while (1){
		runStepper (MOTOR_FORWARD, frequency);
		while (TAIL_SENSOR != Bit_RESET);
		
		runStepper (MOTOR_STOP, frequency);
		delay_ms(2000);
		
		runStepper (MOTOR_BACKWARD, frequency);
		while (HEAD_SENSOR != Bit_RESET);
		
		runStepper (MOTOR_STOP, frequency);
		delay_ms(2000);
	}
	
	printf ("Hello world\r\n");
	
	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	GPIO_SetBits(GPIOD, GPIO_Pin_14);
	while (1);				// program shouldn't reach here
}

/**
  * @brief  USART2 peripheral initialization
  * @param  baudrate --> the baudrate at which the USART is
 * 						   supposed to operate
  * @retval void
  */
void Usart2_init(uint32_t baudrate)
{
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable GPIOA clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	/* GPIOA Configuration: PA2, TX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* GPIOA Configuration: PA3, RX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure the USARTx */
	USART_Init(USART2, &USART_InitStructure);
	
	/* Enable the USARTx */
	USART_Cmd(USART2, ENABLE);
}

void PWM_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	/* TIMx clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);

	/* GPIOB clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* GPIOB Configuration: TIM12 CH1 (PB14) and TIM12 CH2 (PB15) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect TIMx pins to AF */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM12);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12);

	//PWM
	//84M/1000=84000  (timer f/f needed)
	//84000=16800*5 (period*prescaler)
	//16800*0.5=8400 (if for duty cycle 50% and 1000Hz)(4200 for CCRx_Val)(8400 for Period_Val)

	//84M/100=840000  (timer f/f needed)
	//840000=16800*50 (period*prescaler)
	//16800*0.5=8400 (if for duty cycle 50% and 100Hz)(8400 for CCRx_Val)(16800 for Period_Val)

	//(Prescaler - 1)
	//(Period - 1)

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);

	TIM_ARRPreloadConfig(TIM12, ENABLE);

	/* TIM12 disable counter */
	TIM_Cmd(TIM12, DISABLE);
}

void StepperFeedback_init (void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;
	GPIO_InitTypeDef   GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;

	/* Enable GPIOB clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure PB12 pin as input pull up */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect MOTOR feedback to PB13, use EXTI to capture feedback */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource13);

	/* Configure EXTI Line13 */
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Line = EXTI_Line13;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI Line13 Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}


/**
  * @brief  Encoder1 timer initialization
  * @param  void
  * @retval void
  * @brief
  * PB4  --> TIM3 CH1
  * PB5  --> TIM3 CH2
  */
void QEI1_init (void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM3 clock source enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Enable GPIOB, clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);
	/* Configure PB.4,5 as encoder alternate function */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect TIM3 pins to AF2 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_TIM3);

	/* Enable the TIM3 Update Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Timer configuration in Encoder mode */
	TIM_DeInit(TIM3);

	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;  // No prescaling
	TIM_TimeBaseStructure.TIM_Period = (4*4000)-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 6;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	// Clear all pending interrupts
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	//Reset counter
	TIM3->CNT = 0;      //encoder value

	TIM_Cmd(TIM3, ENABLE);
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOD Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	/* Configure PD12, 13, 14, 15, in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	/* CONFIGURE HEAD AND TAIL SENSOR HERE */
	/* GPIOE, B Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	/* Configure PE5, input pull up */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	/* Configure PB11, input pull up */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	/* CONFIGURE STEPPER MOTOR CONTROL SIGNAL HERE, COIL ON/OFF */
	/* Configure 12, in output open drain mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void TIM12_CH1_PWM_OUT (uint16_t frequency, uint8_t dutyCycle)
{
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t pwmDutyCycleCounter;
	uint32_t period;

	// calculate the period and duty cycle needed in instruction cycles
	period = ((uint32_t)(84000000)/(TIM12_PRESCALER+1)/frequency);
	pwmDutyCycleCounter = (dutyCycle*period)/100;		// resolution 1%
	
	// if > 100% duty cycle, just keet it at 100%
	if (dutyCycle >= 100)
		pwmDutyCycleCounter = period + 1;
	
	// disable timer before changing register value
	TIM_Cmd(TIM12, DISABLE);
	
	// update duty cycle
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pwmDutyCycleCounter;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM12, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM12, TIM_OCPreload_Enable);
	
	// update period
	TIM_TimeBaseStructure.TIM_Period = period;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM12, ENABLE);
	
	// reset timer counter to restart from 0
	TIM12->CNT = 0;
	
	// enable timer to continue generate pulse
	TIM_Cmd(TIM12, ENABLE);
}

void TIM12_CH2_PWM_OUT (uint16_t frequency, uint8_t dutyCycle)
{
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t pwmDutyCycleCounter;
	uint32_t period;

	// calculate the period and duty cycle needed in instruction cycles
	period = ((uint32_t)(84000000)/(TIM12_PRESCALER+1)/frequency);
	pwmDutyCycleCounter = (dutyCycle*period)/100;		// resolution 1%
	
	// if > 100% duty cycle, just keet it at 100%
	if (dutyCycle >= 100)
		pwmDutyCycleCounter = period + 1;
	
	// disable timer before changing register value
	TIM_Cmd(TIM12, DISABLE);
	
	// update duty cycle
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = pwmDutyCycleCounter;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM12, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable);
	
	// update period
	TIM_TimeBaseStructure.TIM_Period = period;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM12_PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM12, ENABLE);
	
	// reset timer counter to restart from 0
	TIM12->CNT = 0;
	
	// enable timer to continue generate pulse
	TIM_Cmd(TIM12, ENABLE);
}

void USART_puts(char *string)
{
	uint16_t i;
	for (i=0; string[i] !=0; i++){
		USART_SendData(USART2, string[i]);
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	}
}

void runStepper (uint8_t direction, uint16_t speed)
{
	// the output is inverted, thus 100% duty cycle = 0% duty in actual.
	if (direction == MOTOR_FORWARD){
		TIM12_CH1_PWM_OUT(speed, 100);
		TIM12_CH2_PWM_OUT(speed, 50);
	}
	else if (direction == MOTOR_BACKWARD){
		TIM12_CH2_PWM_OUT(speed, 100);
		TIM12_CH1_PWM_OUT(speed, 50);
	}
	else{
		TIM12_CH1_PWM_OUT(speed, 100);
		TIM12_CH2_PWM_OUT(speed, 100);
	}
}

void Delaynus(vu32 nus)
{
    uint8_t nCount;
    while (nus--)
        for (nCount = 6; nCount != 0; nCount--);
}

/**
* @brief  Inserts a estimated delay time in mS. CPU FREQ = 168MHz
* @param  miliseconds: specifies the delay time in mili second.
* @retval None
*/
void delay_ms(uint32_t miliseconds)
{
	uint32_t i;

	// one loop = 1ms. so loop for x loop to hit the delay.
	for(i=0; i < miliseconds; i++)
		Delaynus(1000);
}

/**
  * @brief  Encoder1 interrupt update
  * @param  void
  * @retval void
  */
void TIM3_IRQHandler(void)
{
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	if (TIM3->CR1&0x10)
		qeiFeedback -= 16000;
	else
		qeiFeedback += 16000;
}

/**
  * @brief  This function handles External Line13 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line13) != RESET){
		vextaFeedback++;
		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line13);
	}
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  USART_SendData(USART2, (uint8_t) ch);
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);

  return ch;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT CRSST 2013s *****END OF FILE*****************/
