#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdio.h>
#include "analysis.h"
#include "thresholds.h"

#define REPORT_PATH_CAPACITY 128U

AppStatus report_write(FILE *output, const KpiMeasurement *records, size_t count,
                       size_t malformed_count, const KpiThresholds *thresholds);
AppStatus report_generate(const char *path, const KpiMeasurement *records,
                          size_t count, size_t malformed_count,
                          const KpiThresholds *thresholds);
AppStatus report_generate_numbered(const KpiMeasurement *records,
                                   size_t count, size_t malformed_count,
                                   const KpiThresholds *thresholds,
                                   char *path_output,
                                   size_t path_output_capacity);

#endif

