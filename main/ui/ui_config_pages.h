#pragma once
#include "ui_common.h"
#include "sw6306_config_worker.h"

typedef void (*ui_config_request_cb_t)(sw6306_config_operation_t operation);
typedef struct {
    lv_obj_t *root;
    lv_obj_t *form;
    lv_obj_t *status;
    lv_obj_t *actual;
    lv_obj_t *apply;
    lv_obj_t *reload;
    lv_obj_t *discard;
    sw6306_config_model_t *model;
    ui_config_request_cb_t request;
} ui_config_page_t;

typedef struct {
    ui_config_page_t base;
    lv_obj_t *force_voltage;
    lv_obj_t *preset_22v;
    lv_obj_t *force_ibus;
    lv_obj_t *values[3];
    lv_obj_t *live;
} ui_output_page_t;

typedef struct {
    ui_config_page_t base;
    lv_obj_t *options[4];
    lv_obj_t *pwm;
    lv_obj_t *defaults;
} ui_buck_page_t;

typedef struct {
    ui_config_page_t base;
    lv_obj_t *roles;
    lv_obj_t *brightness;
    lv_obj_t *brightness_value;
} ui_system_page_t;

void ui_output_control_create(ui_output_page_t *page, lv_obj_t *parent,
                              sw6306_config_model_t *model, ui_config_request_cb_t request);
void ui_output_control_refresh(ui_output_page_t *page, bool busy);
void ui_buckboost_config_create(ui_buck_page_t *page, lv_obj_t *parent,
                               sw6306_config_model_t *model, ui_config_request_cb_t request);
void ui_buckboost_config_refresh(ui_buck_page_t *page, bool busy);
void ui_system_settings_create(ui_system_page_t *page, lv_obj_t *parent,
                               sw6306_config_model_t *model, ui_config_request_cb_t request);
void ui_system_settings_refresh(ui_system_page_t *page, bool busy);
