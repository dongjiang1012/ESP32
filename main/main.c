/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "i2c_device_0x3c.h"
#include "sw6306_register_access.h"
#include "services/sw6306_config_service.h"
#include "services/sw6306_config_worker.h"
#include "services/sw6306_diagnostics.h"
#include "ui/ui_app.h"
#include "data/data_scheduler.h"
#include "data/data_snapshot.h"

#define COLLECTOR_TASK_STACK_SIZE (4096U)
#define COLLECTOR_TASK_PRIORITY   (4U)

static TaskHandle_t s_collector_task;

void app_main(void)
{
    // 初始化显示和触摸设备。
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_180,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    lv_display_t *display = bsp_display_start_with_config(&cfg);
    ESP_ERROR_CHECK(display != NULL ? ESP_OK : ESP_FAIL);

    ESP_ERROR_CHECK(i2c_device_0x3c_init());
    ESP_ERROR_CHECK(sw6306_register_access_init());
    ESP_ERROR_CHECK(sw6306_config_service_apply_startup());
    ESP_ERROR_CHECK(data_snapshot_init());
    ESP_ERROR_CHECK(sw6306_config_worker_init());
    ESP_ERROR_CHECK(xTaskCreate(data_collector_task,
                                "sw6306_collector",
                                COLLECTOR_TASK_STACK_SIZE,
                                NULL,
                                COLLECTOR_TASK_PRIORITY,
                                &s_collector_task) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);

    bsp_display_backlight_on();

    ESP_ERROR_CHECK(bsp_display_lock(-1) ? ESP_OK : ESP_ERR_TIMEOUT);

    // 在持有 LVGL 显示锁时创建正式应用界面。
    ui_app_create();

    bsp_display_unlock();
    ESP_ERROR_CHECK(sw6306_diagnostics_init());
}
