/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : FALL DETECTION SYSTEM
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PD */

#define LCD_ADDR (0x27 << 1)
#define MPU_ADDR (0x68 << 1)

#define FALL_X_THRESHOLD 12000
#define FALL_Z_THRESHOLD 22000

/* USER CODE END PD */

/* USER CODE BEGIN PV */

int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;
char buffer[16];

/* USER CODE END PV */

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN 0 */

// ================= LCD FUNCTIONS =================

void lcd_send_cmd(char cmd)
{
    uint8_t data_u, data_l;
    uint8_t data_t[4];

    data_u = (cmd & 0xF0);
    data_l = ((cmd << 4) & 0xF0);

    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_send_data(char data)
{
    uint8_t data_u, data_l;
    uint8_t data_t[4];

    data_u = (data & 0xF0);
    data_l = ((data << 4) & 0xF0);

    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 4, 100);
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

void lcd_put_cur(int row, int col)
{
    uint8_t pos;

    if(row == 0)
        pos = 0x80 + col;
    else
        pos = 0xC0 + col;

    lcd_send_cmd(pos);

}

void lcd_init(void)
{
    HAL_Delay(50);

    lcd_send_cmd(0x33);
    lcd_send_cmd(0x32);
    lcd_send_cmd(0x28);
    lcd_send_cmd(0x0C);
    lcd_send_cmd(0x06);
    lcd_send_cmd(0x01);

    HAL_Delay(5);
}

void lcd_send_string(char *str)
{
    while(*str)
    {
        lcd_send_data(*str++);
    }
}

// ================= MPU6050 FUNCTIONS =================

void MPU6050_Init(void)
{
    uint8_t check;
    uint8_t data;

    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, 0x75, 1, &check, 1, 1000);

    if(check == 0x68)
    {
        data = 0;
        HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR, 0x6B, 1, &data, 1, 1000);
    }
}

void MPU6050_Read_Accel(void)
{
    uint8_t Rec_Data[6];

    HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR, 0x3B, 1, Rec_Data, 6, 1000);

    Accel_X_RAW = (int16_t)((Rec_Data[0] << 8) | Rec_Data[1]);
    Accel_Y_RAW = (int16_t)((Rec_Data[2] << 8) | Rec_Data[3]);
    Accel_Z_RAW = (int16_t)((Rec_Data[4] << 8) | Rec_Data[5]);
}

/* USER CODE END 0 */

// ================= MAIN =================

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  // ===== LCD INIT =====
  lcd_init();
  HAL_Delay(100);

  // ===== MPU INIT =====
  MPU6050_Init();

  lcd_clear();

  while (1)
  {
      int32_t x_sum = 0;
      int32_t z_sum = 0;

      // ===== TAKE 10 READINGS =====
      for(int i = 0; i < 10; i++)
      {
          MPU6050_Read_Accel();

          x_sum += Accel_X_RAW;
          z_sum += Accel_Z_RAW;

          HAL_Delay(10);
      }

      // ===== AVERAGE =====
      Accel_X_RAW = x_sum / 10;
      Accel_Z_RAW = z_sum / 10;

      // ===== DISPLAY X =====
      sprintf(buffer, "X:%6d", Accel_X_RAW);

      lcd_put_cur(0,0);
      lcd_send_string("                ");

      lcd_put_cur(0,0);
      lcd_send_string(buffer);

      // ===== DISPLAY Z =====
      sprintf(buffer, "Z:%6d", Accel_Z_RAW);

      lcd_put_cur(1,0);
      lcd_send_string("                ");

      lcd_put_cur(1,0);
      lcd_send_string(buffer);

      // ===== FALL DETECTION =====
      if(abs(Accel_X_RAW) > FALL_X_THRESHOLD ||
         abs(Accel_Z_RAW) > FALL_Z_THRESHOLD)
      {
          lcd_clear();

          lcd_put_cur(0,0);
          lcd_send_string("FALL DETECTED");

          lcd_put_cur(1,0);
          lcd_send_string("ALERT!");

          // ===== EXTERNAL LED ON =====
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

          // ===== BUZZER ON =====
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);

          HAL_Delay(3000);

          // ===== EXTERNAL LED OFF =====
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

          // ===== BUZZER OFF =====
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

          lcd_clear();
      }

      HAL_Delay(200);
  }
}

// ================= SYSTEM CLOCK =================

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;

  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

// ================= I2C INIT =================

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;

  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  HAL_I2C_Init(&hi2c1);
}

// ================= UART INIT =================

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

  HAL_UART_Init(&huart2);
}

// ================= GPIO INIT =================

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // ===== EXTERNAL LED PB0 =====
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // ===== BUZZER PA6 =====
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_6;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// ================= ERROR HANDLER =================

void Error_Handler(void)
{
  while(1)
  {
  }
}
