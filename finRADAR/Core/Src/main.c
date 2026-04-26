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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>     // for radar drawing (sin, cos, abs)
#include <stdlib.h>   // for malloc/free in buffer
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_Delay(1000);

  // Init LCD (kept)
  LCD_Init();
  HAL_Delay(300);
  LCD_SetCursor(0, 0);
  LCD_Print(" Radar Ready! ");
  HAL_Delay(1200);
  LCD_Clear();

  // Init OLED
  SSD1306_Init();
  HAL_Delay(100);
  SSD1306_Clear();
  SSD1306_UpdateScreen();

  // Optional welcome on OLED
  SSD1306_Clear();
  for (int i = 0; i < 128; i += 8) SSD1306_DrawPixel(i, 30, 1);
  SSD1306_UpdateScreen();
  HAL_Delay(800);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  radarTask();
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

/* USER CODE BEGIN 4 */
#define SLAVE_ADDRESS_LCD  0x27 << 1

I2C_HandleTypeDef *hi2c_lcd = &hi2c1;

void LCD_SendInternal(uint8_t data, uint8_t flags) {
  uint8_t up = data & 0xF0;
  uint8_t lo = (data << 4) & 0xF0;
  uint8_t buf[4] = {up | flags | 0x08 | 0x04, up | flags | 0x08,
                    lo | flags | 0x08 | 0x04, lo | flags | 0x08};
  HAL_I2C_Master_Transmit(hi2c_lcd, SLAVE_ADDRESS_LCD, buf, 4, 100);
}

void LCD_SendCommand(uint8_t cmd) { LCD_SendInternal(cmd, 0); HAL_Delay(2); }
void LCD_SendData(uint8_t data)   { LCD_SendInternal(data, 0x01); HAL_Delay(1); }

void LCD_Init(void) {
  HAL_Delay(50); LCD_SendCommand(0x03); HAL_Delay(5);
  LCD_SendCommand(0x03); HAL_Delay(1); LCD_SendCommand(0x03);
  LCD_SendCommand(0x02); LCD_SendCommand(0x28); LCD_SendCommand(0x0C);
  LCD_SendCommand(0x06); LCD_SendCommand(0x01); HAL_Delay(2);
}

void LCD_Clear(void) { LCD_SendCommand(0x01); HAL_Delay(2); }
void LCD_SetCursor(uint8_t row, uint8_t col) {
  LCD_SendCommand((row == 0 ? 0x80 : 0xC0) + col);
}
void LCD_Print(const char *str) { while (*str) LCD_SendData(*str++); }

#define SSD1306_I2C_ADDR   (0x3C << 1)   // Change to 0x3D if needed
#define SSD1306_WIDTH      128
#define SSD1306_HEIGHT     64

I2C_HandleTypeDef *hi2c_oled = &hi2c2;   // <-- I2C2 for OLED

uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

void SSD1306_WriteCommand(uint8_t cmd) {
  uint8_t data[2] = {0x00, cmd};
  HAL_I2C_Master_Transmit(hi2c_oled, SSD1306_I2C_ADDR, data, 2, 100);
}

void SSD1306_WriteData(uint8_t* data, uint16_t size) {
  uint8_t *ptr = (uint8_t*)malloc(size + 1);
  if (!ptr) return;
  ptr[0] = 0x40;
  memcpy(ptr+1, data, size);
  HAL_I2C_Master_Transmit(hi2c_oled, SSD1306_I2C_ADDR, ptr, size+1, 200);
  free(ptr);
}

void SSD1306_Init(void) {
  HAL_Delay(100);
  SSD1306_WriteCommand(0xAE); // off
  SSD1306_WriteCommand(0xD5); SSD1306_WriteCommand(0x80);
  SSD1306_WriteCommand(0xA8); SSD1306_WriteCommand(0x3F);
  SSD1306_WriteCommand(0xD3); SSD1306_WriteCommand(0x00);
  SSD1306_WriteCommand(0x40);
  SSD1306_WriteCommand(0x8D); SSD1306_WriteCommand(0x14);
  SSD1306_WriteCommand(0x20); SSD1306_WriteCommand(0x00);
  SSD1306_WriteCommand(0xA1); SSD1306_WriteCommand(0xC8);
  SSD1306_WriteCommand(0xDA); SSD1306_WriteCommand(0x12);
  SSD1306_WriteCommand(0x81); SSD1306_WriteCommand(0xCF);
  SSD1306_WriteCommand(0xD9); SSD1306_WriteCommand(0xF1);
  SSD1306_WriteCommand(0xDB); SSD1306_WriteCommand(0x40);
  SSD1306_WriteCommand(0xA4); SSD1306_WriteCommand(0xA6);
  SSD1306_WriteCommand(0xAF); // on
  memset(SSD1306_Buffer, 0, sizeof(SSD1306_Buffer));
}

void SSD1306_UpdateScreen(void) {
  for(uint8_t i=0; i<8; i++) {
    SSD1306_WriteCommand(0xB0 + i);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(0x10);
    SSD1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
  }
}

void SSD1306_Clear(void) { memset(SSD1306_Buffer, 0, sizeof(SSD1306_Buffer)); }

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
  if (color)
    SSD1306_Buffer[x + (y/8)*SSD1306_WIDTH] |=  (1 << (y%8));
  else
    SSD1306_Buffer[x + (y/8)*SSD1306_WIDTH] &= ~(1 << (y%8));
}

void SSD1306_DrawLine(int x1, int y1, int x2, int y2, uint8_t color) {
  int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
  int dy = -abs(y2-y1), sy = y1<y2 ? 1 : -1;
  int err = dx + dy, e2;
  while(1) {
    SSD1306_DrawPixel(x1, y1, color);
    if (x1==x2 && y1==y2) break;
    e2 = 2*err;
    if (e2 >= dy) { err += dy; x1 += sx; }
    if (e2 <= dx) { err += dx; y1 += sy; }
  }
}

// Radar Visualization on OLED
void drawRadarVisualization(int angle, int dist) {
  SSD1306_Clear();
  int cx = SSD1306_WIDTH / 2;
  int cy = SSD1306_HEIGHT - 8;
  int r  = 52;

  // Concentric arcs
  for (int rad=15; rad<=r; rad+=18) {
    for (float a=0; a<=3.1416f; a+=0.08f) {
      int x = cx + (int)(rad * sinf(a));
      int y = cy - (int)(rad * cosf(a));
      if (x>=0 && x<SSD1306_WIDTH && y>=0 && y<SSD1306_HEIGHT)
        SSD1306_DrawPixel(x, y, 1);
    }
  }

  // Sweep line
  float rad_angle = (angle * 3.14159f) / 180.0f;
  int ex = cx + (int)(r * sinf(rad_angle));
  int ey = cy - (int)(r * cosf(rad_angle));
  SSD1306_DrawLine(cx, cy, ex, ey, 1);

  // Obstacle dot
  if (dist > 0 && dist <= 400) {
    int dist_px = (dist * r) / 400;
    if (dist_px > r) dist_px = r;
    int ox = cx + (int)(dist_px * sinf(rad_angle));
    int oy = cy - (int)(dist_px * cosf(rad_angle));
    for (int dy=-4; dy<=4; dy++)
      for (int dx=-4; dx<=4; dx++)
        if (dx*dx + dy*dy <= 12)
          SSD1306_DrawPixel(ox+dx, oy+dy, 1);
  }

  SSD1306_UpdateScreen();
}

void updateDisplayAndLED(int dist) {
  char buf[17];

  // LEDs OFF
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

  LCD_Clear();

  LCD_SetCursor(0, 0);
  if (dist <= 0 || dist > 400)
    LCD_Print("Dist: ---- cm");
  else {
    snprintf(buf, sizeof(buf), "Dist: %3d cm", dist);
    LCD_Print(buf);
  }

  LCD_SetCursor(1, 0);
  if (dist <= 0 || dist > 400) {
    LCD_Print("NO OBJECT     ");
  } else if (dist < 10) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // Red
    LCD_Print("LEVEL: CLOSE! ");
  } else if (dist <= 30) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET); // Yellow
    LCD_Print("LEVEL: MEDIUM ");
  } else {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET); // Green
    LCD_Print("LEVEL: FAR    ");
  }
}

void setServoAngle(int angle) {
  int pulse = 1000 + (angle * 1000 / 180);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}

int calculateDistance() {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
  DWT_Delay_us(2);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  DWT_Delay_us(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  uint32_t start = HAL_GetTick();
  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    if (HAL_GetTick() - start > 30) return 0;

  uint32_t t1 = DWT->CYCCNT;

  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    if (HAL_GetTick() - start > 30) return 0;

  uint32_t t2 = DWT->CYCCNT;
  uint32_t cycles = t2 - t1;
  float us = cycles / (SystemCoreClock / 1000000.0f);
  int dist = (int)(us * 0.034f / 2.0f);
  return (dist > 400 || dist <= 0) ? 0 : dist;
}

void DWT_Init() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t us) {
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000);
  while ((DWT->CYCCNT - start) < ticks);
}

void radarTask(void) {
  char buf[32];

  for (int angle = 15; angle <= 180; angle += 2) {
    setServoAngle(angle);
    HAL_Delay(15);
    int dist = calculateDistance();

    snprintf(buf, sizeof(buf), "%d,%d\r\n", angle, dist);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);

    updateDisplayAndLED(dist);       // LCD + LEDs
    drawRadarVisualization(angle, dist);  // OLED radar
  }

  for (int angle = 190; angle >= 5; angle -= 2) {
    setServoAngle(angle);
    HAL_Delay(15);
    int dist = calculateDistance();

    snprintf(buf, sizeof(buf), "%d,%d\r\n", angle, dist);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);

    updateDisplayAndLED(dist);
    drawRadarVisualization(angle, dist);
  }
}
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
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
