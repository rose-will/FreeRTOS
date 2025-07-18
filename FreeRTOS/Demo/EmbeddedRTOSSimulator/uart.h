#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "queue.h"

#define UART_TX_BUFFER_SIZE 128
#define UART_RX_BUFFER_SIZE 128

// UART state structure
struct uart_state {
    char tx_buffer[UART_TX_BUFFER_SIZE];
    size_t tx_head, tx_tail;
    char rx_buffer[UART_RX_BUFFER_SIZE];
    size_t rx_head, rx_tail;
    QueueHandle_t rx_queue; // For task notification
};

extern struct uart_state g_uart;

void uart_init(void);
void uart_send(const char *data);
void uart_receive(char *buffer, int maxlen);

// Simulate UART RX interrupt/event
void uart_simulate_rx_event(const char *data);

#endif // UART_H