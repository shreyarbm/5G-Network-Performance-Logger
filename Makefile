CC ?= cc
CXX ?= g++
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Werror -Iinclude
GTEST_LIBS ?= -lgtest_main -lgtest -pthread
CPPCHECK ?= cppcheck
VALGRIND ?= valgrind

BUILD_DIR := build
GTEST_DIR := $(BUILD_DIR)/gtest
COVERAGE_DIR := $(BUILD_DIR)/coverage

ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
MISRA_ADDON ?= /ucrt64/share/Cppcheck/addons/misra.py
else
EXE_EXT :=
MISRA_ADDON ?= /usr/lib/x86_64-linux-gnu/cppcheck/addons/misra.py
endif

APP_SOURCES := src/main.c src/auth.c src/utils.c src/kpi.c src/logger.c \
               src/analysis.c src/thresholds.c src/alerts.c src/report.c
C_TESTS := test_auth test_utils test_kpi test_logger test_analysis test_alerts test_report
GTEST_SOURCES := $(filter-out src/main.c,$(APP_SOURCES))
GTEST_OBJECTS := $(patsubst src/%.c,$(GTEST_DIR)/%.o,$(GTEST_SOURCES))

.PHONY: all directories run test gtest cppcheck misra valgrind coverage clean help

all: directories $(BUILD_DIR)/logger$(EXE_EXT)

directories:
	mkdir -p $(BUILD_DIR) data reports

$(BUILD_DIR)/logger$(EXE_EXT): $(APP_SOURCES) | directories
	$(CC) $(CFLAGS) $(APP_SOURCES) -o $@

run: all
	./$(BUILD_DIR)/logger$(EXE_EXT)

$(BUILD_DIR)/test_auth$(EXE_EXT): test/test_auth.c src/auth.c src/utils.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_utils$(EXE_EXT): test/test_utils.c src/utils.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_kpi$(EXE_EXT): test/test_kpi.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_logger$(EXE_EXT): test/test_logger.c src/logger.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_analysis$(EXE_EXT): test/test_analysis.c src/analysis.c src/kpi.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_alerts$(EXE_EXT): test/test_alerts.c src/alerts.c src/thresholds.c src/kpi.c src/utils.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/test_report$(EXE_EXT): test/test_report.c src/report.c src/analysis.c src/thresholds.c src/kpi.c src/utils.c | directories
	$(CC) $(CFLAGS) $^ -o $@

$(GTEST_DIR)/%.o: src/%.c | directories
	mkdir -p $(GTEST_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GTEST_DIR)/test_gtest$(EXE_EXT): test/test_gtest.cpp $(GTEST_OBJECTS) | directories
	$(CXX) $(CXXFLAGS) $< $(GTEST_OBJECTS) $(GTEST_LIBS) -o $@

gtest: $(GTEST_DIR)/test_gtest$(EXE_EXT)
	$(GTEST_DIR)/test_gtest$(EXE_EXT)

test: $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXE_EXT),$(C_TESTS))) gtest
	$(BUILD_DIR)/test_auth$(EXE_EXT)
	$(BUILD_DIR)/test_utils$(EXE_EXT)
	$(BUILD_DIR)/test_kpi$(EXE_EXT)
	$(BUILD_DIR)/test_logger$(EXE_EXT)
	$(BUILD_DIR)/test_analysis$(EXE_EXT)
	$(BUILD_DIR)/test_alerts$(EXE_EXT)
	$(BUILD_DIR)/test_report$(EXE_EXT)

cppcheck:
	$(CPPCHECK) --enable=warning,style,performance,portability --std=c11 --error-exitcode=1 -Iinclude src/*.c test/*.c

misra:
	$(CPPCHECK) --addon=$(MISRA_ADDON) --std=c11 --error-exitcode=1 -Iinclude src/*.c test/*.c

valgrind: test
	@for name in $(C_TESTS); do \
		$(VALGRIND) --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $(BUILD_DIR)/$$name$(EXE_EXT) || exit $$?; \
	done
	$(VALGRIND) --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $(GTEST_DIR)/test_gtest$(EXE_EXT)

$(COVERAGE_DIR)/%.o: src/%.c | directories
	mkdir -p $(COVERAGE_DIR)
	$(CC) $(CFLAGS) --coverage -c $< -o $@

$(COVERAGE_DIR)/test_auth$(EXE_EXT): test/test_auth.c $(COVERAGE_DIR)/auth.o $(COVERAGE_DIR)/utils.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_utils$(EXE_EXT): test/test_utils.c $(COVERAGE_DIR)/utils.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_kpi$(EXE_EXT): test/test_kpi.c $(COVERAGE_DIR)/kpi.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_logger$(EXE_EXT): test/test_logger.c $(COVERAGE_DIR)/logger.o $(COVERAGE_DIR)/kpi.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_analysis$(EXE_EXT): test/test_analysis.c $(COVERAGE_DIR)/analysis.o $(COVERAGE_DIR)/kpi.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_alerts$(EXE_EXT): test/test_alerts.c $(COVERAGE_DIR)/alerts.o $(COVERAGE_DIR)/thresholds.o $(COVERAGE_DIR)/kpi.o $(COVERAGE_DIR)/utils.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

$(COVERAGE_DIR)/test_report$(EXE_EXT): test/test_report.c $(COVERAGE_DIR)/report.o $(COVERAGE_DIR)/analysis.o $(COVERAGE_DIR)/thresholds.o $(COVERAGE_DIR)/kpi.o $(COVERAGE_DIR)/utils.o | directories
	$(CC) $(CFLAGS) --coverage $^ -o $@

coverage: $(addprefix $(COVERAGE_DIR)/,$(addsuffix $(EXE_EXT),$(C_TESTS)))
	mkdir -p reports/coverage
	rm -f $(COVERAGE_DIR)/*.gcda reports/coverage/*.gcov
	$(COVERAGE_DIR)/test_auth$(EXE_EXT)
	$(COVERAGE_DIR)/test_utils$(EXE_EXT)
	$(COVERAGE_DIR)/test_kpi$(EXE_EXT)
	$(COVERAGE_DIR)/test_logger$(EXE_EXT)
	$(COVERAGE_DIR)/test_analysis$(EXE_EXT)
	$(COVERAGE_DIR)/test_alerts$(EXE_EXT)
	$(COVERAGE_DIR)/test_report$(EXE_EXT)
	gcov -b -c $(COVERAGE_DIR)/*.gcda > reports/coverage/gcov_summary.txt
	mv -f *.gcov reports/coverage/

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets: all, run, test, gtest, cppcheck, misra, valgrind, coverage, clean, help"
