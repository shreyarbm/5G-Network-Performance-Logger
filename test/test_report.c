#include <stdio.h>
#include <string.h>
#include "report.h"

int main(void)
{
    KpiMeasurement m = {1, 10.0, 200.0, 0.1, 20.0, 30.0, KPI_SOURCE_SIMULATED};
    KpiThresholds thresholds;
    FILE *stream = tmpfile();
    char output[2048];
    size_t bytes;

    if (stream == NULL) return puts("FAIL: temporary report stream"), 1;
    thresholds_load_defaults(&thresholds);
    if (report_write(stream, &m, 1U, 0U, &thresholds) != APP_STATUS_OK) {
        fclose(stream); return puts("FAIL: report write"), 1;
    }
    rewind(stream);
    bytes = fread(output, 1U, sizeof(output) - 1U, stream);
    output[bytes] = '\0';
    fclose(stream);
    if (strstr(output, "Records analyzed: 1") == NULL ||
        strstr(output, "Latency: avg 10.00 ms") == NULL ||
        strstr(output, "SIMULATED ONLY") == NULL) {
        puts("FAIL: report content"); return 1;
    }
    stream = tmpfile();
    if (stream == NULL) return puts("FAIL: empty report stream"), 1;
    if (report_write(stream, NULL, 0U, 0U, &thresholds) != APP_STATUS_OK) {
        fclose(stream); return puts("FAIL: empty report handling"), 1;
    }
    fclose(stream);
    if (report_generate("build/missing_directory/report.txt", &m, 1U, 0U, &thresholds) == APP_STATUS_OK) {
        puts("FAIL: report path error handling"); return 1;
    }
    puts("PASS: report content, empty data, and output error handling");
    return 0;
}

