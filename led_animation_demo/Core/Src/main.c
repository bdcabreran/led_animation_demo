/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

#define MAX_NUMBER_LEDs (5)    // Max number of LEDs to use 
#define PB_DEBOUNCE_MS  (200)  // debounce period for push button

/**
 * @brief List of all possible led speeds 
 * 
 */
typedef enum
{
  E_LED_ANIM_SPEED_OFF = 0, // 0Hz   
  E_LED_ANIM_SPEED_LOW,     // 1Hz   speed
  E_LED_ANIM_SPEED_MEDIUM,  // 10Hz  speed
  E_LED_ANIM_SPEED_HIGH,    // 100Hz speed
  E_LED_ANIMn,          
}led_anim_speed_t;

/**
 * @brief Struct to encapsilate LEDs Port, Pin
 * 
 */
typedef struct 
{
  uint16_t     pin;
  GPIO_TypeDef *port;
}led_gpio_port_pin_t;


/*global instance of LED to use*/
led_gpio_port_pin_t led_gpio[MAX_NUMBER_LEDs] =
{
  {.port = LED_1_GPIO_Port, .pin = LED_1_Pin},
  {.port = LED_2_GPIO_Port, .pin = LED_2_Pin},
  {.port = LED_3_GPIO_Port, .pin = LED_3_Pin},
  {.port = LED_4_GPIO_Port, .pin = LED_4_Pin},
  {.port = LED_5_GPIO_Port, .pin = LED_5_Pin}
};

/**
 * @brief Return next animation
 */
led_anim_speed_t led_animation_next(led_anim_speed_t speed)
{
  // return sequence 0,1,2,3,0,1,2,3
  speed = ((speed + 1)% E_LED_ANIMn);
  return speed;
}

/**
 * @brief Return LED animation period in ms 
 */
uint32_t led_animation_get_period_ms(led_anim_speed_t speed)
{
  switch (speed)
  {
  case E_LED_ANIM_SPEED_OFF:      return 0       ; break;
  case E_LED_ANIM_SPEED_LOW :     return 1000    ; break;
  case E_LED_ANIM_SPEED_MEDIUM :  return 1000/4  ; break;
  case E_LED_ANIM_SPEED_HIGH :    return 1000/8  ; break;
  default:
    break;
  }
}


/**
 * @brief Execute blink no blocking mode in ALL LEDs
 * @note HAL_GetTick is a HAL function that increases every ms
 * @param speed 
 */
void led_animation_blink(led_anim_speed_t speed)
{
  if (led_animation_get_period_ms(speed) > 0)
  {
    static uint32_t tick_ms_cnt = 0;

    //wait for the period ms to expire 
    if (HAL_GetTick() - tick_ms_cnt > led_animation_get_period_ms(speed)) 
    {
      for (size_t i = 0; i < MAX_NUMBER_LEDs; i++)
        HAL_GPIO_TogglePin(led_gpio[i].port, led_gpio[i].pin);

      tick_ms_cnt = HAL_GetTick();  //reload tick ms counter 
    }
  }
  else
  {
    /*turn off all LEDs*/
    for (size_t i = 0; i < MAX_NUMBER_LEDs; i++)
      HAL_GPIO_WritePin(led_gpio[i].port, led_gpio[i].pin, GPIO_PIN_RESET);
  }
}

/**
 * @brief Return 1 if button is pressed for more than 200ms to prevent bouncing
 * 
 * @return uint8_t 
 */
uint8_t is_push_button_pressed(void)
{
  static uint32_t tick_ms_cnt = 0;
	GPIO_PinState button_st = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

	if (button_st == GPIO_PIN_RESET)
	{
		if (HAL_GetTick() - tick_ms_cnt >= PB_DEBOUNCE_MS)
		{
			tick_ms_cnt = HAL_GetTick();
			return 1;
		}
	}
	else
		tick_ms_cnt = HAL_GetTick();

	return 0;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  led_anim_speed_t speed = E_LED_ANIM_SPEED_OFF;

  /* Infinite loop */
  while (1)
  {
    if (is_push_button_pressed())
    	speed = led_animation_next(speed);

    led_animation_blink(speed);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|LED_1_Pin|LED_2_Pin|LED_3_Pin
                          |LED_4_Pin|LED_5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 LED_1_Pin LED_2_Pin LED_3_Pin
                           LED_4_Pin LED_5_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_4|LED_1_Pin|LED_2_Pin|LED_3_Pin
                          |LED_4_Pin|LED_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}



/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
