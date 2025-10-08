#pragma once

// Adjust these to match your board later.
// Defaults chosen for NUCLEO-F722ZE bring-up.

// LED (status)
#define LED_GPIO_Port      GPIOA
#define LED_Pin            GPIO_PIN_5

// UART (CLI)
#define UARTx              USART2
// PA2 (TX), PA3 (RX) on Nucleo by default
#define UART_TX_GPIO_Port  GPIOA
#define UART_TX_Pin        GPIO_PIN_2
#define UART_RX_GPIO_Port  GPIOA
#define UART_RX_Pin        GPIO_PIN_3
#define UART_AF            GPIO_AF7_USART2
#define UART_BAUD          115200

// I2C (SCD41)
#define I2C_USED           I2C1
// PB8 (SCL), PB9 (SDA)
#define I2C_SCL_Port       GPIOB
#define I2C_SCL_Pin        GPIO_PIN_8
#define I2C_SDA_Port       GPIOB
#define I2C_SDA_Pin        GPIO_PIN_9
#define I2C_AF             GPIO_AF4_I2C1
#define I2C_SPEED          100000U

// PWM Fans (Timer3 CH1..CH4)
// CH1: PA6, CH2: PA7, CH3: PB0, CH4: PB1 (common & available)
#define FAN_TIM                       TIM3
#define FAN_TIM_AF                    GPIO_AF2_TIM3
#define FAN0_GPIO_Port                GPIOA
#define FAN0_Pin                      GPIO_PIN_6   // CH1
#define FAN1_GPIO_Port                GPIOA
#define FAN1_Pin                      GPIO_PIN_7   // CH2
#define FAN2_GPIO_Port                GPIOB
#define FAN2_Pin                      GPIO_PIN_0   // CH3
#define FAN3_GPIO_Port                GPIOB
#define FAN3_Pin                      GPIO_PIN_1   // CH4
#define FAN_PWM_FREQUENCY_HZ          25000U       // 25kHz for fans
#define FAN_PWM_RESOLUTION_STEPS      1000U        // 0..1000 = 0..100%

// Control/limits
#define ARM_SOFTSTART_MS              300U
#define DUTY_RAMP_PER_10MS            30U   // max change per 10ms tick (0..1000)
