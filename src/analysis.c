#include <stddef.h>

#include "analysis.h"

AppStatus analysis_calculate(const KpiMeasurement *records, size_t count,
                             KpiStatistics *statistics,
                             size_t *invalid_count)
{
    size_t index;
    size_t valid_count;
    double latency_sum;
    double throughput_sum;
    double packet_loss_sum;
    double cpu_sum;
    double memory_sum;

    if (statistics == NULL || invalid_count == NULL ||
        (records == NULL && count > 0U)) {
        return APP_STATUS_INVALID_INPUT;
    }

    *invalid_count = 0U;
    *statistics = (KpiStatistics){0};
    if (count == 0U) {
        return APP_STATUS_OK;
    }

    valid_count = 0U;
    latency_sum = 0.0;
    throughput_sum = 0.0;
    packet_loss_sum = 0.0;
    cpu_sum = 0.0;
    memory_sum = 0.0;

    for (index = 0U; index < count; ++index) {
        const KpiMeasurement *record = &records[index];

        if (kpi_validate(record) != APP_STATUS_OK) {
            (*invalid_count)++;
            continue;
        }

        if (valid_count == 0U) {
            statistics->minimum_latency_ms = record->latency_ms;
            statistics->maximum_latency_ms = record->latency_ms;
            statistics->minimum_throughput_mbps = record->throughput_mbps;
            statistics->maximum_throughput_mbps = record->throughput_mbps;
            statistics->minimum_packet_loss_percent = record->packet_loss_percent;
            statistics->maximum_packet_loss_percent = record->packet_loss_percent;
            statistics->minimum_cpu_utilization_percent = record->cpu_utilization_percent;
            statistics->maximum_cpu_utilization_percent = record->cpu_utilization_percent;
            statistics->minimum_memory_utilization_percent = record->memory_utilization_percent;
            statistics->maximum_memory_utilization_percent = record->memory_utilization_percent;
        } else {
            if (record->latency_ms < statistics->minimum_latency_ms) statistics->minimum_latency_ms = record->latency_ms;
            if (record->latency_ms > statistics->maximum_latency_ms) statistics->maximum_latency_ms = record->latency_ms;
            if (record->throughput_mbps < statistics->minimum_throughput_mbps) statistics->minimum_throughput_mbps = record->throughput_mbps;
            if (record->throughput_mbps > statistics->maximum_throughput_mbps) statistics->maximum_throughput_mbps = record->throughput_mbps;
            if (record->packet_loss_percent < statistics->minimum_packet_loss_percent) statistics->minimum_packet_loss_percent = record->packet_loss_percent;
            if (record->packet_loss_percent > statistics->maximum_packet_loss_percent) statistics->maximum_packet_loss_percent = record->packet_loss_percent;
            if (record->cpu_utilization_percent < statistics->minimum_cpu_utilization_percent) statistics->minimum_cpu_utilization_percent = record->cpu_utilization_percent;
            if (record->cpu_utilization_percent > statistics->maximum_cpu_utilization_percent) statistics->maximum_cpu_utilization_percent = record->cpu_utilization_percent;
            if (record->memory_utilization_percent < statistics->minimum_memory_utilization_percent) statistics->minimum_memory_utilization_percent = record->memory_utilization_percent;
            if (record->memory_utilization_percent > statistics->maximum_memory_utilization_percent) statistics->maximum_memory_utilization_percent = record->memory_utilization_percent;
        }

        latency_sum += record->latency_ms;
        throughput_sum += record->throughput_mbps;
        packet_loss_sum += record->packet_loss_percent;
        cpu_sum += record->cpu_utilization_percent;
        memory_sum += record->memory_utilization_percent;
        valid_count++;
    }

    statistics->record_count = valid_count;
    if (valid_count == 0U) {
        return APP_STATUS_OK;
    }

    statistics->average_latency_ms = latency_sum / (double)valid_count;
    statistics->average_throughput_mbps = throughput_sum / (double)valid_count;
    statistics->average_packet_loss_percent = packet_loss_sum / (double)valid_count;
    statistics->average_cpu_utilization_percent = cpu_sum / (double)valid_count;
    statistics->average_memory_utilization_percent = memory_sum / (double)valid_count;
    return APP_STATUS_OK;
}



