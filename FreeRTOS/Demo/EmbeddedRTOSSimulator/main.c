// EmbeddedRTOSSimulator - main.c
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "uart.h"
#include "spi.h"
#include "pci.h"
#include "task_scheduler.h"
#include "lru_cache.h"
#include "FreeRTOSConfig.h"
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define REMOTE_STATS_IP   "192.168.1.100" // Set to your visualization server
#define REMOTE_STATS_PORT 5005
#define HEAP_WARN_THRESHOLD 2048
#define STACK_WARN_THRESHOLD 128

// Task prototypes
void vSensorTask(void *pvParameters);
void vProtocolTask(void *pvParameters);
void vLoggerTask(void *pvParameters);
void vPCIeDemoTask(void *pvParameters);

int main(void) {
    printf("EmbeddedRTOSSimulator starting...\n");
    board_init();
    uart_init();
    spi_init(SPI_MODE_MASTER);
    lru_cache_init();
    task_scheduler_init();
    // PCIe Root Complex demo
    xTaskCreate(vPCIeDemoTask, "PCIeRC", 512, (void*)PCI_TYPE_RC, TASK_PRIO_PCIE, NULL);
    // PCIe Endpoint demo
    xTaskCreate(vPCIeDemoTask, "PCIeEP", 512, (void*)PCI_TYPE_EP, TASK_PRIO_PCIE, NULL);
    // Other tasks
    xTaskCreate(vSensorTask, "Sensor", 256, NULL, TASK_PRIO_SENSOR, NULL);
    xTaskCreate(vProtocolTask, "Protocol", 256, NULL, TASK_PRIO_PROTOCOL, NULL);
    xTaskCreate(vLoggerTask, "Logger", 256, NULL, TASK_PRIO_LOGGER, NULL);
    // Start scheduler
    vTaskStartScheduler();
    // Should never reach here
    for(;;);
    return 0;
}

// --- Sensor Task: generates sensor data, uses LRU cache, sends to protocol ---
void vSensorTask(void *pvParameters) {
    int key = 0;
    for(;;) {
        int value = rand() % 1000;
        lru_cache_put(key, value);
        sensor_msg_t msg = { .sensor_value = value, .timestamp = xTaskGetTickCount() };
        send_sensor_data(&msg, portMAX_DELAY);
        printf("[SensorTask] Sent sensor data: key=%d value=%d\n", key, value);
        key = (key + 1) % LRU_CACHE_SIZE;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// --- Protocol Task: receives sensor data, handles UART/SPI/PCIe, logs to logger ---
void vProtocolTask(void *pvParameters) {
    sensor_msg_t msg;
    protocol_log_t log;
    for(;;) {
        if (recv_sensor_data(&msg, portMAX_DELAY) == pdTRUE) {
            snprintf(log.log, sizeof(log.log), "Sensor value: %d at %u", msg.sensor_value, msg.timestamp);
            send_protocol_log(&log, portMAX_DELAY);
            // Simulate UART send/receive
            uart_send(log.log);
            char uart_buf[32];
            uart_receive(uart_buf, sizeof(uart_buf));
            // Simulate SPI transfer
            char spi_rx[32];
            spi_transfer(log.log, spi_rx, strlen(log.log)+1);
            // Simulate PCIe AXI write
            pci_axi_write(0x80000000, msg.sensor_value);
            printf("[ProtocolTask] Processed sensor data, UART/SPI/PCIe actions done.\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// FreeRTOS hook implementations
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    fflush(stdout);
    for(;;); // Halt
}

void vApplicationMallocFailedHook(void) {
    printf("[FATAL] Malloc failed!\n");
    fflush(stdout);
    for(;;); // Halt
}

// --- Logger Task: receives logs, prints, and shows LRU cache state ---
void vLoggerTask(void *pvParameters) {
    protocol_log_t log;
    TickType_t lastStats = xTaskGetTickCount();
    int udp_sock = -1;
    struct sockaddr_in remote_addr;
    // Setup UDP socket for remote export
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(REMOTE_STATS_PORT);
    inet_pton(AF_INET, REMOTE_STATS_IP, &remote_addr.sin_addr);

    for(;;) {
        if (recv_protocol_log(&log, portMAX_DELAY) == pdTRUE) {
            printf("[LoggerTask] Log: %s\n", log.log);
            // Show LRU cache state
            printf("[LoggerTask] LRU cache entries: ");
            for (int i = 0; i < LRU_CACHE_SIZE; ++i) {
                int found;
                int val = lru_cache_get(i, &found);
                if (found) printf("[%d]=%d ", i, val);
            }
            printf("\n");
        }
        // Print and export runtime stats every 10 seconds
        if ((xTaskGetTickCount() - lastStats) > pdMS_TO_TICKS(10000)) {
            char stats[1024];
            vTaskList(stats);
            UBaseType_t freeHeap = xPortGetFreeHeapSize();
            UBaseType_t q1 = uxQueueMessagesWaiting(qSensorToProtocol);
            UBaseType_t q2 = uxQueueMessagesWaiting(qProtocolToLogger);
            UBaseType_t semCount = uxSemaphoreGetCount(semPCIeEvent);
            EventBits_t evBits = xEventGroupGetBits(egSystemEvents);
            TaskStatus_t taskStatus[10];
            UBaseType_t numTasks = uxTaskGetSystemState(taskStatus, 10, NULL);
            FILE *f = fopen("sim_stats.log", "a");
            if (!f) f = stdout;
            fprintf(f, "\n[LoggerTask] FreeRTOS Task Stats:\n%s\n", stats);
            fprintf(f, "[LoggerTask] Free heap: %u bytes\n", (unsigned)freeHeap);
            fprintf(f, "[LoggerTask] qSensorToProtocol: %lu messages waiting\n", (unsigned long)q1);
            fprintf(f, "[LoggerTask] qProtocolToLogger: %lu messages waiting\n", (unsigned long)q2);
            fprintf(f, "[LoggerTask] semPCIeEvent count: %lu\n", (unsigned long)semCount);
            fprintf(f, "[LoggerTask] egSystemEvents bits: 0x%08lx\n", (unsigned long)evBits);
            fprintf(f, "[LoggerTask] Per-task stack high water marks:\n");
            int stack_warn = 0;
            for (UBaseType_t i = 0; i < numTasks; ++i) {
                fprintf(f, "  %s: %lu bytes min free\n", taskStatus[i].pcTaskName, (unsigned long)taskStatus[i].usStackHighWaterMark * sizeof(StackType_t));
                if (taskStatus[i].usStackHighWaterMark * sizeof(StackType_t) < STACK_WARN_THRESHOLD) {
                    stack_warn = 1;
                    printf("[WARN] Stack low for task %s: %lu bytes min free\n", taskStatus[i].pcTaskName, (unsigned long)taskStatus[i].usStackHighWaterMark * sizeof(StackType_t));
                }
            }
            if (freeHeap < HEAP_WARN_THRESHOLD) {
                printf("[WARN] FreeRTOS heap low: %u bytes left!\n", (unsigned)freeHeap);
            }
            if (f != stdout) fclose(f);
            // --- Remote export via UDP ---
            char udp_buf[1024];
            int udp_len = snprintf(udp_buf, sizeof(udp_buf),
                "HEAP:%u Q1:%lu Q2:%lu SEM:%lu EV:0x%08lx",
                (unsigned)freeHeap, (unsigned long)q1, (unsigned long)q2, (unsigned long)semCount, (unsigned long)evBits);
            sendto(udp_sock, udp_buf, udp_len, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
            // --- For visualization tools: consider exporting as CSV or JSON ---
            // Example CSV: fprintf(f, "%%u,%%lu,%%lu,%%lu,0x%%08lx\n", ...);
            // Example JSON: fprintf(f, "{\"heap\":%%u,...}\n", ...);
            lastStats = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// --- PCIe Demo Task: initializes as RC or EP, simulates interrupts ---
void vPCIeDemoTask(void *pvParameters) {
    pci_dev_type_t type = (pci_dev_type_t)pvParameters;
    if (type == PCI_TYPE_RC) {
        printf("\n[PCIe Demo] Initializing as Root Complex (RC)...\n");
        pci_init(PCI_TYPE_RC, PCI_GEN7, PCI_LANES_X16);
    } else {
        printf("\n[PCIe Demo] Initializing as Endpoint (EP)...\n");
        pci_init(PCI_TYPE_EP, PCI_GEN7, PCI_LANES_X8);
    }
    for(;;) {
        // Simulate PCIe events, e.g., MSI/MSIX/INTC
        pci_simulate_event(PCI_INT_MSI, 0);
        signal_pcie_event();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}