#include "board.h"
#include <stdio.h>
#include <string.h>

// Global board state
struct board_state g_board;

// Event callback storage (simple fixed array for demo)
#define MAX_EVENT_CBS 8
static board_event_cb_t event_cbs[MAX_EVENT_CBS];
static void *event_ctxs[MAX_EVENT_CBS];
static size_t num_event_cbs = 0;

void board_init(void) {
    memset(&g_board, 0, sizeof(g_board));
    num_event_cbs = 0;
    printf("[Board] ARMv8A virtual board initialized.\n");
}

void board_simulate_event(void) {
    printf("[Board] Simulated hardware event (interrupt).\n");
    // Call all registered event callbacks
    for (size_t i = 0; i < num_event_cbs; ++i) {
        if (event_cbs[i]) {
            event_cbs[i](event_ctxs[i]);
        }
    }
}

uint32_t board_reg_read(uint32_t addr) {
    switch (addr) {
        case BOARD_REG_UART:   return g_board.uart_reg;
        case BOARD_REG_SPI:    return g_board.spi_reg;
        case BOARD_REG_PCI:    return g_board.pci_reg;
        case BOARD_REG_SENSOR: return g_board.sensor_reg;
        default:
            printf("[Board] Invalid reg read: 0x%08x\n", addr);
            return 0;
    }
}

void board_reg_write(uint32_t addr, uint32_t value) {
    switch (addr) {
        case BOARD_REG_UART:   g_board.uart_reg = value; break;
        case BOARD_REG_SPI:    g_board.spi_reg = value; break;
        case BOARD_REG_PCI:    g_board.pci_reg = value; break;
        case BOARD_REG_SENSOR: g_board.sensor_reg = value; break;
        default:
            printf("[Board] Invalid reg write: 0x%08x\n", addr);
            return;
    }
    printf("[Board] Reg write: 0x%08x = 0x%08x\n", addr, value);
}

void board_register_event(board_event_cb_t cb, void *context) {
    if (num_event_cbs < MAX_EVENT_CBS) {
        event_cbs[num_event_cbs] = cb;
        event_ctxs[num_event_cbs] = context;
        ++num_event_cbs;
        printf("[Board] Event callback registered.\n");
    } else {
        printf("[Board] Event callback registration failed (full).\n");
    }
}