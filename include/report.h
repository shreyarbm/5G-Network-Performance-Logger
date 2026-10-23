#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdio.h>
#include "analysis.h"
#include "thresholds.h"

#define REPORT_DEFAULT_PATH "reports/latest_report.txt"

AppStatus report_write(FILE *output, const KpiMeasurement *records, size_t count,
                       size_t malformed_count, const KpiThresholds *thresholds);
AppStatus report_generate(const char *path, const KpiMeasurement *records,
                          size_t count, size_t malformed_count,
                          const KpiThresholds *thresholds);

#endif

