#include <cmath>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>

#include <gtest/gtest.h>

extern "C" {
#include "alerts.h"
#include "analysis.h"
#include "auth.h"
#include "kpi.h"
#include "logger.h"
#include "report.h"
#include "thresholds.h"
#include "utils.h"
}

namespace {

KpiMeasurement Sample(time_t timestamp = (time_t)1234)
{
    return KpiMeasurement{timestamp, 20.0, 200.0, 0.2, 30.0, 40.0,
                          KPI_SOURCE_SIMULATED};
}

FILE *Input(const char *text)
{
    FILE *stream = std::tmpfile();
    if (stream == nullptr) return nullptr;
    if (std::fputs(text, stream) < 0 || std::fflush(stream) != 0 ||
        std::fseek(stream, 0L, SEEK_SET) != 0) {
        (void)std::fclose(stream);
        return nullptr;
    }
    return stream;
}

std::string Contents(FILE *stream)
{
    long length;
    std::string result;

    if (std::fseek(stream, 0L, SEEK_END) != 0) return {};
    length = std::ftell(stream);
    if (length < 0L || std::fseek(stream, 0L, SEEK_SET) != 0) return {};
    result.resize(static_cast<std::size_t>(length));
    if (!result.empty() && std::fread(&result[0], 1U, result.size(), stream) != result.size()) {
        return {};
    }
    return result;
}

void Close(FILE *stream)
{
    if (stream != nullptr) (void)std::fclose(stream);
}

TEST(Auth, AcceptsDemoCredentials)
{
    EXPECT_EQ(auth_check_credentials("demo", "5g-demo"), 1);
}

TEST(Auth, RejectsWrongCredentials)
{
    EXPECT_EQ(auth_check_credentials("other", "5g-demo"), 0);
    EXPECT_EQ(auth_check_credentials("demo", "wrong"), 0);
}

TEST(Auth, RejectsNullAndEmptyCredentials)
{
    EXPECT_EQ(auth_check_credentials(nullptr, "x"), 0);
    EXPECT_EQ(auth_check_credentials("demo", nullptr), 0);
    EXPECT_EQ(auth_check_credentials("", "x"), 0);
    EXPECT_EQ(auth_check_credentials("demo", ""), 0);
}

TEST(AuthLogin, AcceptsValidInput)
{
    FILE *input = Input("demo\n5g-demo\n");
    FILE *output = std::tmpfile();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(auth_login_from(input, output), APP_STATUS_OK);
    EXPECT_NE(Contents(output).find("Demo login successful"), std::string::npos);
    Close(input);
    Close(output);
}

TEST(AuthLogin, DeniesAfterThreeFailures)
{
    FILE *input = Input("no\nx\nno\nx\nno\nx\n");
    FILE *output = std::tmpfile();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(auth_login_from(input, output), APP_STATUS_ERROR);
    EXPECT_NE(Contents(output).find("Maximum login attempts reached"), std::string::npos);
    Close(input);
    Close(output);
}

TEST(AuthLogin, HandlesEmptyInputAndInvalidStreams)
{
    FILE *input = Input("\ndemo\n5g-demo\n");
    FILE *output = std::tmpfile();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(auth_login_from(input, output), APP_STATUS_OK);
    EXPECT_EQ(auth_login_from(nullptr, output), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(auth_login_from(input, nullptr), APP_STATUS_INVALID_INPUT);
    Close(input);
    Close(output);
}

TEST(KpiValidation, AcceptsBoundaries)
{
    KpiMeasurement measurement = Sample();
    measurement.latency_ms = KPI_LATENCY_MIN_MS;
    measurement.throughput_mbps = KPI_THROUGHPUT_MAX_MBPS;
    measurement.packet_loss_percent = KPI_PERCENT_MIN;
    measurement.cpu_utilization_percent = KPI_PERCENT_MAX;
    measurement.memory_utilization_percent = KPI_PERCENT_MIN;
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_OK);
}

TEST(KpiValidation, RejectsNullTimestampAndNonFiniteValues)
{
    KpiMeasurement measurement = Sample();
    EXPECT_EQ(kpi_validate(nullptr), APP_STATUS_INVALID_INPUT);
    measurement.timestamp = (time_t)-1;
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_INVALID_INPUT);
    measurement = Sample();
    measurement.latency_ms = NAN;
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_INVALID_INPUT);
}

TEST(KpiValidation, RejectsOutOfRangeAndWrongSource)
{
    KpiMeasurement measurement = Sample();
    measurement.latency_ms = -0.01;
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_INVALID_INPUT);
    measurement = Sample();
    measurement.packet_loss_percent = 100.01;
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_INVALID_INPUT);
    measurement = Sample();
    measurement.source = static_cast<KpiSource>(1);
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_INVALID_INPUT);
}

TEST(KpiCollection, ProducesValidTimestampedSample)
{
    KpiMeasurement measurement{};
    const time_t before = std::time(nullptr);
    ASSERT_EQ(kpi_collect_simulated(&measurement), APP_STATUS_OK);
    const time_t after = std::time(nullptr);
    EXPECT_GE(measurement.timestamp, before);
    EXPECT_LE(measurement.timestamp, after);
    EXPECT_EQ(measurement.source, KPI_SOURCE_SIMULATED);
    EXPECT_EQ(kpi_validate(&measurement), APP_STATUS_OK);
}

TEST(KpiCollection, RejectsNullOutput)
{
    EXPECT_EQ(kpi_collect_simulated(nullptr), APP_STATUS_INVALID_INPUT);
}

TEST(KpiCollection, RepeatedSamplesRemainInRange)
{
    for (int index = 0; index < 100; ++index) {
        KpiMeasurement measurement{};
        ASSERT_EQ(kpi_collect_simulated(&measurement), APP_STATUS_OK);
        EXPECT_GE(measurement.latency_ms, 5.0);
        EXPECT_LE(measurement.latency_ms, 200.0);
        EXPECT_GE(measurement.throughput_mbps, 50.0);
        EXPECT_LE(measurement.throughput_mbps, 1000.0);
    }
}

TEST(UtilsInput, ReadsLineAndBlankLine)
{
    char buffer[16];
    FILE *input = Input("abc\n\n");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(utils_read_line_from(input, nullptr, nullptr, buffer, sizeof(buffer)), APP_STATUS_OK);
    EXPECT_STREQ(buffer, "abc");
    EXPECT_EQ(utils_read_line_from(input, nullptr, nullptr, buffer, sizeof(buffer)), APP_STATUS_OK);
    EXPECT_STREQ(buffer, "");
    Close(input);
}

TEST(UtilsInput, RejectsOverlongLineAndBadArguments)
{
    char buffer[4];
    FILE *input = Input("abcdef\n");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(utils_read_line_from(input, nullptr, nullptr, buffer, sizeof(buffer)), APP_STATUS_INVALID_INPUT);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(utils_read_line_from(nullptr, nullptr, nullptr, buffer, sizeof(buffer)), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(utils_read_line_from(input, nullptr, "prompt", buffer, sizeof(buffer)), APP_STATUS_INVALID_INPUT);
    Close(input);
}

TEST(UtilsInput, ReportsEndOfFile)
{
    char buffer[16];
    FILE *input = Input("");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(utils_read_line_from(input, nullptr, nullptr, buffer, sizeof(buffer)), APP_STATUS_ERROR);
    EXPECT_STREQ(buffer, "");
    Close(input);
}

TEST(Timestamp, FormatsFixedTimeConsistently)
{
    char actual[32];
    char expected[32];
    time_t fixed_time = (time_t)1234;
    struct tm *time_info = std::localtime(&fixed_time);
    ASSERT_NE(time_info, nullptr);
    ASSERT_NE(std::strftime(expected, sizeof(expected), "%Y-%m-%d %H:%M:%S", time_info), 0U);
    EXPECT_EQ(utils_format_timestamp(fixed_time, actual, sizeof(actual)), APP_STATUS_OK);
    EXPECT_STREQ(actual, expected);
}

TEST(Timestamp, RejectsInvalidTimeAndBuffer)
{
    char buffer[32];
    EXPECT_EQ(utils_format_timestamp((time_t)-1, buffer, sizeof(buffer)), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(utils_format_timestamp((time_t)1234, nullptr, sizeof(buffer)), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(utils_format_timestamp((time_t)1234, buffer, 1U), APP_STATUS_ERROR);
}

TEST(Timestamp, HandlesRepresentableTimeValues)
{
    char buffer[32];
    EXPECT_EQ(utils_format_timestamp((time_t)0, buffer, sizeof(buffer)), APP_STATUS_OK);
    EXPECT_EQ(std::strlen(buffer), 19U);
}

TEST(Analysis, HandlesEmptyAndNullArguments)
{
    KpiStatistics statistics{};
    std::size_t invalid_count = 99U;
    EXPECT_EQ(analysis_calculate(nullptr, 0U, &statistics, &invalid_count), APP_STATUS_OK);
    EXPECT_EQ(statistics.record_count, 0U);
    EXPECT_EQ(invalid_count, 0U);
    EXPECT_EQ(analysis_calculate(nullptr, 1U, &statistics, &invalid_count), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(analysis_calculate(nullptr, 0U, nullptr, &invalid_count), APP_STATUS_INVALID_INPUT);
}

TEST(Analysis, CalculatesOneRecord)
{
    const KpiMeasurement measurement = Sample();
    KpiStatistics statistics{};
    std::size_t invalid_count = 0U;
    ASSERT_EQ(analysis_calculate(&measurement, 1U, &statistics, &invalid_count), APP_STATUS_OK);
    EXPECT_EQ(statistics.record_count, 1U);
    EXPECT_DOUBLE_EQ(statistics.average_latency_ms, measurement.latency_ms);
    EXPECT_DOUBLE_EQ(statistics.minimum_latency_ms, measurement.latency_ms);
    EXPECT_DOUBLE_EQ(statistics.maximum_latency_ms, measurement.latency_ms);
}

TEST(Analysis, CalculatesMultipleRecordsAndSkipsInvalid)
{
    KpiMeasurement records[3] = {Sample(), Sample((time_t)1235), Sample((time_t)1236)};
    KpiStatistics statistics{};
    std::size_t invalid_count = 0U;
    records[1].latency_ms = 40.0;
    records[2].latency_ms = -1.0;
    ASSERT_EQ(analysis_calculate(records, 3U, &statistics, &invalid_count), APP_STATUS_OK);
    EXPECT_EQ(statistics.record_count, 2U);
    EXPECT_EQ(invalid_count, 1U);
    EXPECT_DOUBLE_EQ(statistics.average_latency_ms, 30.0);
    EXPECT_DOUBLE_EQ(statistics.minimum_latency_ms, 20.0);
    EXPECT_DOUBLE_EQ(statistics.maximum_latency_ms, 40.0);
}

TEST(Logger, AppendsAndPreservesTimestamp)
{
    const char *path = "build/gtest/kpi_log.csv";
    const KpiMeasurement measurement = Sample((time_t)1700000000);
    KpiMeasurement loaded[2]{};
    std::size_t count = 0U;
    std::size_t malformed = 0U;
    (void)std::remove(path);
    ASSERT_EQ(logger_append(path, &measurement), APP_STATUS_OK);
    ASSERT_EQ(logger_load(path, loaded, 2U, &count, &malformed), APP_STATUS_OK);
    EXPECT_EQ(count, 1U);
    EXPECT_EQ(malformed, 0U);
    EXPECT_EQ(loaded[0].timestamp, measurement.timestamp);
    EXPECT_DOUBLE_EQ(loaded[0].latency_ms, measurement.latency_ms);
    (void)std::remove(path);
}

TEST(Logger, AppendsMultipleRowsWithoutDuplicateHeaders)
{
    const char *path = "build/gtest/kpi_log.csv";
    const KpiMeasurement measurement = Sample();
    KpiMeasurement loaded[2]{};
    std::size_t count = 0U;
    std::size_t malformed = 0U;
    (void)std::remove(path);
    ASSERT_EQ(logger_append(path, &measurement), APP_STATUS_OK);
    ASSERT_EQ(logger_append(path, &measurement), APP_STATUS_OK);
    ASSERT_EQ(logger_load(path, loaded, 2U, &count, &malformed), APP_STATUS_OK);
    EXPECT_EQ(count, 2U);
    EXPECT_EQ(malformed, 0U);
    (void)std::remove(path);
}

TEST(Logger, HandlesMalformedMissingAndInvalidInputs)
{
    const char *path = "build/gtest/kpi_log.csv";
    const KpiMeasurement measurement = Sample();
    KpiMeasurement loaded[2]{};
    std::size_t count = 0U;
    std::size_t malformed = 0U;
    FILE *file;
    (void)std::remove(path);
    ASSERT_EQ(logger_append(path, &measurement), APP_STATUS_OK);
    file = std::fopen(path, "a");
    ASSERT_NE(file, nullptr);
    ASSERT_NE(std::fputs("not,a,valid,row\n", file), EOF);
    ASSERT_EQ(std::fclose(file), 0);
    ASSERT_EQ(logger_load(path, loaded, 2U, &count, &malformed), APP_STATUS_OK);
    EXPECT_EQ(count, 1U);
    EXPECT_EQ(malformed, 1U);
    EXPECT_EQ(logger_load("build/gtest/missing.csv", loaded, 2U, &count, &malformed), APP_STATUS_ERROR);
    EXPECT_EQ(logger_append(path, nullptr), APP_STATUS_INVALID_INPUT);
    (void)std::remove(path);
}

TEST(ThresholdDefaults, LoadsDocumentedDefaults)
{
    KpiThresholds thresholds{};
    thresholds_load_defaults(&thresholds);
    EXPECT_DOUBLE_EQ(thresholds.latency_warning_ms, 100.0);
    EXPECT_DOUBLE_EQ(thresholds.latency_critical_ms, 200.0);
    EXPECT_DOUBLE_EQ(thresholds.throughput_warning_mbps, 100.0);
    EXPECT_DOUBLE_EQ(thresholds.throughput_critical_mbps, 50.0);
}

TEST(ThresholdDefaults, AllowsNullAndIsRepeatable)
{
    KpiThresholds first{};
    KpiThresholds second{};
    thresholds_load_defaults(nullptr);
    thresholds_load_defaults(&first);
    thresholds_load_defaults(&second);
    EXPECT_EQ(std::memcmp(&first, &second, sizeof(first)), 0);
}

TEST(ThresholdDefaults, InitializesEveryKpi)
{
    KpiThresholds thresholds{};
    thresholds_load_defaults(&thresholds);
    EXPECT_DOUBLE_EQ(thresholds.packet_loss_warning_percent, 1.0);
    EXPECT_DOUBLE_EQ(thresholds.packet_loss_critical_percent, 3.0);
    EXPECT_DOUBLE_EQ(thresholds.cpu_warning_percent, 80.0);
    EXPECT_DOUBLE_EQ(thresholds.cpu_critical_percent, 95.0);
    EXPECT_DOUBLE_EQ(thresholds.memory_warning_percent, 80.0);
    EXPECT_DOUBLE_EQ(thresholds.memory_critical_percent, 95.0);
}

TEST(ThresholdCheck, ReportsNormalWarningAndCriticalBoundaries)
{
    KpiThresholds thresholds{};
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    std::size_t result_count = 0U;
    KpiMeasurement measurement = Sample();
    thresholds_load_defaults(&thresholds);
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &result_count), APP_STATUS_OK);
    EXPECT_EQ(result_count, KPI_THRESHOLD_RESULT_CAPACITY);
    EXPECT_EQ(results[KPI_METRIC_LATENCY].severity, KPI_SEVERITY_NORMAL);
    measurement.latency_ms = thresholds.latency_warning_ms;
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &result_count), APP_STATUS_OK);
    EXPECT_EQ(results[KPI_METRIC_LATENCY].severity, KPI_SEVERITY_WARNING);
    measurement.latency_ms = thresholds.latency_critical_ms;
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &result_count), APP_STATUS_OK);
    EXPECT_EQ(results[KPI_METRIC_LATENCY].severity, KPI_SEVERITY_CRITICAL);
}

TEST(ThresholdCheck, DetectsLowThroughputAndMultipleViolations)
{
    KpiThresholds thresholds{};
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    std::size_t result_count = 0U;
    KpiMeasurement measurement = Sample();
    thresholds_load_defaults(&thresholds);
    measurement.throughput_mbps = 49.0;
    measurement.packet_loss_percent = 4.0;
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &result_count), APP_STATUS_OK);
    EXPECT_EQ(results[KPI_METRIC_THROUGHPUT].severity, KPI_SEVERITY_CRITICAL);
    EXPECT_EQ(results[KPI_METRIC_PACKET_LOSS].severity, KPI_SEVERITY_CRITICAL);
}

TEST(ThresholdCheck, RejectsInvalidThresholdAndCapacity)
{
    KpiThresholds thresholds{};
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    std::size_t result_count = 0U;
    const KpiMeasurement measurement = Sample();
    thresholds_load_defaults(&thresholds);
    thresholds.latency_warning_ms = 250.0;
    EXPECT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &result_count), APP_STATUS_INVALID_INPUT);
    thresholds_load_defaults(&thresholds);
    EXPECT_EQ(thresholds_check(&measurement, &thresholds, results, 4U,
                               &result_count), APP_STATUS_INVALID_INPUT);
}

TEST(ThresholdNames, MapsMetrics)
{
    EXPECT_STREQ(thresholds_metric_name(KPI_METRIC_LATENCY), "Latency");
    EXPECT_STREQ(thresholds_metric_name(KPI_METRIC_THROUGHPUT), "Throughput");
    EXPECT_STREQ(thresholds_metric_name(KPI_METRIC_MEMORY), "Memory utilization");
}

TEST(ThresholdNames, HandlesInvalidMetric)
{
    EXPECT_STREQ(thresholds_metric_name(static_cast<KpiMetric>(7)), "Unknown KPI");
}

TEST(ThresholdNames, MapsAllMetricNames)
{
    EXPECT_STREQ(thresholds_metric_name(KPI_METRIC_PACKET_LOSS), "Packet loss");
    EXPECT_STREQ(thresholds_metric_name(KPI_METRIC_CPU), "CPU utilization");
}

TEST(ThresholdNames, MapsSeveritiesAndUnknown)
{
    EXPECT_STREQ(thresholds_severity_name(KPI_SEVERITY_NORMAL), "NORMAL");
    EXPECT_STREQ(thresholds_severity_name(KPI_SEVERITY_WARNING), "WARNING");
    EXPECT_STREQ(thresholds_severity_name(KPI_SEVERITY_CRITICAL), "CRITICAL");
    EXPECT_STREQ(thresholds_severity_name(static_cast<KpiSeverity>(3)), "UNKNOWN");
}

TEST(ThresholdNames, HandlesInvalidSeverityValues)
{
    EXPECT_STREQ(thresholds_severity_name(static_cast<KpiSeverity>(3)), "UNKNOWN");
}

TEST(ThresholdNames, SeverityLabelsAreStable)
{
    EXPECT_STREQ(thresholds_severity_name(KPI_SEVERITY_WARNING), "WARNING");
    EXPECT_STREQ(thresholds_severity_name(KPI_SEVERITY_WARNING), "WARNING");
}

TEST(Alerts, PrintsNormalStatusWithTimestamp)
{
    const KpiMeasurement measurement = Sample();
    KpiThresholds thresholds{};
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    std::size_t count = 0U;
    char formatted[32];
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &count), APP_STATUS_OK);
    ASSERT_EQ(utils_format_timestamp(measurement.timestamp, formatted, sizeof(formatted)), APP_STATUS_OK);
    ASSERT_EQ(alerts_generate(output, &measurement, results, count), APP_STATUS_OK);
    const std::string text = Contents(output);
    EXPECT_NE(text.find(formatted), std::string::npos);
    EXPECT_NE(text.find("NORMAL"), std::string::npos);
    Close(output);
}

TEST(Alerts, IncludesViolationNameValueAndLimit)
{
    KpiMeasurement measurement = Sample();
    KpiThresholds thresholds{};
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    std::size_t count = 0U;
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    measurement.latency_ms = 201.0;
    ASSERT_EQ(thresholds_check(&measurement, &thresholds, results,
                               KPI_THRESHOLD_RESULT_CAPACITY, &count), APP_STATUS_OK);
    ASSERT_EQ(alerts_generate(output, &measurement, results, count), APP_STATUS_OK);
    const std::string text = Contents(output);
    EXPECT_NE(text.find("CRITICAL"), std::string::npos);
    EXPECT_NE(text.find("Latency"), std::string::npos);
    EXPECT_NE(text.find("201.00 ms"), std::string::npos);
    EXPECT_NE(text.find("200.00 ms"), std::string::npos);
    Close(output);
}

TEST(Alerts, RejectsInvalidArguments)
{
    const KpiMeasurement measurement = Sample();
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY]{};
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(alerts_generate(nullptr, &measurement, results,
                              KPI_THRESHOLD_RESULT_CAPACITY), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(alerts_generate(output, &measurement, results, 1U), APP_STATUS_INVALID_INPUT);
    Close(output);
}

TEST(Report, WritesGeneratedTimestampAndStatistics)
{
    const KpiMeasurement measurement = Sample();
    KpiThresholds thresholds{};
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    ASSERT_EQ(report_write(output, &measurement, 1U, 0U, &thresholds), APP_STATUS_OK);
    const std::string text = Contents(output);
    EXPECT_NE(text.find("Report generated: "), std::string::npos);
    EXPECT_NE(text.find("Records analyzed: 1"), std::string::npos);
    EXPECT_NE(text.find("SIMULATED ONLY"), std::string::npos);
    Close(output);
}

TEST(Report, HandlesEmptyInput)
{
    KpiThresholds thresholds{};
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    EXPECT_EQ(report_write(output, nullptr, 0U, 0U, &thresholds), APP_STATUS_OK);
    EXPECT_NE(Contents(output).find("No valid measurements"), std::string::npos);
    Close(output);
}

TEST(Report, RejectsInvalidArguments)
{
    KpiThresholds thresholds{};
    const KpiMeasurement measurement = Sample();
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    EXPECT_EQ(report_write(nullptr, &measurement, 1U, 0U, &thresholds), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(report_write(output, nullptr, 1U, 0U, &thresholds), APP_STATUS_INVALID_INPUT);
    EXPECT_EQ(report_write(output, &measurement, 1U, 0U, nullptr), APP_STATUS_INVALID_INPUT);
    Close(output);
}

TEST(Report, GeneratesFileAndRejectsBadPath)
{
    const char *path = "build/gtest/report.txt";
    const KpiMeasurement measurement = Sample();
    KpiThresholds thresholds{};
    thresholds_load_defaults(&thresholds);
    (void)std::remove(path);
    EXPECT_EQ(report_generate(path, &measurement, 1U, 0U, &thresholds), APP_STATUS_OK);
    EXPECT_EQ(report_generate("build/gtest/missing/report.txt", &measurement, 1U,
                              0U, &thresholds), APP_STATUS_ERROR);
    FILE *file = std::fopen(path, "r");
    char line[256];
    std::string report_text;
    ASSERT_NE(file, nullptr);
    while (std::fgets(line, static_cast<int>(sizeof(line)), file) != nullptr) {
        report_text += line;
    }
    EXPECT_NE(report_text.find("Report generated: "), std::string::npos);
    Close(file);
    (void)std::remove(path);
}

TEST(Report, ReportsMalformedAndInvalidRows)
{
    KpiMeasurement records[2] = {Sample(), Sample((time_t)1235)};
    KpiThresholds thresholds{};
    FILE *output = std::tmpfile();
    ASSERT_NE(output, nullptr);
    thresholds_load_defaults(&thresholds);
    records[1].latency_ms = -1.0;
    ASSERT_EQ(report_write(output, records, 2U, 2U, &thresholds), APP_STATUS_OK);
    const std::string text = Contents(output);
    EXPECT_NE(text.find("Malformed CSV rows skipped: 2"), std::string::npos);
    EXPECT_NE(text.find("Invalid measurements skipped: 1"), std::string::npos);
    EXPECT_NE(text.find("Data quality: INCOMPLETE"), std::string::npos);
    Close(output);
}

}
