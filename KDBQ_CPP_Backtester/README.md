# kdb+/q + C++ Backtesting Engine (Hedge-Fund-Style)
Hello I have created **ready-to-run project scaffold** that combines **kdb+/q** (data store, signals, orchestration) with **C++** (execution simulation with latency, slippage, TIF).
It includes:
- q scripts to ingest/generate market data, produce orders, orchestrate runs, and report PnL.
- A C++ backtesting engine with a simple but realistic fill model.
- Sample data to run a demo immediately (no external downloads required).
- CMake build for CLion or VS Code; works on macOS.

> ⚠️ You must install kdb+/q locally to run the q pieces. For C++ only, you can still compile and run the engine against the provided CSVs.

## Quickstart (Demo)
```bash
# 0) Build C++
cd cpp && cmake -S . -B build && cmake --build build --config Release
./build/engine/backtester --quotes ../data/sample/quotes.csv --orders ../data/sample/orders.csv --out ../artifact --latency_ticks 2 --slip_bps 0.5

# 1) (Optional) Generate orders with q from quotes
# In another terminal (q not required for basic demo since sample orders are provided)
cd ../q
q run.q sym:`DEMO date:2025.09.03 quotes:`../data/sample/quotes.csv outdir:`../artifact

# 2) Report with q
q report.q fills:`../artifact/fills.csv quotes:`../data/sample/quotes.csv out:`../artifact/report.csv
```

## Structure
```
/q
  ingest.q    # Ingest CSV or synth-generate ticks into kdb+
  run.q       # Generate signals -> orders.csv (market/limit) + orchestrate engine
  report.q    # Load fills + quotes, compute PnL & stats
  server.q    # (Optional) IPC server stubs for q<->C++

/cpp
  /engine
    Backtester.cpp  # Main C++ engine + fill model
    Engine.h        # Engine types
    Order.h
    BookView.h
  /bridge
    Client.cpp      # (Optional) IPC client stub using files; replace with k.h for real IPC
  CMakeLists.txt

/config
  config.yaml       # Strategy and engine parameters

/data/sample
  quotes.csv        # Sample L1 quotes (bid/ask) for one symbol
  orders.csv        # Sample orders for quick demo

/scripts
  make_demo.sh      # One-command demo runner
```

## macOS Setup
- Install **kdb+/q** (free personal edition) from kx.com and set `QHOME`.
- Install **CMake** and a C++ compiler (Apple Clang is fine).
- (Optional) Install **qStudio** to browse tables; use **CLion** or **VS Code** for C++.

## Notes
- The provided engine uses a **tick-based latency** model (`--latency_ticks`) for simplicity. You can switch to timestamp-based later.
- Orders support `type=market|limit` and `tif=IOC|GFD`.
- Slippage (`--slip_bps`) is applied on taker fills.
- Extend the fill model to include partial fills, queue approximations, and maker/taker fees.

Feel free to connect or reach out!

Happy Coding!
