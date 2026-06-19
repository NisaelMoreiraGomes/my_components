#include "display-st7789.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY (5 * 1000)

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static gpio_num_t blk_pin = GPIO_NUM_NC;

static esp_err_t validate_pins(const gpio_num_t *pins, size_t count);
static esp_err_t validate_miso_pin(const gpio_num_t pin);
static esp_err_t free_panel_handle(void);
static esp_err_t free_io_handle(void);

esp_err_t display_init(const display_config_t *config)
{
    if (!config)
        return ESP_ERR_INVALID_ARG;

    esp_err_t result;

    gpio_num_t pins[] = {
        config->mosi,
        config->sclk,
        config->cs,
        config->dc,
        config->rst,
        config->blk};

    result = validate_pins(pins, sizeof(pins) / sizeof(pins[0]));
    if (result != ESP_OK)
        return result;

    blk_pin = config->blk;

    result = validate_miso_pin(config->miso);
    if (result != ESP_OK)
        return result;

    spi_bus_config_t buscfg = {
        .sclk_io_num = config->sclk,
        .mosi_io_num = config->mosi,
        .miso_io_num = config->miso,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = config->width * config->height * sizeof(uint16_t),
    };

    result = spi_bus_initialize(config->host_id, &buscfg, SPI_DMA_CH_AUTO);
    if (result != ESP_OK)
        return result;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = config->dc,
        .cs_gpio_num = config->cs,
        .pclk_hz = config->pclk_hz,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };

    result = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->host_id, &io_config, &io_handle);
    if (result != ESP_OK)
    {
        free_io_handle();
        return result;
    }

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rst,

        .rgb_ele_order = config->is_bgr ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,

        .bits_per_pixel = 16,
    };

    result = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    if (result != ESP_OK)
    {
        free_panel_handle();
        return result;
    }

    result = esp_lcd_panel_reset(panel_handle);
    if (result != ESP_OK)
        return result;

    result = esp_lcd_panel_init(panel_handle);
    if (result != ESP_OK)
        return result;

    result = esp_lcd_panel_invert_color(panel_handle, config->is_inverted_color);
    if (result != ESP_OK)
        return result;

    if (config->is_horizontal)
    {
        result = esp_lcd_panel_swap_xy(panel_handle, true);
        if (result != ESP_OK)
            return result;

        result = esp_lcd_panel_mirror(panel_handle, false, true);
        if (result != ESP_OK)
            return result;
    }

    result = esp_lcd_panel_set_gap(panel_handle, config->offset_x, config->offset_y);
    if (result != ESP_OK)
        return result;

    result = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (result != ESP_OK)
        return result;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK};

    result = ledc_timer_config(&ledc_timer);
    if (result != ESP_OK)
        return result;

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = blk_pin,
        .duty = 0,
        .hpoint = 0};

    result = ledc_channel_config(&ledc_channel);
    if (result != ESP_OK)
        return result;

    result = display_backlight_on();
    if (result != ESP_OK)
        return result;

    return ESP_OK;
}

esp_err_t display_deinit(void)
{
    esp_err_t result_panel = free_panel_handle();
    esp_err_t result_io = free_io_handle();

    if (result_panel != ESP_OK)
        return result_panel;

    if (result_io != ESP_OK)
        return result_io;

    return ESP_OK;
}

esp_err_t display_backlight_on(void)
{
    if (blk_pin != GPIO_NUM_NC)
        return display_set_brightness(255);

    return ESP_ERR_INVALID_STATE;
}

esp_err_t display_backlight_off(void)
{
    if (blk_pin != GPIO_NUM_NC)
        return display_set_brightness(0);

    return ESP_ERR_INVALID_STATE;
}

esp_err_t display_set_brightness(uint8_t brightness)
{
    if (blk_pin == GPIO_NUM_NC)
        return ESP_ERR_INVALID_STATE;

    esp_err_t result;

    result = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness);
    if (result != ESP_OK)
        return result;

    return ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

esp_err_t display_draw_bitmap(int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    if (!panel_handle)
        return ESP_ERR_INVALID_STATE;

    return esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
}

esp_lcd_panel_handle_t display_get_panel_handle(void)
{
    return panel_handle;
}

static esp_err_t validate_pins(const gpio_num_t *pins, size_t count)
{
    if (!pins || count == 0)
        return ESP_ERR_INVALID_ARG;

    for (size_t i = 0; i < count; i++)
    {
        gpio_num_t pin = pins[i];

        if (pin == GPIO_NUM_NC)
            return ESP_ERR_INVALID_ARG;

        if (!GPIO_IS_VALID_GPIO(pin))
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static esp_err_t validate_miso_pin(const gpio_num_t pin)
{
    if (pin == GPIO_NUM_NC)
        return ESP_OK;

    return GPIO_IS_VALID_GPIO(pin) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t free_panel_handle(void)
{
    esp_err_t result = ESP_OK;

    if (panel_handle)
    {
        result = esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }

    return result;
}

static esp_err_t free_io_handle(void)
{
    esp_err_t result = ESP_OK;

    if (io_handle)
    {
        result = esp_lcd_panel_io_del(io_handle);
        io_handle = NULL;
    }

    return result;
}
