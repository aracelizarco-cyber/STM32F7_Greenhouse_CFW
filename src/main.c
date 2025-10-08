#include "stm32f7xx_hal.h"
#include "pins.h"
#include "uart_proto.h"
#include "fan.h"
#include "scd41.h"
#include <string.h>
#include <stdbool.h>

// Prototypes
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART_Init(void);
static void MX_I2C_Init(void);
static void Error_Handler(void);

// Globals
UART_HandleTypeDef huart2;
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim3;

uint32_t millis(void) {
    return HAL_GetTick();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART_Init();
    MX_I2C_Init();

    uart_proto_init();
    fan_init();

#if SCD41_ENABLE
    scd41_init();
    scd41_start_periodic();
#endif

    // 10ms tick for ramping
    uint32_t t10 = millis();
    // 5s sensor cadence
    uint32_t t_sense = millis();

    uart_proto_printf("Greenhouse F7 Starter - Ready\r\n");

    while (1) {
        uart_proto_poll();

        uint32_t now = millis();
        if ((int32_t)(now - t10) >= 10) {
            fan_tick_10ms();
            t10 += 10;
        }

#if SCD41_ENABLE
        if ((int32_t)(now - t_sense) >= 5000) {
            t_sense += 5000;
            scd41_sample_t s;
            if (scd41_read_measurement(&s) && s.valid) {
                uart_proto_printf("SENS CO2=%u ppm T=%d.%02d C RH=%u.%02u %%\r\n",
                    s.co2_ppm,
                    s.temp_c_x100 / 100, (int)(s.temp_c_x100 % 100),
                    s.rh_x100 / 100, s.rh_x100 % 100);
            } else {
                uart_proto_printf("SENS NODATA\r\n");
            }
        }
#endif
    }
}

/* Clock config: 216MHz SYSCLK (HSE=8MHz) typical for F7 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 432;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType           = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK
                                          | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource        = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider       = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider      = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider      = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};

    // LED
    g.Pin   = LED_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &g);

    // UART pins (PA2 TX, PA3 RX)
    g.Pin       = UART_TX_Pin | UART_RX_Pin;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = UART_AF;
    HAL_GPIO_Init(UART_TX_GPIO_Port, &g);

    // PWM pins (set AF per pin)
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = FAN_TIM_AF;

    g.Pin = FAN0_Pin; HAL_GPIO_Init(FAN0_GPIO_Port, &g);
    g.Pin = FAN1_Pin; HAL_GPIO_Init(FAN1_GPIO_Port, &g);
    g.Pin = FAN2_Pin; HAL_GPIO_Init(FAN2_GPIO_Port, &g);
    g.Pin = FAN3_Pin; HAL_GPIO_Init(FAN3_GPIO_Port, &g);

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

static void MX_USART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance          = UARTx;
    huart2.Init.BaudRate     = UART_BAUD;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_I2C_Init(void)
{
#if SCD41_ENABLE
    __HAL_RCC_I2C1_CLK_ENABLE();

    hi2c1.Instance             = I2C_USED;
    hi2c1.Init.Timing          = 0x20404768; // ~100kHz for 216MHz sys (tune as needed)
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
#else
    (void)hi2c1;
#endif
}

static void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(200);
    }
}
