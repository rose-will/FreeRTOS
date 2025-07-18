#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

// Task priorities (ARMv8A-style)
#define TASK_PRIO_SENSOR   4
#define TASK_PRIO_PROTOCOL 3
#define TASK_PRIO_LOGGER   2
#define TASK_PRIO_PCIE     5

// Inter-task communication handles
extern QueueHandle_t qSensorToProtocol;
extern QueueHandle_t qProtocolToLogger;
extern SemaphoreHandle_t semPCIeEvent;
extern EventGroupHandle_t egSystemEvents;

// Event group bits
#define EV_SYSTEM_UART_RX   (1 << 0)
#define EV_SYSTEM_SPI_RX    (1 << 1)
#define EV_SYSTEM_PCIE_INT  (1 << 2)

void task_scheduler_init(void);

// API for sending/receiving messages between tasks
BaseType_t send_sensor_data(const void *data, TickType_t timeout);
BaseType_t recv_sensor_data(void *data, TickType_t timeout);
BaseType_t send_protocol_log(const void *data, TickType_t timeout);
BaseType_t recv_protocol_log(void *data, TickType_t timeout);

// API for signaling PCIe events
void signal_pcie_event(void);
void wait_for_pcie_event(void);

#endif // TASK_SCHEDULER_H