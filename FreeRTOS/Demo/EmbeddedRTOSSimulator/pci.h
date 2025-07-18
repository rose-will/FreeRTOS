#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define PCI_NUM_BARS 6
#define PCI_NUM_ATU_REGIONS 4
#define PCI_NUM_MSI_VECTORS 8
#define PCI_NUM_MSIX_VECTORS 8
#define PCI_NUM_CAPS 4
#define PCI_NUM_INT_TASKS 8

// PCIe device type
typedef enum {
    PCI_TYPE_RC = 0, // Root Complex
    PCI_TYPE_EP = 1  // Endpoint
} pci_dev_type_t;

// PCIe link speed (Gen1-Gen7)
typedef enum {
    PCI_GEN1 = 1,
    PCI_GEN2,
    PCI_GEN3,
    PCI_GEN4,
    PCI_GEN5,
    PCI_GEN6,
    PCI_GEN7
} pci_link_speed_t;

// PCIe lane width
typedef enum {
    PCI_LANES_X1 = 1,
    PCI_LANES_X2 = 2,
    PCI_LANES_X4 = 4,
    PCI_LANES_X8 = 8,
    PCI_LANES_X16 = 16,
    PCI_LANES_X32 = 32
} pci_lane_width_t;

// ATU region type
typedef enum {
    ATU_TYPE_INBOUND = 0,
    ATU_TYPE_OUTBOUND = 1
} atu_type_t;

// MSI/MSIX/INTC types
typedef enum {
    PCI_INT_NONE = 0,
    PCI_INT_LEGACY,
    PCI_INT_MSI,
    PCI_INT_MSIX,
    PCI_INT_INTC
} pci_int_type_t;

// PCIe ATU region structure
struct atu_region {
    atu_type_t type;
    uint32_t base;
    uint32_t limit;
    uint32_t target;
};

// PCIe capability structure
struct pcie_capability {
    uint8_t cap_id;
    uint8_t next_ptr;
    uint8_t data[14];
};

// MSI/MSIX vector table
struct msi_vector {
    uint8_t enabled;
    uint8_t masked;
    TaskHandle_t task;
};

struct msix_vector {
    uint8_t enabled;
    uint8_t masked;
    TaskHandle_t task;
};

// Interrupt registration table
struct pci_int_task_entry {
    pci_int_type_t type;
    int vector;
    TaskHandle_t task;
};

// PCIe state structure
struct pci_state {
    pci_dev_type_t dev_type;
    pci_link_speed_t link_speed;
    pci_lane_width_t lane_width;
    uint32_t config_space[64]; // 256B config space
    uint32_t bar[PCI_NUM_BARS];
    uint32_t bar_mask[PCI_NUM_BARS];
    struct atu_region atu[PCI_NUM_ATU_REGIONS];
    uint32_t link_up;
    uint32_t pll_locked;
    uint32_t perst_deasserted;
    uint32_t fw_loaded;
    uint32_t cr_para_written;
    uint32_t ltssm_state;
    pci_int_type_t int_type;
    struct pcie_capability caps[PCI_NUM_CAPS];
    struct msi_vector msi[PCI_NUM_MSI_VECTORS];
    struct msix_vector msix[PCI_NUM_MSIX_VECTORS];
    struct pci_int_task_entry int_tasks[PCI_NUM_INT_TASKS];
    QueueHandle_t event_queue; // For event notification
};

extern struct pci_state g_pci;

void pci_init(pci_dev_type_t type, pci_link_speed_t speed, pci_lane_width_t width);
void pci_clock_pll_init(void);
void pci_perst_deassert(void);
void pci_firmware_load(void);
void pci_cr_para_axi_write(void);
void pci_header_init(void);
void pci_set_link_speed_and_width(pci_link_speed_t speed, pci_lane_width_t width);
void pci_link_training(void);
void pci_linkup(void);
void pci_reset_bars(void);
void pci_map_bar(int bar, uint32_t addr, uint32_t mask);
void pci_config_write(int offset, uint32_t value);
uint32_t pci_config_read(int offset);
void pci_capability_add(uint8_t cap_id, const uint8_t *data, size_t len);
void pci_atu_configure(int region, atu_type_t type, uint32_t base, uint32_t limit, uint32_t target);
void pci_axi_write(uint32_t addr, uint32_t value);
uint32_t pci_axi_read(uint32_t addr);
void pci_generate_interrupt(pci_int_type_t type, int vector);
void pci_interrupt_register(pci_int_type_t type, int vector, TaskHandle_t task);
void pci_msi_configure(int vector, TaskHandle_t task);
void pci_msix_configure(int vector, TaskHandle_t task);
void pci_send(const char *data);
void pci_receive(char *buffer, int maxlen);
void pci_simulate_event(pci_int_type_t type, int vector);

#endif // PCI_H