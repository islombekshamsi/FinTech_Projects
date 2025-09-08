#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
echo "[1/3] Building C++ engine..."
cmake -S cpp -B cpp/build >/dev/null
cmake --build cpp/build --config Release >/dev/null
echo "[2/3] Running backtester on sample data..."
mkdir -p artifact
./cpp/build/backtester --quotes data/sample/quotes.csv --orders data/sample/orders.csv --out artifact --latency_ticks 2 --slip_bps 0.5
echo "[3/3] (Optional) Reporting with q (if installed)..."
if command -v q >/dev/null 2>&1; then
  q q/report.q fills:`artifact/fills.csv quotes:`data/sample/quotes.csv out:`artifact/report.csv
  echo "Report written to artifact/report.csv"
else
  echo "q not found; skipping report step."
fi
echo "Done. See 'artifact/' for outputs."
