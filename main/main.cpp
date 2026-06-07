/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Include ----------------------------------------------------------------- */
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_idf_version.h"

#include <stdio.h>

#include "ei_device_espressif_esp32.h"

#include "ei_at_handlers.h"
#include "ei_classifier_porting.h"
#include "ei_run_impulse.h"

#include "ei_analogsensor.h"
#include "ei_inertial_sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#define RED_LED_PIN GPIO_NUM_21
#define WHITE_LED_PIN GPIO_NUM_22

EiDeviceInfo *EiDevInfo = dynamic_cast<EiDeviceInfo *>(EiDeviceESP32::get_device());
static ATServer *at;

/* Private variables ------------------------------------------------------- */

/* Public functions -------------------------------------------------------- */

void setup_led() {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_rom_gpio_pad_select_gpio(RED_LED_PIN);
    esp_rom_gpio_pad_select_gpio(WHITE_LED_PIN);
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    gpio_pad_select_gpio(RED_LED_PIN);
    gpio_pad_select_gpio(WHITE_LED_PIN);
#endif
    gpio_set_direction(RED_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(WHITE_LED_PIN, GPIO_MODE_OUTPUT);
}

static const char *TAG = "MY_TASK";

void blink_task(void *pvParameters){
	
	while(1){
		gpio_set_level(RED_LED_PIN, 0);
		gpio_set_level(WHITE_LED_PIN, 0);
		ei_printf("RED LED ON\r\n");
		vTaskDelay(pdMS_TO_TICKS(1000));

		gpio_set_level(RED_LED_PIN, 1);
		gpio_set_level(WHITE_LED_PIN, 1);
		ei_printf("RED LED OFF\r\n");
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

extern "C" int app_main()
{
    setup_led();

    // GPIO 21, 22는 LED와 I2C(가속도계)가 공유하므로,
    // blink 테스트 시에는 ei_inertial_init() 비활성화 필요
   // BaseType_t res = xTaskCreate(blink_task, "blink_task", 4096, nullptr, 5, nullptr);

    // if(res == pdPASS){
    //     ei_printf("Task created successfully\r\n");
    // }
    // else{
    //     ei_printf("Task create failed\r\n");
    // }

    /* Initialize Edge Impulse sensors and commands */

    EiDeviceESP32* dev = static_cast<EiDeviceESP32*>(EiDeviceESP32::get_device());

    ei_printf(
        "Hello from Edge Impulse Device SDK.\r\n"
        "Compiled on %s %s\r\n",
        __DATE__,
        __TIME__);

    /* Setup the inertial sensor */
    // GPIO 21/22 핀 충돌: 가속도계 I2C와 LED가 같은 핀 사용
    // blink 테스트 중에는 아래 줄을 주석처리
    /* if (ei_inertial_init() == false) {
        ei_printf("Inertial sensor initialization failed\r\n");
    } */

    /* Setup the analog sensor */
    if (ei_analog_sensor_init() == false) {
        ei_printf("ADC sensor initialization failed\r\n");
    }else{
    	ei_printf("ABC sensor initialization success\r\n");
   	}
	
    at = ei_at_init(dev);
    ei_printf("Type AT+HELP to see a list of commands.\r\n");
    at->print_prompt();

    dev->set_state(eiStateFinished);

    while(1){
        /* handle command comming from uart */
        char data = ei_get_serial_byte();

        while (data != 0xFF) {
            at->handle(data);
            data = ei_get_serial_byte();
        }
    }
}