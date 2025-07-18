#include "pci.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

struct pci_state g_pci;

// --- PCIe Initialization Steps ---

void pci_init(pci_dev_type_t type, pci_link_speed_t speed, pci_lane_width_t width) {
    memset(&g_pci, 0, sizeof(g_pci));
    g_pci.event_queue = xQueueCreate(8, sizeof(pci_int_type_t));
    g_pci.dev_type = type;
    g_pci.link_speed = speed;
    g_pci.lane_width = width;
    printf("[PCIe] Init: type=%s, speed=Gen%d, lanes=x%d\n", type == PCI_TYPE_RC ? "RC" : "EP", speed, width);
    pci_clock_pll_init();
    pci_perst_deassert();
    pci_firmware_load();
    pci_cr_para_axi_write();
    pci_header_init();
    pci_set_link_speed_and_width(speed, width);
    pci_link_training();
    pci_reset_bars();
    for (int i = 0; i < PCI_NUM_ATU_REGIONS; ++i) {
        pci_atu_configure(i, ATU_TYPE_OUTBOUND, 0x80000000 + i*0x100000, 0x800FFFFF + i*0x100000, 0x00000000 + i*0x100000);
    }
    pci_linkup();
}

void pci_clock_pll_init(void) {
    g_pci.pll_locked = 1;
    printf("[PCIe] Clock/PLL initialized and locked.\n");
    board_reg_write(BOARD_REG_PCI, 0x10);
}

void pci_perst_deassert(void) {
    g_pci.perst_deasserted = 1;
    printf("[PCIe] PERST# deasserted.\n");
    board_reg_write(BOARD_REG_PCI, 0x11);
}

void pci_firmware_load(void) {
    g_pci.fw_loaded = 1;
    printf("[PCIe] Firmware loaded (if soft IP/FPGA).\n");
    board_reg_write(BOARD_REG_PCI, 0x12);
}

void pci_cr_para_axi_write(void) {
    g_pci.cr_para_written = 1;
    printf("[PCIe] CR_PARA AXI config written.\n");
    board_reg_write(BOARD_REG_PCI, 0x13);
}

void pci_header_init(void) {
    // Vendor ID, Device ID, Class Code, etc.
    g_pci.config_space[0x00/4] = 0x12348086; // Device/Vendor
    g_pci.config_space[0x08/4] = 0x06040000; // Class code (Bridge), revision
    g_pci.config_space[0x0C/4] = 0x00100000; // Header type, BIST, etc.
    g_pci.config_space[0x2C/4] = 0xABCD5678; // Subsystem Vendor/ID
    g_pci.config_space[0x34/4] = 0x40;      // Capabilities pointer
    g_pci.config_space[0x10/4] = 0x00000000; // BAR0
    g_pci.config_space[0x14/4] = 0x00000000; // BAR1
    printf("[PCIe] Header/config space initialized.\n");
    // Add PCIe and MSI capabilities
    uint8_t pcie_cap[14] = {0};
    pcie_cap[0] = 0x10; // PCIe Cap ID
    pci_capability_add(0x10, pcie_cap, sizeof(pcie_cap));
    uint8_t msi_cap[14] = {0};
    msi_cap[0] = 0x05; // MSI Cap ID
    pci_capability_add(0x05, msi_cap, sizeof(msi_cap));
}

void pci_set_link_speed_and_width(pci_link_speed_t speed, pci_lane_width_t width) {
    g_pci.link_speed = speed;
    g_pci.lane_width = width;
    printf("[PCIe] Link speed set: Gen%d, Lane width: x%d\n", speed, width);
    board_reg_write(BOARD_REG_PCI, 0x14);
}

void pci_link_training(void) {
    g_pci.ltssm_state = 1; // Simulate LTSSM in training
    printf("[PCIe] Link training (LTSSM)...\n");
    g_pci.ltssm_state = 2; // LTSSM in L0 (link up)
    printf("[PCIe] LTSSM state: L0 (link up)\n");
    board_reg_write(BOARD_REG_PCI, 0x15);
}

void pci_linkup(void) {
    g_pci.link_up = 1;
    printf("[PCIe] Link up!\n");
    board_reg_write(BOARD_REG_PCI, 1);
}

void pci_reset_bars(void) {
    for (int i = 0; i < PCI_NUM_BARS; ++i) {
        g_pci.bar[i] = 0;
        g_pci.bar_mask[i] = 0;
    }
    printf("[PCIe] BARs reset.\n");
}

void pci_map_bar(int bar, uint32_t addr, uint32_t mask) {
    if (bar < 0 || bar >= PCI_NUM_BARS) return;
    g_pci.bar[bar] = addr;
    g_pci.bar_mask[bar] = mask;
    printf("[PCIe] BAR%d mapped: addr=0x%08x mask=0x%08x\n", bar, addr, mask);
}

void pci_config_write(int offset, uint32_t value) {
    if (offset < 0 || offset >= 64) return;
    g_pci.config_space[offset] = value;
    printf("[PCIe] Config write: offset=%d value=0x%08x\n", offset, value);
}

uint32_t pci_config_read(int offset) {
    if (offset < 0 || offset >= 64) return 0;
    printf("[PCIe] Config read: offset=%d\n", offset);
    return g_pci.config_space[offset];
}

void pci_capability_add(uint8_t cap_id, const uint8_t *data, size_t len) {
    for (int i = 0; i < PCI_NUM_CAPS; ++i) {
        if (g_pci.caps[i].cap_id == 0) {
            g_pci.caps[i].cap_id = cap_id;
            g_pci.caps[i].next_ptr = 0; // For demo
            memcpy(g_pci.caps[i].data, data, len > 14 ? 14 : len);
            printf("[PCIe] Capability added: cap_id=0x%02x\n", cap_id);
            return;
        }
    }
    printf("[PCIe] Capability table full!\n");
}

void pci_atu_configure(int region, atu_type_t type, uint32_t base, uint32_t limit, uint32_t target) {
    if (region < 0 || region >= PCI_NUM_ATU_REGIONS) return;
    g_pci.atu[region].type = type;
    g_pci.atu[region].base = base;
    g_pci.atu[region].limit = limit;
    g_pci.atu[region].target = target;
    printf("[PCIe] ATU region %d configured: %s base=0x%08x limit=0x%08x target=0x%08x\n", region, type == ATU_TYPE_INBOUND ? "INBOUND" : "OUTBOUND", base, limit, target);
}

void pci_axi_write(uint32_t addr, uint32_t value) {
    // Simulate ATU translation
    for (int i = 0; i < PCI_NUM_ATU_REGIONS; ++i) {
        if (g_pci.atu[i].type == ATU_TYPE_OUTBOUND && addr >= g_pci.atu[i].base && addr <= g_pci.atu[i].limit) {
            uint32_t translated = g_pci.atu[i].target + (addr - g_pci.atu[i].base);
            printf("[PCIe] AXI write: addr=0x%08x (translated=0x%08x) value=0x%08x\n", addr, translated, value);
            return;
        }
    }
    printf("[PCIe] AXI write: addr=0x%08x (no ATU match) value=0x%08x\n", addr, value);
}

uint32_t pci_axi_read(uint32_t addr) {
    for (int i = 0; i < PCI_NUM_ATU_REGIONS; ++i) {
        if (g_pci.atu[i].type == ATU_TYPE_OUTBOUND && addr >= g_pci.atu[i].base && addr <= g_pci.atu[i].limit) {
            uint32_t translated = g_pci.atu[i].target + (addr - g_pci.atu[i].base);
            printf("[PCIe] AXI read: addr=0x%08x (translated=0x%08x)\n", addr, translated);
            return 0xDEADBEEF;
        }
    }
    printf("[PCIe] AXI read: addr=0x%08x (no ATU match)\n", addr);
    return 0xDEADBEEF;
}

void pci_generate_interrupt(pci_int_type_t type, int vector) {
    printf("[PCIe] Interrupt generated: type=%d vector=%d\n", type, vector);
    // Notify registered task(s)
    for (int i = 0; i < PCI_NUM_INT_TASKS; ++i) {
        if (g_pci.int_tasks[i].type == type && g_pci.int_tasks[i].vector == vector && g_pci.int_tasks[i].task) {
            xTaskNotifyGive(g_pci.int_tasks[i].task);
            printf("[PCIe] Notified task for interrupt type=%d vector=%d\n", type, vector);
        }
    }
    // For MSI/MSIX, also check vector tables
    if (type == PCI_INT_MSI && vector < PCI_NUM_MSI_VECTORS && g_pci.msi[vector].enabled && !g_pci.msi[vector].masked && g_pci.msi[vector].task) {
        xTaskNotifyGive(g_pci.msi[vector].task);
        printf("[PCIe] MSI vector %d delivered to task\n", vector);
    }
    if (type == PCI_INT_MSIX && vector < PCI_NUM_MSIX_VECTORS && g_pci.msix[vector].enabled && !g_pci.msix[vector].masked && g_pci.msix[vector].task) {
        xTaskNotifyGive(g_pci.msix[vector].task);
        printf("[PCIe] MSIX vector %d delivered to task\n", vector);
    }
    board_reg_write(BOARD_REG_PCI, 2); // Simulate interrupt
}

void pci_interrupt_register(pci_int_type_t type, int vector, TaskHandle_t task) {
    for (int i = 0; i < PCI_NUM_INT_TASKS; ++i) {
        if (g_pci.int_tasks[i].task == NULL) {
            g_pci.int_tasks[i].type = type;
            g_pci.int_tasks[i].vector = vector;
            g_pci.int_tasks[i].task = task;
            printf("[PCIe] Task registered for interrupt type=%d vector=%d\n", type, vector);
            return;
        }
    }
    printf("[PCIe] Interrupt registration table full!\n");
}

void pci_msi_configure(int vector, TaskHandle_t task) {
    if (vector < 0 || vector >= PCI_NUM_MSI_VECTORS) return;
    g_pci.msi[vector].enabled = 1;
    g_pci.msi[vector].masked = 0;
    g_pci.msi[vector].task = task;
    printf("[PCIe] MSI vector %d configured for task\n", vector);
}

void pci_msix_configure(int vector, TaskHandle_t task) {
    if (vector < 0 || vector >= PCI_NUM_MSIX_VECTORS) return;
    g_pci.msix[vector].enabled = 1;
    g_pci.msix[vector].masked = 0;
    g_pci.msix[vector].task = task;
    printf("[PCIe] MSIX vector %d configured for task\n", vector);
}

void pci_send(const char *data) {
    printf("[PCIe] Send: %s\n", data);
}

void pci_receive(char *buffer, int maxlen) {
    snprintf(buffer, maxlen, "PCI_DATA");
    printf("[PCIe] Receive: %s\n", buffer);
}

void pci_simulate_event(pci_int_type_t type, int vector) {
    printf("[PCIe] Simulated event: type=%d vector=%d\n", type, vector);
    pci_generate_interrupt(type, vector);
}