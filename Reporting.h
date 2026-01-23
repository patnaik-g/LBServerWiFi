#ifndef REPORTING_H
#define REPORTING_H

#include <Arduino.h>

#define REPORT_BUFFER_SIZE 64

struct ReportMsg {
    char text[REPORT_BUFFER_SIZE];
};

// Global handle so all tasks can find the reporter
extern QueueHandle_t reportQueue;

void reportingTask(void *pvParameters);

#endif