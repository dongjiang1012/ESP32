#pragma once
#include "lvgl.h"

typedef enum {
    UI_PAGE_MONITOR,
    UI_PAGE_OUTPUT,
    UI_PAGE_BUCKBOOST,
    UI_PAGE_SYSTEM,
    UI_PAGE_FAULT_ANALYSIS,
    UI_PAGE_COUNT,
} ui_page_id_t;

/* Pages live for the application's lifetime, preserving drafts and chart data. */
void ui_page_manager_create(lv_obj_t *screen, void (*on_show)(ui_page_id_t));
lv_obj_t *ui_page_manager_root(ui_page_id_t page);
void ui_page_manager_show(ui_page_id_t page);
ui_page_id_t ui_page_manager_current(void);
