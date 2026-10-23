#ifndef ALERTS_H
#define ALERTS_H

#include <stdio.h>
#include "thresholds.h"


AppStatus alerts_generate(FILE *output, const KpiMeasurement *measurement,
                          const KpiThresholdResult *results, size_t result_count);

#endif

