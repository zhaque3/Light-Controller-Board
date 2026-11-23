/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "fdcan.h"
#include "i2c.h"
#include "tim.h"
#include "usb.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_LEDS_P1   16
#define BITS_PER_LED  24
#define BIT_0         5
#define BIT_1         10
#define LEAD_RESET    240   /* 300 us low BEFORE data: absorbs the
                               spurious start pulse, resets the strip */
#define TRAIL_SLOTS   8     /* lets the last bit finish cleanly
                               before the callback stops the timer */
#define P1_BUF_LEN    (LEAD_RESET + NUM_LEDS_P1 * BITS_PER_LED + TRAIL_SLOTS)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef struct { uint8_t r, g, b; } Color;

static uint16_t p1_dma_buf[P1_BUF_LEN];
static volatile uint8_t p1_dma_busy = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void panel1_set_all(uint8_t r, uint8_t g, uint8_t b);
static float read_temperature(ADC_HandleTypeDef *hadc, uint32_t channel);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Fill panel 1's DMA buffer with one color for all LEDs and transmit.
   Non-blocking: returns as soon as DMA is started. */
static void panel1_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    while (p1_dma_busy);

    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    int n = 0;
    for (int i = 0; i < LEAD_RESET; i++)
        p1_dma_buf[n++] = 0;                    /* leading reset  */

    for (int led = 0; led < NUM_LEDS_P1; led++)
        for (int k = 23; k >= 0; k--)
            p1_dma_buf[n++] = (grb >> k) & 1 ? BIT_1 : BIT_0;

    for (int i = 0; i < TRAIL_SLOTS; i++)
        p1_dma_buf[n++] = 0;                    /* trailing pad   */

    p1_dma_busy = 1;
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2,
                          (uint32_t*)p1_dma_buf, P1_BUF_LEN);
}

/* DMA transfer complete: stop PWM, release the busy flag */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2);
        p1_dma_busy = 0;
    }
}

/* MCP9700 sensors:
     Panel 1: PA6 = ADC2_IN3
     Panel 2: PA7 = ADC2_IN4
     Panel 3: PA8 = ADC5_IN1
     Panel 4: PA9 = ADC5_IN2
   No external cap on VOUT, so we rely on a long 640.5-cycle sampling
   time plus 8-sample averaging to deal with the sensor's high output
   impedance. The channel is (re)configured on every call so the four
   sensors can share the two ADCs. */
static float read_temperature(ADC_HandleTypeDef *hadc, uint32_t channel)
{

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;
    HAL_ADC_ConfigChannel(hadc, &sConfig);

    uint32_t sum = 0;

    for (int i = 0; i < 8; i++) {
        HAL_ADC_Start(hadc);
        HAL_ADC_PollForConversion(hadc, 10);
        sum += HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);
    }

    float raw = sum / 8.0f;
    float mv  = (raw * 3300.0f) / 4095.0f;   /* 12-bit @ 3.3 V VREF        */
    return (mv - 500.0f) / 10.0f;            /* MCP9700: 500 mV @ 0 °C,
                                                10 mV per °C               */
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FDCAN2_Init();
  MX_I2C1_Init();
  MX_USB_PCD_Init();
  MX_ADC5_Init();
  MX_ADC2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  ssd1306_Init();

  /* ADC calibration (G4 requires this before first conversion) */
  HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
  HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, 1);
    static Color c    = { .r = 255, .g = 255, .b = 255 };   /* startup: white */
    static Color last = { .r = 0,   .g = 0,   .b = 0};   /* force 1st send */

    if (HAL_GPIO_ReadPin(SW4_GPIO_Port, SW4_Pin) == GPIO_PIN_RESET) {
        c = (Color){ .r = 254, .g = 0, .b = 0 };      /* SW4: latch red   */
    }
    else if (HAL_GPIO_ReadPin(SW5_GPIO_Port, SW5_Pin) == GPIO_PIN_RESET) {
        c = (Color){ .r = 0, .g = 254, .b = 0 };      /* SW5: latch green */
    }
    else if (HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET) {
        c = (Color){ .r = 0, .g = 0, .b = 254};       /* SW2: latch blue  */
    }

    /* Send to panel 1 only when the color actually changes */
    if (c.r != last.r || c.g != last.g || c.b != last.b) {
        panel1_set_all(c.r, c.g, c.b);
        last = c;
    }

    /* Read all four MCP9700s and update the OLED every 1 second */
    static uint32_t last_temp_tick = 0;
    static uint8_t  first_run      = 1;

    if (first_run || (HAL_GetTick() - last_temp_tick >= 1000U)) {
        first_run      = 0;
        last_temp_tick = HAL_GetTick();

        float temps[4];
        temps[0] = read_temperature(&hadc2, ADC_CHANNEL_3);   /* P1: PA6 */
        temps[1] = read_temperature(&hadc2, ADC_CHANNEL_4);   /* P2: PA7 */
        temps[2] = read_temperature(&hadc5, ADC_CHANNEL_1);   /* P3: PA8 */
        temps[3] = read_temperature(&hadc5, ADC_CHANNEL_2);   /* P4: PA9 */

        char ScreenBuffer[24];
        ssd1306_Fill(Black);

        for (int p = 0; p < 4; p++) {
            ssd1306_SetCursor(0, p * 16);
            if (temps[p] < 18) {
                sprintf(ScreenBuffer, "Panel %d is off", p + 1);
            }
            else {
                sprintf(ScreenBuffer, "P%d Temp(C) (%.1f)", p + 1, temps[p]);
            }
            ssd1306_WriteString(ScreenBuffer, Font_7x10, White);
        }

        ssd1306_UpdateScreen();
    }

    HAL_Delay(25);
  }
  /* USER CODE END 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
