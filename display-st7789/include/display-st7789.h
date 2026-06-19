#pragma once

#include "driver/gpio.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief SPI pixel clock frequency presets.
 */
#define DISPLAY_PCLK_HZ_10 (10 * 1000 * 1000)
#define DISPLAY_PCLK_HZ_20 (20 * 1000 * 1000)
#define DISPLAY_PCLK_HZ_40 (40 * 1000 * 1000)
#define DISPLAY_PCLK_HZ_60 (60 * 1000 * 1000)
#define DISPLAY_PCLK_HZ_80 (80 * 1000 * 1000)

/**
 * @brief Pack three color components into a 16-bit color value.
 *
 * The macro does not assign semantic meaning to the components.
 * The first component is encoded using 5 bits, the second using
 * 6 bits, and the third using 5 bits.
 *
 * Expected ranges:
 * - c1: 0-31
 * - c2: 0-63
 * - c3: 0-31
 *
 * @param c1 First 5-bit component.
 * @param c2 Second 6-bit component.
 * @param c3 Third 5-bit component.
 *
 * @return Packed 16-bit color value.
 */
#define DISPLAY_COLOR_565(c1, c2, c3)               \
    ((uint16_t)(((((uint16_t)(c1)) & 0x1F) << 11) | \
                ((((uint16_t)(c2)) & 0x3F) << 5) |  \
                ((((uint16_t)(c3)) & 0x1F))))

/**
 * @brief Convert three 8-bit color components to a packed 16-bit color value.
 *
 * The macro does not assign semantic meaning to the components.
 * The first and third components are reduced to 5 bits, while
 * the second component is reduced to 6 bits.
 *
 * Expected ranges:
 * - c1: 0-255
 * - c2: 0-255
 * - c3: 0-255
 *
 * @param c1 First 8-bit component.
 * @param c2 Second 8-bit component.
 * @param c3 Third 8-bit component.
 *
 * @return Packed 16-bit color value.
 */
#define DISPLAY_COLOR_255_TO_565(c1, c2, c3) \
    DISPLAY_COLOR_565(                       \
        ((uint16_t)(c1) >> 3),               \
        ((uint16_t)(c2) >> 2),               \
        ((uint16_t)(c3) >> 3))

    /**
     * @brief ST7789 display configuration.
     *
     * This structure contains all hardware and display parameters
     * required to initialize the display driver.
     */
    typedef struct
    {
        /** SPI host peripheral used by the display. */
        spi_host_device_t host_id;

        /** SPI pixel clock frequency in Hz. */
        uint32_t pclk_hz;

        /** MOSI GPIO pin. */
        gpio_num_t mosi;

        /** MISO GPIO pin (optional, can be unused). */
        gpio_num_t miso;

        /** SPI clock GPIO pin. */
        gpio_num_t sclk;

        /** Chip Select GPIO pin. */
        gpio_num_t cs;

        /** Data/Command GPIO pin. */
        gpio_num_t dc;

        /** Hardware reset GPIO pin. */
        gpio_num_t rst;

        /** Backlight control GPIO pin. */
        gpio_num_t blk;

        /** Display width in pixels. */
        uint16_t width;

        /** Display height in pixels. */
        uint16_t height;

        /** Horizontal display offset. */
        uint16_t offset_x;

        /** Vertical display offset. */
        uint16_t offset_y;

        /** True if the display should be initialized in landscape mode. */
        bool is_horizontal;

        /** True to invert display colors. */
        bool is_inverted_color;

        /** True if the display uses BGR color order. */
        bool is_bgr;
    } display_config_t;

    /**
     * @brief Initialize the display driver.
     *
     * Configures the SPI bus, creates the ST7789 panel instance,
     * initializes the display, and prepares the backlight control.
     *
     * @param config Pointer to the display configuration structure.
     *
     * @return
     *
     *      - ESP_OK on success
     *      - An error code otherwise
     */
    esp_err_t display_init(const display_config_t *config);

    /**
     * @brief Deinitialize the display driver.
     *
     * Releases resources allocated by the display driver.
     *
     * @return
     *
     *      - ESP_OK on success
     *      - An error code otherwise
     */
    esp_err_t display_deinit(void);

    /**
     * @brief Turn the display backlight on.
     *
     * @return
     *
     *      - ESP_OK on success
     *      - An error code otherwise
     */
    esp_err_t display_backlight_on(void);

    /**
     * @brief Turn the display backlight off.
     *
     * @return
     *
     *      - ESP_OK on success
     *      - An error code otherwise
     */
    esp_err_t display_backlight_off(void);

    /**
     * @brief Set the display backlight brightness.
     *
     * @param brightness Brightness value from 0 (off) to 255 (maximum).
     *
     * @return
     *
     *      - ESP_OK on success
     *      - ESP_ERR_INVALID_STATE if backlight is not initialized
     */
    esp_err_t display_set_brightness(uint8_t brightness);

    /**
     * @brief Draw a bitmap to the display.
     *
     * Transfers pixel data to the specified display area.
     * This function is typically used by graphics libraries
     * such as LVGL to update the screen using DMA transfers.
     *
     * @param x_start Left coordinate of the update area.
     * @param y_start Top coordinate of the update area.
     * @param x_end Right coordinate of the update area (exclusive).
     * @param y_end Bottom coordinate of the update area (exclusive).
     * @param color_data Pointer to the pixel buffer.
     *
     * @return
     *
     *      - ESP_OK on success
     *      - An error code otherwise
     */
    esp_err_t display_draw_bitmap(int x_start, int y_start, int x_end, int y_end, const void *color_data);

    /**
     * @brief Get the native ESP-IDF panel handle.
     *
     * Allows external libraries or application code to directly
     * access and control the underlying LCD panel driver.
     *
     * @return Native panel handle.
     */
    esp_lcd_panel_handle_t display_get_panel_handle(void);

#ifdef __cplusplus
}
#endif
