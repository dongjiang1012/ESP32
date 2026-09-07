idf_component_register(
    SRCS
         main.c
         drivers/i2c_device_0x3c.c
         drivers/sw6306_register_access.c
         drivers/sw6306_adc.c
         drivers/sw6306_fuel_gauge.c
         drivers/sw6306_fast_charge_status.c
         drivers/sw6306_fault_status.c
         drivers/sw6306_startup_config.c
         drivers/sw6306_typec_mode.c
         drivers/sw6306_power_config.c
         config/sw6306_config_model.c
         config/sw6306_config_validate.c
         services/sw6306_config_service.c
         services/sw6306_config_worker.c
         services/sw6306_diagnostics.c
         services/sw6306_data_service.c
         data/data_scheduler.c
         data/data_snapshot.c
         ui/ui_app.c
         ui/ui_page_manager.c
         ui/ui_common.c
         ui/ui_fonts.c
         ui/ui_numeric_input.c
         ui/ui_power_chart.c
         ui/ui_fault_analysis.c
         ui/ui_output_control.c
         ui/ui_buckboost_config.c
         ui/ui_system_settings.c
         ui/ui_brightness_panel.c
         ui/fonts/ui_font_cn_16.c
         ui/fonts/ui_font_cn_24.c
    INCLUDE_DIRS . drivers config services data ui
    REQUIRES esp32_p4_wifi6_touch_lcd_7b lvgl__lvgl esp_driver_i2c)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
)
