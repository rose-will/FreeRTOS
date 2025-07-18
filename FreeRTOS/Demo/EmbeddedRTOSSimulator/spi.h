#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "queue.h"

#define SPI_BUFFER_SIZE 128

typedef enum {
    SPI_MODE_MASTER = 0,
    SPI_MODE_SLAVE = 1
} spi_mode_t;

// SPI state structure
struct spi_state {
    spi_mode_t mode;
    char tx_buffer[SPI_BUFFER_SIZE];
    size_t tx_head, tx_tail;
    char rx_buffer[SPI_BUFFER_SIZE];
    size_t rx_head, rx_tail;
    QueueHandle_t rx_queue; // For task notification
};

extern struct spi_state g_spi;

void spi_init(spi_mode_t mode);
void spi_transfer(const char *tx, char *rx, int len);

// Simulate SPI RX event (data received from other device)
void spi_simulate_rx_event(const char *data);

#endif // SPI_H