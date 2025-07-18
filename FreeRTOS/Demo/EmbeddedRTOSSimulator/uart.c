#include "uart.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

struct uart_state g_uart;

// Forward declaration for event callback
static void uart_rx_event_cb(void *context);

void uart_init(void) {
    memset(&g_uart, 0, sizeof(g_uart));
    g_uart.rx_queue = xQueueCreate(UART_RX_BUFFER_SIZE, sizeof(char));
    board_register_event(uart_rx_event_cb, NULL);
    printf("[UART] Initialized (ARMv8A emu, RX queue size %d).\n", UART_RX_BUFFER_SIZE);
}

void uart_send(const char *data) {
    // Simulate writing to TX buffer and hardware register
    size_t len = strlen(data);
    for (size_t i = 0; i < len; ++i) {
        size_t next = (g_uart.tx_head + 1) % UART_TX_BUFFER_SIZE;
        if (next == g_uart.tx_tail) {
            printf("[UART] TX buffer full, dropping data.\n");
            break;
        }
        g_uart.tx_buffer[g_uart.tx_head] = data[i];
        g_uart.tx_head = next;
    }
    board_reg_write(BOARD_REG_UART, 1); // Simulate TX ready
    printf("[UART] Send: %s\n", data);
}

void uart_receive(char *buffer, int maxlen) {
    // Read from RX queue (blocking)
    int i = 0;
    char ch;
    while (i < maxlen - 1 && xQueueReceive(g_uart.rx_queue, &ch, 0) == pdTRUE) {
        buffer[i++] = ch;
    }
    buffer[i] = '\0';
    printf("[UART] Receive: %s\n", buffer);
}

void uart_simulate_rx_event(const char *data) {
    // Simulate incoming data (e.g., from hardware/board)
    size_t len = strlen(data);
    for (size_t i = 0; i < len; ++i) {
        size_t next = (g_uart.rx_head + 1) % UART_RX_BUFFER_SIZE;
        if (next == g_uart.rx_tail) {
            printf("[UART] RX buffer full, dropping data.\n");
            break;
        }
        g_uart.rx_buffer[g_uart.rx_head] = data[i];
        g_uart.rx_head = next;
        // Also push to FreeRTOS queue for task notification
        xQueueSend(g_uart.rx_queue, &data[i], 0);
    }
    board_reg_write(BOARD_REG_UART, 2); // Simulate RX ready
    printf("[UART] Simulated RX event: %s\n", data);
}

static void uart_rx_event_cb(void *context) {
    // This would be called by board_simulate_event (ISR context)
    printf("[UART] RX event callback triggered (ISR).\n");
    // In real code, would notify a task or set a flag
}