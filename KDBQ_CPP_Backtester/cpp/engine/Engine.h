#pragma once
#include <string>
#include <vector>
#include <optional>

struct BookTick {
    std::string ts;   // ISO string for demo
    double bid{0.0};
    double ask{0.0};
    int bsz{0};
    int asz{0};
};

struct Order {
    std::string ts;   // submission timestamp (string idx for demo)
    std::string sym;
    std::string side; // "buy" or "sell"
    std::string type; // "market" or "limit"
    double px{0.0};   // limit price or reference (ignored for market)
    int qty{0};
    std::string tif;  // "IOC" or "GFD"
    int id{0};
};

struct Fill {
    int order_id{0};
    std::string ts; // fill timestamp
    double px{0.0};
    int qty{0};
    std::string side; // copy from order
    std::string liq;  // "taker" or "maker"
};

struct EngineParams {
    int latency_ticks{2};
    double slip_bps{0.5}; // applied to taker fills
};
