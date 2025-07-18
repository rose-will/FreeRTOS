#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>

// Simulated ARMv8A memory-mapped registers
#define BOARD_REG_BASE 0x40000000
#define BOARD_REG_UART 0x40001000
#define BOARD_REG_SPI  0x40002000
#define BOARD_REG_PCI  0x40003000
#define BOARD_REG_SENSOR 0x40004000

// Board state structure
struct board_state {
    uint32_t uart_reg;
    uint32_t spi_reg;
    uint32_t pci_reg;
    uint32_t sensor_reg;
};

extern struct board_state g_board;

// Event/interrupt callback type
typedef void (*board_event_cb_t)(void *context);

void board_init(void);
void board_simulate_event(void);

// Register read/write API
uint32_t board_reg_read(uint32_t addr);
void board_reg_write(uint32_t addr, uint32_t value);

// Register event/interrupt callback
void board_register_event(board_event_cb_t cb, void *context);

#endif // BOARD_H