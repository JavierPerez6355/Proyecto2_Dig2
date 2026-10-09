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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUF_LEN     20      //maximo de caracteres que se leen de la terminal
#define SPI_FRAME_LEN  4       //la trama SPI es de 4 bytes
#define SPI_HEADER     0xA5    //encabezado para que el ESP32 sepa que la trama es valida
#define LED_MIN        1
#define LED_MAX        3
#define TIEMPO_MIN     1
#define TIEMPO_MAX     60000   // ms
#define TIM2_TICKS_MS  10      //el TIM2 cuenta cada 0.1 ms, entonces 10 cuentas = 1 ms
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Print(const char *s);
static void UART_ReadLine(char *buf, uint16_t len);
static void Mostrar_Menu(void);
static void Submenu_SPI(void);
static uint8_t Parse_Comando(const char *s, uint8_t *led, uint16_t *tiempo);
static HAL_StatusTypeDef SPI_Enviar_Comando(uint8_t led, uint16_t tiempo);
static void Senal_Actualizar(uint16_t tiempo);
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
	char opcion[8];
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  //arranca la señal cuadrada en PA0, inicia con 500 ms alto / 500 ms bajo
  //el timer hace el cambio de estado solo, no ocupa al CPU
  if (HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1) != HAL_OK)
  {
      Error_Handler();
  }

  UART_Print("\r\n=== Proyecto 2 - Digital 2 ===\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    	Mostrar_Menu();

    	UART_ReadLine(opcion, sizeof(opcion));

        if (strcmp(opcion, "1") == 0)
        {
            char texto[] = "\r\nSeleccionaste controlar SPI\r\n";

            HAL_UART_Transmit(&huart2,(uint8_t *)texto, strlen(texto), HAL_MAX_DELAY);

            Submenu_SPI();
        }
        else if (strcmp(opcion, "2") == 0)
        {
            char texto[] = "\r\nSeleccionaste medir sensor I2C\r\n";

            HAL_UART_Transmit(&huart2, (uint8_t *)texto, strlen(texto), HAL_MAX_DELAY);

            HAL_StatusTypeDef estado;
            uint8_t datos[4] = {0, 0, 0, 0};
            uint16_t adcRecibido;
            uint16_t milivoltiosRecibidos;
            char mensaje[100];

            estado = HAL_I2C_Master_Receive(&hi2c1, 0x28 << 1, datos, 4, 100);

            if (estado == HAL_OK)
            {
            	adcRecibido = ((uint16_t)datos[0] << 8) | datos[1];
            	milivoltiosRecibidos = ((uint16_t)datos[2] << 8) | datos[3];

            	snprintf(mensaje, sizeof(mensaje), "\r\nADC: %u bits\r\nVoltaje: %u.%03u V\r\n",
            			(unsigned int)adcRecibido, (unsigned int)(milivoltiosRecibidos / 1000),
						(unsigned int)(milivoltiosRecibidos % 1000));
                HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);

            }
            else
            {
            	snprintf(mensaje, sizeof(mensaje), "\r\nError al recibir datos. Codigo I2C: %lu\r\n", HAL_I2C_GetError(&hi2c1));
                HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);
            }
        }
        else
        {
            char texto[] = "\r\nOpcion no valida\r\n";

            HAL_UART_Transmit(&huart2, (uint8_t *)texto, strlen(texto), HAL_MAX_DELAY);
        }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ESP32_CS_Pin */
  GPIO_InitStruct.Pin = ESP32_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ESP32_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* Envia un texto por la terminal */
static void UART_Print(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/* Lee una linea hasta Enter, con eco y soporte de borrar */
static void UART_ReadLine(char *buf, uint16_t len)
{
  uint16_t i = 0;
  uint8_t c;

  while (1)
  {
    //recibe un caracter a la vez
    if (HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY) != HAL_OK) continue;

    //si llega Enter (13 o 10) se termina la lectura
    if (c == '\r' || c == '\n')
    {
      if (i == 0) continue;          // ignora Enter vacio o el \n despues del \r
      break;
    }
    if (c == '\b' || c == 127)       // Backspace
    {
      if (i > 0) { i--; UART_Print("\b \b"); }
      continue;
    }
    //guarda el caracter si todavia hay espacio
    if (i < len - 1)
    {
      buf[i++] = c;
      HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);   // eco
    }
  }
  buf[i] = '\0';   //cierra el texto
  UART_Print("\r\n");
}

static void Mostrar_Menu(void)//imprime el menu principal
{
  UART_Print("\r\n------ MENU PRINCIPAL ------\r\n");
  UART_Print("1. Controlar dispositivo SPI \r\n");
  UART_Print("2. Obtener medicion del sensor I2C\r\n");
  UART_Print("Seleccione una opcion: ");
}

/* Valida el formato "LED,TIEMPO". Retorna 1 si es valido, 0 si no */
static uint8_t Parse_Comando(const char *s, uint8_t *led, uint16_t *tiempo)//Revisa que el comando sea valido
{
  uint32_t valor = 0;
  uint8_t digitos = 0;

  while (*s == ' ') s++;   //salta espacios al inicio

  /* --- numero de LED (1 digito) --- */
  while (*s >= '0' && *s <= '9')
  {
    valor = valor * 10 + (*s - '0');   //convierte el caracter a numero
    s++;
    if (++digitos > 1) return 0;
  }
  if (digitos == 0 || valor < LED_MIN || valor > LED_MAX) return 0;
  *led = (uint8_t)valor;

  /* --- coma separadora --- */
  while (*s == ' ') s++;
  if (*s != ',') return 0;
  s++;
  while (*s == ' ') s++;

  /* --- tiempo en ms (max 5 digitos) --- */
  valor = 0;
  digitos = 0;
  while (*s >= '0' && *s <= '9')
  {
    valor = valor * 10 + (*s - '0');
    s++;
    if (++digitos > 5) return 0;
  }
  if (digitos == 0) return 0;

  while (*s == ' ') s++;
  if (*s != '\0') return 0;          // no debe haber basura al final

  if (valor < TIEMPO_MIN || valor > TIEMPO_MAX) return 0;
  *tiempo = (uint16_t)valor;
  return 1;
}

/* Envia la trama de 4 bytes al ESP32: [0xA5][LED][T_H][T_L] */
static HAL_StatusTypeDef SPI_Enviar_Comando(uint8_t led, uint16_t tiempo)//Pone el CS en bajo y manda los 4 bytes por SPI
{
  //el tiempo se parte en dos bytes: parte alta y parte baja
  uint8_t tx[SPI_FRAME_LEN] = { SPI_HEADER, led, (uint8_t)(tiempo >> 8), (uint8_t)(tiempo & 0xFF) };
  uint8_t rx[SPI_FRAME_LEN] = {0};
  HAL_StatusTypeDef st;

  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET);   //selecciona al ESP32
  HAL_Delay(1);                                   // tiempo para que el esclavo se prepare
  st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, SPI_FRAME_LEN, 100);
  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);     //libera al ESP32

  return st;
}

/* Cambia el tiempo de la senal cuadrada del TIM2 */
static void Senal_Actualizar(uint16_t tiempo)
{
  //el pin cambia de estado cada vez que el contador llega al periodo
  //entonces el periodo del timer es el tiempo en alto (y el tiempo en bajo)
  __HAL_TIM_SET_AUTORELOAD(&htim2, (uint32_t)tiempo * TIM2_TICKS_MS - 1);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
  //se reinicia el contador para que no se pase del nuevo periodo
  __HAL_TIM_SET_COUNTER(&htim2, 0);
}

static void Submenu_SPI(void)////Pide el compando y lo valida
{
  char linea[RX_BUF_LEN];
  char msg[64];
  uint8_t led;
  uint16_t tiempo;

  while (1)
  {
    UART_Print("\r\n--- CONTROL DE LEDs ---\r\n");
    UART_Print("Formato: LED,TIEMPO_ms \r\n");
    UART_Print("LED: 1-3   Tiempo: 1-60000 ms   (0 = regresar)\r\n");
    UART_Print("> ");
    UART_ReadLine(linea, sizeof(linea));//Lee lo que se escribe hasta presionar enter haceptando backspace y borrar.

    if (strcmp(linea, "0") == 0) return;          // regresar al menu

    if (!Parse_Comando(linea, &led, &tiempo))
    {
      UART_Print("Error: comando invalido. Intente de nuevo.\r\n"); //Manda el texto a la terminal
      continue;
    }

    //parte 5: con el mismo comando se cambia el periodo de la senal cuadrada


    if (SPI_Enviar_Comando(led, tiempo) != HAL_OK)
    {
      UART_Print("Error: fallo la transmision SPI.\r\n");
      return;
    }

    Senal_Actualizar(tiempo);
    snprintf(msg, sizeof(msg), "Senal cuadrada: %u ms alto / %u ms bajo\r\n", tiempo, tiempo);
    UART_Print(msg);

    snprintf(msg, sizeof(msg), "Enviado: LED %u encendido por %u ms...\r\n", led, tiempo);
    UART_Print(msg);

    UART_Print("Operacion finalizada.\r\n");
    return;                                        // regresa al menu principal
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
