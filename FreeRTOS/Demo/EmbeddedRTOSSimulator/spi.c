#include "spi.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

struct spi_state g_spi;

void spi_init(spi_mode_t mode) {
    memset(&g_spi, 0, sizeof(g_spi));
    g_spi.mode = mode;
    g_spi.rx_queue = xQueueCreate(SPI_BUFFER_SIZE, sizeof(char));
    printf("[SPI] Initialized (ARMv8A emu, mode=%s, RX queue size %d).\n", mode == SPI_MODE_MASTER ? "MASTER" : "SLAVE", SPI_BUFFER_SIZE);
}

void spi_transfer(const char *tx, char *rx, int len) {
    // Simulate SPI transfer: master sends, slave receives (loopback for demo)
    for (int i = 0; i < len && tx[i] != '\0'; ++i) {
        size_t next_tx = (g_spi.tx_head + 1) % SPI_BUFFER_SIZE;
        if (next_tx == g_spi.tx_tail) {
            printf("[SPI] TX buffer full, dropping data.\n");
            break;
        }
        g_spi.tx_buffer[g_spi.tx_head] = tx[i];
        g_spi.tx_head = next_tx;
        // For demo, echo back to RX
        size_t next_rx = (g_spi.rx_head + 1) % SPI_BUFFER_SIZE;
        if (next_rx != g_spi.rx_tail) {
            g_spi.rx_buffer[g_spi.rx_head] = tx[i];
            g_spi.rx_head = next_rx;
            xQueueSend(g_spi.rx_queue, &tx[i], 0);
        }
        if (rx && i < len) rx[i] = tx[i];
    }
    if (rx) rx[len-1] = '\0';
    board_reg_write(BOARD_REG_SPI, 1); // Simulate SPI transfer complete
    printf("[SPI] Transfer: TX=%s RX=%s\n", tx, rx ? rx : "");
}

void spi_simulate_rx_event(const char *data) {
    // Simulate incoming data (from other device)
    size_t len = strlen(data);
    for (size_t i = 0; i < len; ++i) {
        size_t next = (g_spi.rx_head + 1) % SPI_BUFFER_SIZE;
        if (next == g_spi.rx_tail) {
            printf("[SPI] RX buffer full, dropping data.\n");
            break;
        }
        g_spi.rx_buffer[g_spi.rx_head] = data[i];
        g_spi.rx_head = next;
        xQueueSend(g_spi.rx_queue, &data[i], 0);
    }
    board_reg_write(BOARD_REG_SPI, 2); // Simulate RX ready
    printf("[SPI] Simulated RX event: %s\n", data);
}