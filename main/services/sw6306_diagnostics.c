#include "sw6306_diagnostics.h"
#include "sw6306_register_access.h"
#include "sw6306_power_config.h"
#include "sw6306_adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static void status(void)
{
    const uint16_t regs[] = {0x18,0x19,0x1c,0x1d,0x28,0x2a,0x2b,0x2c,
        0x40,0x41,0x42,0x43,0x44,0x100,0x101,0x107,0x108,0x118,0x132};
    if (sw6306_register_access_begin() != ESP_OK) return;
    for (unsigned i=0;i<sizeof(regs)/sizeof(regs[0]);i++) {
        uint8_t v=0;
        esp_err_t err=sw6306_register_read(regs[i],&v);
        printf("SW REG %03X=%02X %s\n",regs[i],v,esp_err_to_name(err));
    }
    esp_err_t err=sw6306_adc_read_all();
    if(err==ESP_OK) printf("SW ADC VBUS=%ld IBUS=%ld VBAT=%ld IBAT=%ld DIE=%ld\n",
        (long)g_sw6306_adc_data.bus_voltage_mv.value,
        (long)g_sw6306_adc_data.bus_current_ma.value,
        (long)g_sw6306_adc_data.battery_voltage_mv.value,
        (long)g_sw6306_adc_data.battery_current_ma.value,
        (long)g_sw6306_adc_data.chip_temperature_mc.value);
    else printf("SW ADC ERROR %s\n",esp_err_to_name(err));
    sw6306_register_access_end();
}

static void command(const char *line)
{
    esp_err_t err=ESP_OK;
    if(strcmp(line,"sw status")==0) {status();return;}
    if(strcmp(line,"sw 12v")==0 || strcmp(line,"sw off")==0) {
        sw6306_output_config_t c;
        err=sw6306_register_access_begin();
        if(err!=ESP_OK) return;
        err=sw6306_power_read_output(&c);
        if(err==ESP_OK) {
            c.force_voltage=strcmp(line,"sw 12v")==0;
            c.voltage_mv=12000;
            err=sw6306_power_write_output(&c);
        }
        sw6306_register_access_end();
        printf("SW RESULT %s %s\n",line,esp_err_to_name(err));
        status();
    } else printf("SW commands: sw status | sw 12v | sw off\n");
}

static void task(void *arg)
{
    (void)arg;
    char line[64];size_t used=0;
    printf("SW diagnostics ready; output is not automatically enabled\n");
    status();
    for(;;) {
        int ch=getchar();
        if(ch==EOF) {clearerr(stdin);vTaskDelay(pdMS_TO_TICKS(20));continue;}
        if(ch=='\r'||ch=='\n') {
            if(used) {line[used]=0;command(line);used=0;}
        } else if(used<sizeof(line)-1) line[used++]=(char)ch;
        else used=0;
    }
}

esp_err_t sw6306_diagnostics_init(void)
{
    return xTaskCreate(task,"sw6306_diag",6144,NULL,2,NULL)==pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
