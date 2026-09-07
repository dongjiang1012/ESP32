#pragma once

#include "data_snapshot.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *summary;
    lv_obj_t *raw;
    lv_obj_t *events;
    lv_obj_t *discharge;
    lv_obj_t *charge;
    lv_obj_t *other;
    uint32_t last_update_count;
    bool has_sample;
} ui_fault_analysis_t;

void ui_fault_analysis_create(ui_fault_analysis_t *view, lv_obj_t *parent);
void ui_fault_analysis_refresh(ui_fault_analysis_t *view,
                               const data_snapshot_t *snapshot);
