CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror -Iinclude
BUILD_DIR := build
APP_SOURCES := src/main.c src/auth.c src/utils.c src/kpi.c src/logger.c src/analysis.c src/thresholds.c src/alerts.c src/report.c

.PHONY: all test clean help directories

all: directories $(BUILD_DIR)/logger

directories:
	mkdir -p $(BUILD_DIR) data reports

$(BUILD_DIR)/logger: $(APP_SOURCES) | directories
	$(CC) $(CFLAGS) $(APP_SOURCES) -o $@

$(BUILD_DIR)/test_auth: test/test_auth.c src/auth.c src/utils.c | directories
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD_DIR)/test_kpi: test/test_kpi.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD_DIR)/test_logger: test/test_logger.c src/logger.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD_DIR)/test_analysis: test/test_analysis.c src/analysis.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD_DIR)/test_alerts: test/test_alerts.c src/alerts.c src/thresholds.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@
$(BUILD_DIR)/test_report: test/test_report.c src/report.c src/analysis.c src/thresholds.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@

test: $(BUILD_DIR)/test_auth $(BUILD_DIR)/test_kpi $(BUILD_DIR)/test_logger $(BUILD_DIR)/test_analysis $(BUILD_DIR)/test_alerts $(BUILD_DIR)/test_report
	$(BUILD_DIR)/test_auth
	$(BUILD_DIR)/test_kpi
	$(BUILD_DIR)/test_logger
	$(BUILD_DIR)/test_analysis
	$(BUILD_DIR)/test_alerts
	$(BUILD_DIR)/test_report

clean:
	rm -f $(BUILD_DIR)/logger $(BUILD_DIR)/test_auth $(BUILD_DIR)/test_kpi $(BUILD_DIR)/test_logger $(BUILD_DIR)/test_analysis $(BUILD_DIR)/test_alerts $(BUILD_DIR)/test_report

help:
	@echo "Targets: all (build app), test (run module tests), clean (remove binaries), help"
