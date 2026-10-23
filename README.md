# 5G Telecom Network Performance Logger (POC)

A beginner-friendly, portable C11 proof of concept for collecting and reviewing **simulated** network KPI samples. The program does not connect to a 5G network, and its values do not describe actual network performance.

## Features

- Demo-only login with three attempts. Credentials are `demo` / `5g-demo`; they are hardcoded solely for this demonstration and are not secure authentication.
- Simulated KPI collection with timestamp and source label.
- CSV append and historical retrieval, including malformed-row skipping and reporting.
- Average, minimum, and maximum analysis.
- Warning and critical threshold checks plus readable alerts.
- Terminal report and optional text export to `reports/latest_report.txt`.
- Module tests with deterministic input data.

## Project layout

```text
include/   Public declarations and shared structures
src/       Application and module implementations
test/      Independent module tests
data/      Historical CSV data
reports/   Generated text reports
build/     Compiled application and test binaries
Makefile   Build, test, clean, and help targets
```

The project modules are `auth`, `utils`, `kpi`, `logger`, `analysis`, `thresholds`, `alerts`, and `report`. `main.c` handles authentication and the menu workflow.

## KPIs and simulated ranges

| KPI | Unit | Simulated range | Validation range |
|---|---:|---:|---:|
| Latency | ms | 5 to 200 | 0 to 1000 |
| Throughput | Mbps | 50 to 1000 | 0 to 10000 |
| Packet loss | percent | 0 to 5 | 0 to 100 |
| CPU utilization | percent | 0 to 100 | 0 to 100 |
| Memory utilization | percent | 0 to 100 | 0 to 100 |

The generated ranges are illustrative only. Threshold defaults are also demonstrations, not universal 5G standards: latency warning/critical at 100/200 ms; throughput warning/critical at 100/50 Mbps (low values are concerning); packet loss at 1/3%; CPU and memory at 80/95%.

## CSV format

`data/kpi_log.csv` contains a header followed by records:

```csv
timestamp,latency_ms,throughput_mbps,packet_loss_percent,cpu_utilization_percent,memory_utilization_percent,source
```

Timestamp is Unix time in seconds. The source field is `SIMULATED`. The logger appends rows and creates a header when the target file is empty. Keep the working directory at the project root so the relative `data/` and `reports/` paths resolve. OnlineGDB may not retain files after a session ends; download your files if you need to keep them.

## Build and run on Linux

Install a C11 compiler and GNU Make (for example, GCC and Make), then from the project root run:

```sh
make help
make
./build/logger
```

Log in with the demonstration credentials. Menu options collect a sample, view the latest sample or history, analyze data, check thresholds, generate alerts, write a report, and exit.

Build uses `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror`. `make clean` removes generated binaries. It does not remove CSV data or reports.

## Tests

Run all module tests from the project root:

```sh
make test
```

The tests use deterministic records and print `PASS` or `FAIL`. Coverage includes login credential checks, KPI bounds, CSV append/read/malformed/missing-file handling, analysis calculations and empty data, critical threshold/alert output, and report content/empty data. Some OS-level file failures (such as a full disk) are difficult to reproduce reliably and are handled through checked return values instead.

## OnlineGDB

OnlineGDB is useful for trying small C programs, but its editor may not support this full directory layout, multiple compilation units, Make, or persistent files in the way a local project does. If its interface accepts multiple source files, include every `.c` file in `src/` and add `-Iinclude` to compiler options. Use C11 and the warning flags above. The `main.c` application links all source modules.

If only one source file can be compiled at once, test modules by copying the required module `.c` files and their dependencies into one temporary OnlineGDB program. Keep the original VS Code project files unchanged. Use separate sessions/programs for tests, since each test source has its own `main()`.

## Limitations and future real data

This POC has no telecom equipment integration, network API, subscriber data, concurrency, or production authentication. Random simulation is for exercising program structure only. The CSV file is local and has no locking or multi-process coordination.

To add a real source later, implement a provider that fills the existing `KpiMeasurement` structure, validates its fields, and records the source and timestamp. Keep provider-specific APIs in their own module; do not describe simulated values as measured network performance. Transfer the source tree to a Linux server only when access is available, then build and run it from the project root with the commands above.

## Safety-conscious C notes

The code uses bounded buffers, validates numeric KPI ranges and non-finite values, checks file operations, and avoids dynamic allocation. A future MISRA C review may require further changes to library usage, numeric conversions, error handling, and project-specific coding rules. This project has **not** been formally reviewed for or certified as MISRA C compliant. Static analysis can be run where available, for example `cppcheck --enable=warning,style,performance,portability --std=c11 include src test`.


