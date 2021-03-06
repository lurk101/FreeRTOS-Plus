/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

#include "my_debug.h"
#include "sd_card.h"
#include "sd_spi.h"
#include "spi.h"

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf  // task_printf

void sd_spi_go_high_frequency(sd_card_t *pSD) {
    uint actual = spi_set_baudrate(pSD->spi->hw_inst, pSD->spi->baud_rate);
    // TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, (long)actual);
}

void sd_spi_go_low_frequency(sd_card_t *pSD) {
    uint actual = spi_set_baudrate(pSD->spi->hw_inst, 100 * 1000);
    // TRACE_PRINTF("%s: Actual frequency: %lu\n", __FUNCTION__, (long)actual);
}

// Would do nothing if pSD->ss_gpio were set to GPIO_FUNC_SPI.
static void sd_spi_select(sd_card_t *pSD) {
    gpio_put(pSD->ss_gpio, 0);
    // A fill byte seems to be necessary, sometimes:
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(pSD->spi->hw_inst, &fill, 1);
}

static void sd_spi_deselect(sd_card_t *pSD) {
    gpio_put(pSD->ss_gpio, 1);
    /*
    MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
    means there is a posibility of bus conflict with MMC/SDC and another SPI
    slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
    line, the master device needs to send a byte after the CS signal is
    deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    spi_write_blocking(pSD->spi->hw_inst, &fill, 1);
}

void sd_spi_acquire(sd_card_t *pSD) {
    spi_lock(pSD->spi);
    sd_spi_select(pSD);
}

void sd_spi_release(sd_card_t *pSD) {
    sd_spi_deselect(pSD);
    spi_unlock(pSD->spi);
}

uint8_t sd_spi_write(sd_card_t *pSD, const uint8_t value) {
    // TRACE_PRINTF("%s\n", __FUNCTION__);
    u_int8_t received = SPI_FILL_CHAR;

    configASSERT(xTaskGetCurrentTaskHandle() == pSD->spi->owner);

    int num = spi_write_read_blocking(pSD->spi->hw_inst, &value, &received, 1);
    configASSERT(1 == num);
    return received;
}

bool sd_spi_transfer(sd_card_t *pSD, const uint8_t *tx, uint8_t *rx,
                     size_t length) {
    return spi_transfer(pSD->spi, tx, rx, length);
}

void sd_spi_init_pl022(sd_card_t *pSD) {
    // Let the PL022 SPI handle it.
    // the CS line is brought high between each byte during transmission.
    gpio_set_function(pSD->ss_gpio, GPIO_FUNC_SPI);
}

