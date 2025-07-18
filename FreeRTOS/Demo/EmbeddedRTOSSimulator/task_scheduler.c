#include "task_scheduler.h"
#include <stdio.h>

// Inter-task communication handles
typedef struct {
    int sensor_value;
    uint32_t timestamp;
} sensor_msg_t;

typedef struct {
    char log[64];
} protocol_log_t;

QueueHandle_t qSensorToProtocol = NULL;
QueueHandle_t qProtocolToLogger = NULL;
SemaphoreHandle_t semPCIeEvent = NULL;
EventGroupHandle_t egSystemEvents = NULL;

void task_scheduler_init(void) {
    qSensorToProtocol = xQueueCreate(8, sizeof(sensor_msg_t));
    qProtocolToLogger = xQueueCreate(8, sizeof(protocol_log_t));
    semPCIeEvent = xSemaphoreCreateBinary();
    egSystemEvents = xEventGroupCreate();
    printf("[TaskScheduler] Queues, semaphore, and event group initialized.\n");
}

BaseType_t send_sensor_data(const void *data, TickType_t timeout) {
    return xQueueSend(qSensorToProtocol, data, timeout);
}

BaseType_t recv_sensor_data(void *data, TickType_t timeout) {
    return xQueueReceive(qSensorToProtocol, data, timeout);
}

BaseType_t send_protocol_log(const void *data, TickType_t timeout) {
    return xQueueSend(qProtocolToLogger, data, timeout);
}

BaseType_t recv_protocol_log(void *data, TickType_t timeout) {
    return xQueueReceive(qProtocolToLogger, data, timeout);
}

void signal_pcie_event(void) {
    xSemaphoreGive(semPCIeEvent);
    xEventGroupSetBits(egSystemEvents, EV_SYSTEM_PCIE_INT);
    printf("[TaskScheduler] PCIe event signaled.\n");
}

void wait_for_pcie_event(void) {
    xSemaphoreTake(semPCIeEvent, portMAX_DELAY);
    printf("[TaskScheduler] PCIe event received.\n");
}