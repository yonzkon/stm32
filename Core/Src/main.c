#include "main.h"
#include <string.h>
#include <unistd.h>
#include <ringbuf.h>
#include <log.h>
#include <modbus.h>

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
struct ringbuf *huart2_rxbuf;

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PC13
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PA0
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    // PA1
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
}

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
        Error_Handler();
    huart2_rxbuf = ringbuf_new(0);
    HAL_UARTEx_ReceiveToIdle_IT(
        &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
        ringbuf_spare(huart2_rxbuf));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    LOG_INFO("cplt");
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    LOG_INFO("halfcplt");
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
        LOG_ERROR("huart1");
    else if (huart == &huart2)
        LOG_ERROR("huart2");

    if (ringbuf_spare_right(huart2_rxbuf) > 0) {
        HAL_UARTEx_ReceiveToIdle_IT(
            &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
            ringbuf_spare_right(huart2_rxbuf));
    } else {
        HAL_UARTEx_ReceiveToIdle_IT(
            &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
            ringbuf_spare_left(huart2_rxbuf));
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    ringbuf_write_advance(huart2_rxbuf, Size);
    if (ringbuf_spare_right(huart2_rxbuf) > 0) {
        HAL_UARTEx_ReceiveToIdle_IT(
            &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
            ringbuf_spare_right(huart2_rxbuf));
    } else {
        HAL_UARTEx_ReceiveToIdle_IT(
            &huart2, (uint8_t *)ringbuf_write_pos(huart2_rxbuf),
            ringbuf_spare_left(huart2_rxbuf));
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }
    }
}

extern int apistt_init();
extern int apistt_fini();
extern int apistt_loop();

void init(void)
{
    log_set_level(LOG_LV_DEBUG);
    apistt_init();
    LOG_INFO("system initial finished, start main loop ...");

    modbus_t *ctx = modbus_new_rtu("uart2", 115200, 'N', 8, 0);
    modbus_set_slave(ctx, 12);
    modbus_mapping_t *map = modbus_mapping_new(500, 500, 500, 500);
    modbus_connect(ctx);

    while (1) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        memset(query, 0, sizeof(query));
        int rc = modbus_receive(ctx, query);
        if (rc >= 0) {
            modbus_reply(ctx, query, rc, map);
            LOG_INFO("modbus reply");
        } else {
            LOG_INFO("modbus receive failed");
        }

        //apistt_loop();

        usleep(100 * 1000 * 1);
    }

    LOG_INFO("exit main loop ...");
    apistt_fini();
}

extern void fenix_main();

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    fenix_main(init);
}
