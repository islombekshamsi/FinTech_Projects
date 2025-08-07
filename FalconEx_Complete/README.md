
# FalconEx 🚀
**High-Performance Limit Order Book Engine with Replay and Strategy Simulation**

FalconEx is a C++-based simulation engine for high-frequency trading environments. It supports:
- ⚙️ Real-time multithreaded order matching (BUY/SELL, LIMIT/MARKET)
- 📉 Market data replay for backtesting strategies
- 🧠 Plug-in strategy modules (e.g., momentum-based algo)
- 📊 Built-in benchmarking (latency, throughput)

## 🔧 Features
- Thread-safe matching engine
- Real-time simulation and latency stress tests
- Order book snapshot visualization
- Historical market replay with text input
- Momentum-based sample trading strategy

## 📁 File Structure
```
FalconEx/
├── src/              # C++ source code
│   └── falconex.cpp
├── data/             # Sample replay files
│   └── sample_replay.txt
├── docs/             # Architecture diagrams, strategy notes
├── README.md         # This file
└── LICENSE           # MIT License (recommended)
```

## 🚀 How to Build & Run
1. Compile:
```bash
g++ -std=c++17 -O2 src/falconex.cpp -o falconex -pthread
```
2. Run:
```bash
./falconex
```

Use commands like `buy`, `sell`, `sim`, `replay`, `strat`, and `show`.

## 📈 Example Replay Input (data/sample_replay.txt)
```
buy 102.45 100
sell 103.20 50
buy 101.75 75
sell 104.00 200
```

## 🧠 Future Additions
- Real-time market data via API (e.g., Polygon.io, Twelve Data)
- REST API wrapper (Flask or FastAPI)
- Strategy analytics dashboard (Streamlit + Pandas)
- Support for multiple instruments

## 👨‍💻 Author
Islombek Shamsiev — [GitHub](https://github.com/islombekshamsi) | [LinkedIn](https://www.linkedin.com/in/islom-shamsiev/)

## 📄 License
MIT License
