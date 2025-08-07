#include <iostream>
#include <map>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

using namespace std;
using namespace std::chrono;

enum class OrderType { LIMIT, MARKET };
enum class Side { BUY, SELL };

struct Order {
    int id;
    string symbol;
    Side side;
    OrderType type;
    int quantity;
    double price;
    long timestamp;
};

class OrderBook {
private:
    map<double, queue<Order>, greater<double>> buyOrders;
    map<double, queue<Order>> sellOrders;
    mutex bookMutex;
    vector<string> tradeLog;

public:
    void addOrder(const Order& order) {
        lock_guard<mutex> lock(bookMutex);

        if (order.side == Side::BUY) {
            buyOrders[order.price].push(order);
        } else {
            sellOrders[order.price].push(order);
        }
    }

    void matchOrders() {
        lock_guard<mutex> lock(bookMutex);

        while (!buyOrders.empty() && !sellOrders.empty()) {
            auto highestBuy = buyOrders.begin();
            auto lowestSell = sellOrders.begin();

            if (highestBuy->first >= lowestSell->first) {
                Order buyOrder = highestBuy->second.front();
                Order sellOrder = lowestSell->second.front();

                int tradedQty = min(buyOrder.quantity, sellOrder.quantity);

                stringstream log;
                log << "TRADE: " << tradedQty << " shares of " << buyOrder.symbol
                    << " at $" << fixed << setprecision(2) << lowestSell->first;
                tradeLog.push_back(log.str());
                cout << log.str() << endl;

                buyOrder.quantity -= tradedQty;
                sellOrder.quantity -= tradedQty;

                if (buyOrder.quantity == 0) {
                    highestBuy->second.pop();
                    if (highestBuy->second.empty()) buyOrders.erase(highestBuy);
                } else {
                    highestBuy->second.front().quantity = buyOrder.quantity;
                }

                if (sellOrder.quantity == 0) {
                    lowestSell->second.pop();
                    if (lowestSell->second.empty()) sellOrders.erase(lowestSell);
                } else {
                    lowestSell->second.front().quantity = sellOrder.quantity;
                }

            } else {
                break;
            }
        }
    }

    void printBook() {
        lock_guard<mutex> lock(bookMutex);
        cout << "\nOrder Book Snapshot:" << endl;
        cout << "BUY SIDE:" << endl;
        for (auto& [price, q] : buyOrders) {
            cout << "Price: $" << price << " Qty: " << q.front().quantity << endl;
        }
        cout << "SELL SIDE:" << endl;
        for (auto& [price, q] : sellOrders) {
            cout << "Price: $" << price << " Qty: " << q.front().quantity << endl;
        }
    }

    void exportLog(const string& filename) {
        ofstream file(filename);
        for (const auto& entry : tradeLog) {
            file << entry << endl;
        }
        file.close();
    }
};

class MatchingEngine {
private:
    OrderBook book;
    atomic<int> orderIdCounter{1};

public:
    void placeOrder(Side side, OrderType type, double price, int quantity, const string& symbol = "AAPL") {
        Order o = {
            .id = orderIdCounter++,
            .symbol = symbol,
            .side = side,
            .type = type,
            .quantity = quantity,
            .price = price,
            .timestamp = chrono::system_clock::now().time_since_epoch().count()
        };
        book.addOrder(o);
        book.matchOrders();
    }

    void simulateClients(int numThreads, int numOrdersPerThread) {
        vector<thread> threads;
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> priceDist(100.0, 110.0);
        uniform_int_distribution<> qtyDist(1, 100);
        uniform_int_distribution<> sideDist(0, 1);

        auto start = high_resolution_clock::now();

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([=]() mutable {
                for (int j = 0; j < numOrdersPerThread; ++j) {
                    Side side = sideDist(gen) == 0 ? Side::BUY : Side::SELL;
                    double price = priceDist(gen);
                    int qty = qtyDist(gen);
                    placeOrder(side, OrderType::LIMIT, price, qty);
                    this_thread::sleep_for(chrono::milliseconds(1));
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        int totalOrders = numThreads * numOrdersPerThread;

        cout << "\n===== BENCHMARK RESULTS =====" << endl;
        cout << "Total Orders: " << totalOrders << endl;
        cout << "Total Time: " << duration << " ms" << endl;
        cout << "Throughput: " << (totalOrders * 1000.0 / duration) << " orders/sec" << endl;
        cout << "=============================" << endl;
    }

    void replayMarketData(const string& filename) {
        ifstream file(filename);
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string cmd;
            double price;
            int qty;
            ss >> cmd >> price >> qty;
            if (cmd == "buy" || cmd == "sell") {
                placeOrder(cmd == "buy" ? Side::BUY : Side::SELL, OrderType::LIMIT, price, qty);
            }
        }
    }

    void runMomentumStrategy(int steps = 100) {
        default_random_engine gen(random_device{}());
        uniform_real_distribution<> priceDist(100.0, 110.0);

        for (int i = 0; i < steps; ++i) {
            double price = priceDist(gen);
            Side side = price > 105.0 ? Side::SELL : Side::BUY;
            placeOrder(side, OrderType::LIMIT, price, 10);
            this_thread::sleep_for(milliseconds(5));
        }
    }

    void run() {
        string cmd;
        while (true) {
            cout << "\nEnter Command (buy/sell/show/sim/replay/strat/exit): ";
            cin >> cmd;
            if (cmd == "buy" || cmd == "sell") {
                double price;
                int qty;
                cout << "Price: "; cin >> price;
                cout << "Qty: "; cin >> qty;
                placeOrder(cmd == "buy" ? Side::BUY : Side::SELL, OrderType::LIMIT, price, qty);
            } else if (cmd == "show") {
                book.printBook();
            } else if (cmd == "sim") {
                int threads, orders;
                cout << "# Threads: "; cin >> threads;
                cout << "Orders per thread: "; cin >> orders;
                simulateClients(threads, orders);
            } else if (cmd == "replay") {
                string file;
                cout << "Enter file path: "; cin >> file;
                replayMarketData(file);
            } else if (cmd == "strat") {
                runMomentumStrategy();
            } else if (cmd == "exit") {
                book.exportLog("trades.txt");
                break;
            }
        }
    }
};

int main() {
    MatchingEngine engine;
    engine.run();
    return 0;
}

/*
Sample Output: 
/Users/islomshamsiev/CLionProjects/FalconEx/cmake-build-debug/FalconEx

Enter Command (buy/sell/show/sim/replay/strat/exit): buy
Price: 102.5
Qty: 50

Enter Command (buy/sell/show/sim/replay/strat/exit): sell
Price: 102.5
Qty: 50
TRADE: 50 shares of AAPL at $102.50

Enter Command (buy/sell/show/sim/replay/strat/exit): show

Order Book Snapshot:
BUY SIDE:
SELL SIDE:

Enter Command (buy/sell/show/sim/replay/strat/exit): sim
# Threads: 4
Orders per thread: 250
TRADE: 32 shares of AAPL at $106.61
TRADE: 32 shares of AAPL at $106.61
TRADE: 9 shares of AAPL at $106.61
TRADE: 23 shares of AAPL at $106.61
TRADE: 32 shares of AAPL at $106.61
TRADE: 18 shares of AAPL at $100.92
TRADE: 71 shares of AAPL at $100.92
TRADE: 2 shares of AAPL at $100.92
TRADE: 73 shares of AAPL at $100.92
TRADE: 14 shares of AAPL at $100.92
TRADE: 13 shares of AAPL at $100.92
TRADE: 27 shares of AAPL at $100.92
TRADE: 27 shares of AAPL at $100.92
TRADE: 22 shares of AAPL at $100.92
TRADE: 5 shares of AAPL at $100.92
TRADE: 40 shares of AAPL at $100.92
TRADE: 40 shares of AAPL at $100.92
TRADE: 4 shares of AAPL at $100.92
TRADE: 68 shares of AAPL at $103.61
TRADE: 17 shares of AAPL at $103.61
TRADE: 51 shares of AAPL at $103.61
TRADE: 34 shares of AAPL at $103.61
TRADE: 34 shares of AAPL at $103.61
TRADE: 51 shares of AAPL at $103.61
TRADE: 17 shares of AAPL at $103.61
TRADE: 50 shares of AAPL at $103.61
TRADE: 18 shares of AAPL at $103.61
TRADE: 45 shares of AAPL at $101.63
TRADE: 34 shares of AAPL at $101.63
TRADE: 11 shares of AAPL at $101.63
TRADE: 45 shares of AAPL at $101.63
TRADE: 23 shares of AAPL at $101.63
TRADE: 22 shares of AAPL at $101.63
TRADE: 35 shares of AAPL at $108.75
TRADE: 22 shares of AAPL at $108.75
TRADE: 13 shares of AAPL at $108.75
TRADE: 35 shares of AAPL at $108.75
TRADE: 31 shares of AAPL at $108.75
TRADE: 4 shares of AAPL at $108.75
TRADE: 8 shares of AAPL at $107.55
TRADE: 8 shares of AAPL at $107.55
TRADE: 8 shares of AAPL at $107.55
TRADE: 5 shares of AAPL at $107.55
TRADE: 3 shares of AAPL at $107.55
TRADE: 20 shares of AAPL at $104.49
TRADE: 10 shares of AAPL at $104.49
TRADE: 10 shares of AAPL at $104.49
TRADE: 20 shares of AAPL at $104.49
TRADE: 3 shares of AAPL at $104.49
TRADE: 17 shares of AAPL at $104.49
TRADE: 8 shares of AAPL at $108.45
TRADE: 8 shares of AAPL at $108.45
TRADE: 35 shares of AAPL at $101.40
TRADE: 31 shares of AAPL at $101.40
TRADE: 4 shares of AAPL at $101.40
TRADE: 35 shares of AAPL at $101.40
TRADE: 27 shares of AAPL at $101.40
TRADE: 8 shares of AAPL at $101.40
TRADE: 58 shares of AAPL at $103.28
TRADE: 20 shares of AAPL at $103.28
TRADE: 46 shares of AAPL at $103.28
TRADE: 32 shares of AAPL at $103.28
TRADE: 50 shares of AAPL at $103.28
TRADE: 28 shares of AAPL at $103.28
TRADE: 22 shares of AAPL at $103.28
TRADE: 56 shares of AAPL at $103.28
TRADE: 31 shares of AAPL at $106.19
TRADE: 31 shares of AAPL at $106.19
TRADE: 31 shares of AAPL at $106.19
TRADE: 25 shares of AAPL at $106.19
TRADE: 6 shares of AAPL at $106.19
TRADE: 80 shares of AAPL at $103.61
TRADE: 1 shares of AAPL at $103.61
TRADE: 79 shares of AAPL at $103.61
TRADE: 8 shares of AAPL at $103.61
TRADE: 72 shares of AAPL at $103.61
TRADE: 9 shares of AAPL at $103.61
TRADE: 71 shares of AAPL at $103.61
TRADE: 10 shares of AAPL at $103.55
TRADE: 10 shares of AAPL at $103.55
TRADE: 20 shares of AAPL at $103.55
TRADE: 20 shares of AAPL at $103.55
TRADE: 20 shares of AAPL at $103.55
TRADE: 8 shares of AAPL at $108.45
TRADE: 8 shares of AAPL at $108.45
TRADE: 8 shares of AAPL at $106.64
TRADE: 8 shares of AAPL at $106.64
TRADE: 8 shares of AAPL at $106.64
TRADE: 8 shares of AAPL at $106.64
TRADE: 9 shares of AAPL at $105.17
TRADE: 50 shares of AAPL at $105.17
TRADE: 7 shares of AAPL at $105.17
TRADE: 52 shares of AAPL at $105.17
TRADE: 5 shares of AAPL at $105.17
TRADE: 54 shares of AAPL at $105.17
TRADE: 3 shares of AAPL at $105.17
TRADE: 11 shares of AAPL at $105.17
TRADE: 45 shares of AAPL at $105.17
TRADE: 7 shares of AAPL at $106.10
TRADE: 7 shares of AAPL at $106.10
TRADE: 7 shares of AAPL at $106.10
TRADE: 7 shares of AAPL at $106.10
TRADE: 8 shares of AAPL at $104.62
TRADE: 44 shares of AAPL at $104.62
TRADE: 15 shares of AAPL at $104.62
TRADE: 29 shares of AAPL at $104.62
TRADE: 38 shares of AAPL at $104.62
TRADE: 6 shares of AAPL at $104.62
TRADE: 44 shares of AAPL at $104.62
TRADE: 40 shares of AAPL at $102.06
TRADE: 8 shares of AAPL at $102.06
TRADE: 32 shares of AAPL at $102.06
TRADE: 16 shares of AAPL at $102.06
TRADE: 24 shares of AAPL at $102.06
TRADE: 24 shares of AAPL at $102.06
TRADE: 16 shares of AAPL at $102.06
TRADE: 32 shares of AAPL at $102.06
TRADE: 17 shares of AAPL at $104.62
TRADE: 67 shares of AAPL at $104.62
TRADE: 12 shares of AAPL at $104.90
TRADE: 12 shares of AAPL at $104.90
TRADE: 12 shares of AAPL at $104.90
TRADE: 12 shares of AAPL at $104.90
TRADE: 9 shares of AAPL at $104.90
TRADE: 5 shares of AAPL at $104.90
TRADE: 62 shares of AAPL at $104.90
TRADE: 26 shares of AAPL at $104.90
TRADE: 36 shares of AAPL at $104.90
TRADE: 57 shares of AAPL at $104.90
TRADE: 5 shares of AAPL at $104.90
TRADE: 7 shares of AAPL at $107.33
TRADE: 7 shares of AAPL at $107.33
TRADE: 7 shares of AAPL at $107.33
TRADE: 7 shares of AAPL at $107.33
TRADE: 82 shares of AAPL at $108.83
TRADE: 12 shares of AAPL at $108.83
TRADE: 70 shares of AAPL at $108.83
TRADE: 24 shares of AAPL at $108.83
TRADE: 58 shares of AAPL at $108.83
TRADE: 36 shares of AAPL at $108.83
TRADE: 46 shares of AAPL at $108.83
TRADE: 20 shares of AAPL at $107.51
TRADE: 12 shares of AAPL at $107.51
TRADE: 8 shares of AAPL at $107.51
TRADE: 20 shares of AAPL at $107.51
TRADE: 4 shares of AAPL at $107.51
TRADE: 16 shares of AAPL at $107.51
TRADE: 16 shares of AAPL at $101.30
TRADE: 32 shares of AAPL at $101.30
TRADE: 27 shares of AAPL at $101.30
TRADE: 21 shares of AAPL at $101.30
TRADE: 54 shares of AAPL at $101.30
TRADE: 6 shares of AAPL at $101.30
TRADE: 28 shares of AAPL at $101.30
TRADE: 28 shares of AAPL at $101.30
TRADE: 13 shares of AAPL at $101.30
TRADE: 15 shares of AAPL at $101.30
TRADE: 28 shares of AAPL at $101.30
TRADE: 32 shares of AAPL at $101.30
TRADE: 62 shares of AAPL at $101.31
TRADE: 11 shares of AAPL at $101.31
TRADE: 73 shares of AAPL at $101.31
TRADE: 10 shares of AAPL at $101.31
TRADE: 63 shares of AAPL at $101.31
TRADE: 31 shares of AAPL at $101.31
TRADE: 42 shares of AAPL at $101.31
TRADE: 10 shares of AAPL at $104.63
TRADE: 10 shares of AAPL at $104.63
TRADE: 10 shares of AAPL at $104.63
TRADE: 1 shares of AAPL at $104.63
TRADE: 9 shares of AAPL at $104.63
TRADE: 52 shares of AAPL at $102.62
TRADE: 11 shares of AAPL at $102.62
TRADE: 53 shares of AAPL at $102.62
TRADE: 10 shares of AAPL at $102.62
TRADE: 63 shares of AAPL at $102.62
TRADE: 23 shares of AAPL at $102.62
TRADE: 40 shares of AAPL at $102.62
TRADE: 22 shares of AAPL at $104.63
TRADE: 4 shares of AAPL at $104.63
TRADE: 26 shares of AAPL at $104.63
TRADE: 1 shares of AAPL at $104.63
TRADE: 25 shares of AAPL at $104.63
TRADE: 6 shares of AAPL at $104.63
TRADE: 20 shares of AAPL at $105.98
TRADE: 56 shares of AAPL at $101.79
TRADE: 25 shares of AAPL at $101.79
TRADE: 71 shares of AAPL at $101.79
TRADE: 9 shares of AAPL at $101.79
TRADE: 1 shares of AAPL at $101.79
TRADE: 8 shares of AAPL at $101.79
TRADE: 9 shares of AAPL at $101.79
TRADE: 9 shares of AAPL at $101.79
TRADE: 55 shares of AAPL at $101.79
TRADE: 36 shares of AAPL at $101.79
TRADE: 45 shares of AAPL at $101.79
TRADE: 78 shares of AAPL at $105.98
TRADE: 2 shares of AAPL at $105.98
TRADE: 76 shares of AAPL at $105.98
TRADE: 24 shares of AAPL at $105.98
TRADE: 54 shares of AAPL at $105.98
TRADE: 46 shares of AAPL at $105.98
TRADE: 32 shares of AAPL at $105.98
TRADE: 1 shares of AAPL at $102.47
TRADE: 1 shares of AAPL at $102.47
TRADE: 1 shares of AAPL at $102.47
TRADE: 1 shares of AAPL at $102.47
TRADE: 42 shares of AAPL at $100.07
TRADE: 44 shares of AAPL at $100.07
TRADE: 47 shares of AAPL at $100.07
TRADE: 39 shares of AAPL at $100.07
TRADE: 52 shares of AAPL at $100.07
TRADE: 34 shares of AAPL at $100.07
TRADE: 2 shares of AAPL at $100.07
TRADE: 40 shares of AAPL at $100.07
TRADE: 40 shares of AAPL at $100.07
TRADE: 4 shares of AAPL at $100.07
TRADE: 56 shares of AAPL at $105.14
TRADE: 13 shares of AAPL at $105.14
TRADE: 43 shares of AAPL at $105.14
TRADE: 26 shares of AAPL at $105.14
TRADE: 30 shares of AAPL at $105.14
TRADE: 39 shares of AAPL at $105.14
TRADE: 17 shares of AAPL at $105.14
TRADE: 36 shares of AAPL at $105.68
TRADE: 16 shares of AAPL at $105.68
TRADE: 36 shares of AAPL at $101.87
TRADE: 40 shares of AAPL at $101.87
TRADE: 40 shares of AAPL at $101.87
TRADE: 8 shares of AAPL at $101.87
TRADE: 8 shares of AAPL at $101.87
TRADE: 8 shares of AAPL at $101.87
TRADE: 8 shares of AAPL at $101.87
TRADE: 4 shares of AAPL at $101.87
TRADE: 38 shares of AAPL at $101.87
TRADE: 38 shares of AAPL at $101.87
TRADE: 4 shares of AAPL at $101.87
TRADE: 42 shares of AAPL at $101.87
TRADE: 30 shares of AAPL at $101.87
TRADE: 12 shares of AAPL at $100.93
TRADE: 44 shares of AAPL at $100.93
TRADE: 40 shares of AAPL at $100.93
TRADE: 4 shares of AAPL at $100.93
TRADE: 44 shares of AAPL at $100.93
TRADE: 44 shares of AAPL at $100.93
TRADE: 4 shares of AAPL at $100.93
TRADE: 47 shares of AAPL at $100.93
TRADE: 49 shares of AAPL at $100.93
TRADE: 2 shares of AAPL at $100.93
TRADE: 51 shares of AAPL at $100.93
TRADE: 43 shares of AAPL at $100.93
TRADE: 14 shares of AAPL at $101.99
TRADE: 4 shares of AAPL at $101.99
TRADE: 10 shares of AAPL at $101.99
TRADE: 8 shares of AAPL at $101.99
TRADE: 6 shares of AAPL at $101.99
TRADE: 12 shares of AAPL at $101.99
TRADE: 2 shares of AAPL at $101.99
TRADE: 16 shares of AAPL at $101.99
TRADE: 2 shares of AAPL at $105.68
TRADE: 18 shares of AAPL at $105.68
TRADE: 18 shares of AAPL at $105.68
TRADE: 18 shares of AAPL at $105.68
TRADE: 36 shares of AAPL at $105.68
TRADE: 4 shares of AAPL at $105.98
TRADE: 40 shares of AAPL at $105.98
TRADE: 24 shares of AAPL at $105.98
TRADE: 14 shares of AAPL at $106.11
TRADE: 2 shares of AAPL at $106.11
TRADE: 12 shares of AAPL at $106.11
TRADE: 14 shares of AAPL at $106.11
TRADE: 14 shares of AAPL at $106.11
TRADE: 26 shares of AAPL at $102.55
TRADE: 26 shares of AAPL at $102.55
TRADE: 21 shares of AAPL at $102.55
TRADE: 5 shares of AAPL at $102.55
TRADE: 26 shares of AAPL at $102.55
TRADE: 8 shares of AAPL at $106.86
TRADE: 8 shares of AAPL at $106.86
TRADE: 8 shares of AAPL at $106.86
TRADE: 8 shares of AAPL at $106.86
TRADE: 42 shares of AAPL at $102.11
TRADE: 40 shares of AAPL at $102.11
TRADE: 33 shares of AAPL at $102.11
TRADE: 49 shares of AAPL at $102.11
TRADE: 24 shares of AAPL at $102.11
TRADE: 58 shares of AAPL at $102.11
TRADE: 6 shares of AAPL at $102.11
TRADE: 64 shares of AAPL at $102.11
TRADE: 12 shares of AAPL at $102.11
TRADE: 52 shares of AAPL at $103.40
TRADE: 47 shares of AAPL at $103.40
TRADE: 17 shares of AAPL at $103.40
TRADE: 82 shares of AAPL at $103.40
TRADE: 6 shares of AAPL at $103.40
TRADE: 88 shares of AAPL at $103.40
TRADE: 5 shares of AAPL at $103.40
TRADE: 83 shares of AAPL at $103.40
TRADE: 16 shares of AAPL at $103.40
TRADE: 48 shares of AAPL at $102.95
TRADE: 24 shares of AAPL at $102.95
TRADE: 24 shares of AAPL at $102.95
TRADE: 15 shares of AAPL at $102.95
TRADE: 33 shares of AAPL at $102.95
TRADE: 6 shares of AAPL at $102.95
TRADE: 39 shares of AAPL at $102.95
TRADE: 3 shares of AAPL at $102.95
TRADE: 14 shares of AAPL at $104.61
TRADE: 14 shares of AAPL at $104.61
TRADE: 8 shares of AAPL at $104.61
TRADE: 6 shares of AAPL at $104.61
TRADE: 14 shares of AAPL at $104.61
TRADE: 11 shares of AAPL at $105.78
TRADE: 11 shares of AAPL at $105.78
TRADE: 11 shares of AAPL at $105.78
TRADE: 11 shares of AAPL at $105.78
TRADE: 14 shares of AAPL at $106.86
TRADE: 24 shares of AAPL at $106.86
TRADE: 54 shares of AAPL at $106.86
TRADE: 16 shares of AAPL at $106.86
TRADE: 62 shares of AAPL at $106.86
TRADE: 8 shares of AAPL at $106.86
TRADE: 70 shares of AAPL at $106.86
TRADE: 83 shares of AAPL at $103.31
TRADE: 13 shares of AAPL at $103.31
TRADE: 70 shares of AAPL at $103.31
TRADE: 26 shares of AAPL at $103.31
TRADE: 57 shares of AAPL at $103.31
TRADE: 39 shares of AAPL at $103.31
TRADE: 44 shares of AAPL at $103.31
TRADE: 57 shares of AAPL at $100.65
TRADE: 6 shares of AAPL at $100.65
TRADE: 51 shares of AAPL at $100.65
TRADE: 12 shares of AAPL at $100.65
TRADE: 45 shares of AAPL at $100.65
TRADE: 18 shares of AAPL at $100.65
TRADE: 39 shares of AAPL at $100.65
TRADE: 35 shares of AAPL at $103.31
TRADE: 17 shares of AAPL at $103.31
TRADE: 18 shares of AAPL at $107.52
TRADE: 9 shares of AAPL at $107.52
TRADE: 26 shares of AAPL at $107.52
TRADE: 1 shares of AAPL at $107.52
TRADE: 27 shares of AAPL at $107.52
TRADE: 7 shares of AAPL at $107.52
TRADE: 20 shares of AAPL at $107.52
TRADE: 50 shares of AAPL at $108.20
TRADE: 21 shares of AAPL at $108.20
TRADE: 49 shares of AAPL at $108.20
TRADE: 22 shares of AAPL at $108.20
TRADE: 48 shares of AAPL at $108.20
TRADE: 23 shares of AAPL at $108.20
TRADE: 47 shares of AAPL at $108.20
TRADE: 24 shares of AAPL at $101.67
TRADE: 7 shares of AAPL at $101.67
TRADE: 31 shares of AAPL at $101.67
TRADE: 31 shares of AAPL at $101.67
TRADE: 7 shares of AAPL at $101.67
TRADE: 24 shares of AAPL at $101.67
TRADE: 47 shares of AAPL at $100.76
TRADE: 47 shares of AAPL at $100.76
TRADE: 6 shares of AAPL at $100.76
TRADE: 41 shares of AAPL at $100.76
TRADE: 47 shares of AAPL at $100.76
TRADE: 3 shares of AAPL at $105.95
TRADE: 3 shares of AAPL at $105.95
TRADE: 3 shares of AAPL at $105.95
TRADE: 3 shares of AAPL at $105.95
TRADE: 60 shares of AAPL at $106.71
TRADE: 60 shares of AAPL at $106.71
TRADE: 60 shares of AAPL at $106.71
TRADE: 60 shares of AAPL at $106.71
TRADE: 64 shares of AAPL at $101.19
TRADE: 15 shares of AAPL at $101.19
TRADE: 49 shares of AAPL at $101.19
TRADE: 30 shares of AAPL at $101.19
TRADE: 34 shares of AAPL at $101.19
TRADE: 45 shares of AAPL at $101.19
TRADE: 19 shares of AAPL at $101.19
TRADE: 60 shares of AAPL at $101.52
TRADE: 8 shares of AAPL at $101.52
TRADE: 22 shares of AAPL at $101.52
TRADE: 42 shares of AAPL at $101.52
TRADE: 4 shares of AAPL at $101.52
TRADE: 38 shares of AAPL at $101.52
TRADE: 30 shares of AAPL at $101.52
TRADE: 12 shares of AAPL at $101.52
TRADE: 12 shares of AAPL at $101.52
TRADE: 44 shares of AAPL at $101.52
TRADE: 31 shares of AAPL at $106.26
TRADE: 16 shares of AAPL at $106.26
TRADE: 15 shares of AAPL at $106.26
TRADE: 31 shares of AAPL at $106.26
TRADE: 1 shares of AAPL at $106.26
TRADE: 30 shares of AAPL at $106.26
TRADE: 56 shares of AAPL at $100.17
TRADE: 22 shares of AAPL at $100.17
TRADE: 78 shares of AAPL at $100.17
TRADE: 76 shares of AAPL at $100.17
TRADE: 2 shares of AAPL at $100.17
TRADE: 74 shares of AAPL at $100.17
TRADE: 4 shares of AAPL at $100.17
TRADE: 17 shares of AAPL at $106.26
TRADE: 47 shares of AAPL at $106.26
TRADE: 4 shares of AAPL at $107.45
TRADE: 68 shares of AAPL at $107.45
TRADE: 12 shares of AAPL at $107.45
TRADE: 56 shares of AAPL at $107.45
TRADE: 28 shares of AAPL at $107.45
TRADE: 40 shares of AAPL at $107.45
TRADE: 15 shares of AAPL at $103.86
TRADE: 15 shares of AAPL at $103.86
TRADE: 15 shares of AAPL at $103.86
TRADE: 15 shares of AAPL at $103.86
TRADE: 3 shares of AAPL at $103.86
TRADE: 27 shares of AAPL at $103.86
TRADE: 30 shares of AAPL at $103.86
TRADE: 6 shares of AAPL at $103.86
TRADE: 24 shares of AAPL at $103.86
TRADE: 30 shares of AAPL at $103.86
TRADE: 9 shares of AAPL at $103.86
TRADE: 63 shares of AAPL at $103.86
TRADE: 19 shares of AAPL at $103.96
TRADE: 19 shares of AAPL at $103.96
TRADE: 19 shares of AAPL at $103.96
TRADE: 19 shares of AAPL at $103.96
TRADE: 31 shares of AAPL at $106.38
TRADE: 6 shares of AAPL at $106.38
TRADE: 25 shares of AAPL at $106.38
TRADE: 12 shares of AAPL at $106.38
TRADE: 19 shares of AAPL at $106.38
TRADE: 18 shares of AAPL at $106.38
TRADE: 13 shares of AAPL at $106.38
TRADE: 5 shares of AAPL at $107.45
TRADE: 5 shares of AAPL at $107.45
TRADE: 5 shares of AAPL at $107.45
TRADE: 5 shares of AAPL at $107.45
TRADE: 24 shares of AAPL at $107.45
TRADE: 5 shares of AAPL at $107.45
TRADE: 29 shares of AAPL at $107.45
TRADE: 29 shares of AAPL at $107.45
TRADE: 21 shares of AAPL at $107.45
TRADE: 6 shares of AAPL at $105.05
TRADE: 2 shares of AAPL at $105.05
TRADE: 4 shares of AAPL at $105.05
TRADE: 6 shares of AAPL at $105.05
TRADE: 6 shares of AAPL at $105.05
TRADE: 8 shares of AAPL at $105.26
TRADE: 6 shares of AAPL at $102.70
TRADE: 6 shares of AAPL at $102.70
TRADE: 2 shares of AAPL at $102.70
TRADE: 4 shares of AAPL at $102.70
TRADE: 6 shares of AAPL at $102.70
TRADE: 4 shares of AAPL at $102.70
TRADE: 8 shares of AAPL at $102.70
TRADE: 6 shares of AAPL at $102.70
TRADE: 6 shares of AAPL at $102.70
TRADE: 8 shares of AAPL at $102.70
TRADE: 46 shares of AAPL at $105.26
TRADE: 9 shares of AAPL at $105.26
TRADE: 37 shares of AAPL at $105.26
TRADE: 26 shares of AAPL at $105.26
TRADE: 20 shares of AAPL at $105.26
TRADE: 43 shares of AAPL at $105.26
TRADE: 3 shares of AAPL at $105.26
TRADE: 26 shares of AAPL at $102.91
TRADE: 26 shares of AAPL at $102.91
TRADE: 26 shares of AAPL at $102.91
TRADE: 2 shares of AAPL at $102.91
TRADE: 24 shares of AAPL at $102.91
TRADE: 56 shares of AAPL at $102.57
TRADE: 27 shares of AAPL at $102.57
TRADE: 53 shares of AAPL at $102.57
TRADE: 30 shares of AAPL at $102.57
TRADE: 50 shares of AAPL at $102.57
TRADE: 4 shares of AAPL at $102.57
TRADE: 12 shares of AAPL at $102.57
TRADE: 17 shares of AAPL at $102.57
TRADE: 57 shares of AAPL at $102.57
TRADE: 26 shares of AAPL at $102.57
TRADE: 17 shares of AAPL at $100.10
TRADE: 17 shares of AAPL at $100.10
TRADE: 14 shares of AAPL at $100.10
TRADE: 3 shares of AAPL at $100.10
TRADE: 17 shares of AAPL at $100.10
TRADE: 44 shares of AAPL at $104.84
TRADE: 13 shares of AAPL at $104.84
TRADE: 31 shares of AAPL at $104.84
TRADE: 26 shares of AAPL at $104.84
TRADE: 18 shares of AAPL at $104.84
TRADE: 39 shares of AAPL at $104.84
TRADE: 5 shares of AAPL at $104.84
TRADE: 2 shares of AAPL at $101.18
TRADE: 2 shares of AAPL at $101.18
TRADE: 2 shares of AAPL at $101.18
TRADE: 2 shares of AAPL at $101.18
TRADE: 52 shares of AAPL at $104.84
TRADE: 48 shares of AAPL at $105.26
TRADE: 12 shares of AAPL at $105.26
TRADE: 18 shares of AAPL at $100.79
TRADE: 18 shares of AAPL at $100.79
TRADE: 18 shares of AAPL at $100.79
TRADE: 18 shares of AAPL at $100.79
TRADE: 34 shares of AAPL at $106.80
TRADE: 21 shares of AAPL at $106.80
TRADE: 13 shares of AAPL at $106.80
TRADE: 34 shares of AAPL at $106.80
TRADE: 8 shares of AAPL at $106.80
TRADE: 26 shares of AAPL at $106.80
TRADE: 24 shares of AAPL at $108.20
TRADE: 5 shares of AAPL at $108.28
TRADE: 51 shares of AAPL at $108.28
TRADE: 4 shares of AAPL at $108.28
TRADE: 16 shares of AAPL at $102.85
TRADE: 3 shares of AAPL at $102.85
TRADE: 19 shares of AAPL at $102.85
TRADE: 19 shares of AAPL at $102.85
TRADE: 19 shares of AAPL at $102.85
TRADE: 40 shares of AAPL at $103.62
TRADE: 48 shares of AAPL at $103.62
TRADE: 52 shares of AAPL at $103.62
TRADE: 9 shares of AAPL at $103.62
TRADE: 9 shares of AAPL at $103.62
TRADE: 9 shares of AAPL at $103.62
TRADE: 9 shares of AAPL at $103.62
TRADE: 5 shares of AAPL at $103.43
TRADE: 5 shares of AAPL at $103.43
TRADE: 5 shares of AAPL at $103.43
TRADE: 5 shares of AAPL at $103.43
TRADE: 13 shares of AAPL at $103.62
TRADE: 33 shares of AAPL at $103.62
TRADE: 33 shares of AAPL at $103.62
TRADE: 9 shares of AAPL at $103.62
TRADE: 24 shares of AAPL at $103.62
TRADE: 64 shares of AAPL at $103.62
TRADE: 35 shares of AAPL at $104.15
TRADE: 58 shares of AAPL at $104.15
TRADE: 41 shares of AAPL at $104.15
TRADE: 52 shares of AAPL at $104.15
TRADE: 47 shares of AAPL at $104.15
TRADE: 46 shares of AAPL at $104.15
TRADE: 53 shares of AAPL at $104.15
TRADE: 40 shares of AAPL at $104.15
TRADE: 24 shares of AAPL at $105.56
TRADE: 24 shares of AAPL at $105.56
TRADE: 9 shares of AAPL at $105.56
TRADE: 15 shares of AAPL at $105.56
TRADE: 24 shares of AAPL at $105.56
TRADE: 18 shares of AAPL at $105.56
TRADE: 52 shares of AAPL at $105.56
TRADE: 5 shares of AAPL at $105.56
TRADE: 57 shares of AAPL at $105.56
TRADE: 8 shares of AAPL at $105.95
TRADE: 55 shares of AAPL at $105.95
TRADE: 15 shares of AAPL at $105.95
TRADE: 48 shares of AAPL at $105.95
TRADE: 22 shares of AAPL at $105.95
TRADE: 23 shares of AAPL at $105.95
TRADE: 18 shares of AAPL at $105.95
TRADE: 5 shares of AAPL at $105.95
TRADE: 23 shares of AAPL at $105.95
TRADE: 23 shares of AAPL at $105.95
TRADE: 12 shares of AAPL at $105.95
TRADE: 48 shares of AAPL at $103.66
TRADE: 21 shares of AAPL at $103.66
TRADE: 39 shares of AAPL at $103.66
TRADE: 30 shares of AAPL at $103.66
TRADE: 30 shares of AAPL at $103.66
TRADE: 39 shares of AAPL at $103.66
TRADE: 21 shares of AAPL at $103.66
TRADE: 8 shares of AAPL at $103.66
TRADE: 40 shares of AAPL at $103.66
TRADE: 8 shares of AAPL at $104.77
TRADE: 48 shares of AAPL at $104.77
TRADE: 27 shares of AAPL at $104.77
TRADE: 21 shares of AAPL at $104.77
TRADE: 13 shares of AAPL at $104.77
TRADE: 13 shares of AAPL at $104.77
TRADE: 13 shares of AAPL at $104.77
TRADE: 13 shares of AAPL at $104.77
TRADE: 10 shares of AAPL at $104.77
TRADE: 49 shares of AAPL at $104.77
TRADE: 34 shares of AAPL at $104.77
TRADE: 25 shares of AAPL at $104.77
TRADE: 58 shares of AAPL at $104.77
TRADE: 1 shares of AAPL at $105.63
TRADE: 59 shares of AAPL at $105.63
TRADE: 28 shares of AAPL at $105.63
TRADE: 72 shares of AAPL at $105.63
TRADE: 16 shares of AAPL at $105.63
TRADE: 84 shares of AAPL at $105.63
TRADE: 4 shares of AAPL at $105.63
TRADE: 88 shares of AAPL at $105.63
TRADE: 8 shares of AAPL at $106.14
TRADE: 64 shares of AAPL at $106.14
TRADE: 36 shares of AAPL at $106.14
TRADE: 36 shares of AAPL at $106.14
TRADE: 49 shares of AAPL at $106.14
TRADE: 23 shares of AAPL at $106.14
TRADE: 62 shares of AAPL at $106.14
TRADE: 10 shares of AAPL at $106.14
TRADE: 75 shares of AAPL at $106.69
TRADE: 25 shares of AAPL at $106.69
TRADE: 60 shares of AAPL at $106.69
TRADE: 40 shares of AAPL at $106.69
TRADE: 42 shares of AAPL at $106.69
TRADE: 58 shares of AAPL at $106.69
TRADE: 24 shares of AAPL at $106.69
TRADE: 76 shares of AAPL at $106.69
TRADE: 6 shares of AAPL at $108.28
TRADE: 46 shares of AAPL at $108.28
TRADE: 36 shares of AAPL at $108.28
TRADE: 71 shares of AAPL at $101.93
TRADE: 11 shares of AAPL at $101.93
TRADE: 60 shares of AAPL at $101.93
TRADE: 22 shares of AAPL at $101.93
TRADE: 49 shares of AAPL at $101.93
TRADE: 33 shares of AAPL at $101.93
TRADE: 38 shares of AAPL at $101.93
TRADE: 38 shares of AAPL at $101.93
TRADE: 6 shares of AAPL at $101.93
TRADE: 32 shares of AAPL at $104.21
TRADE: 38 shares of AAPL at $104.21
TRADE: 14 shares of AAPL at $104.21
TRADE: 24 shares of AAPL at $104.21
TRADE: 32 shares of AAPL at $104.21
TRADE: 28 shares of AAPL at $104.21
TRADE: 4 shares of AAPL at $104.21
TRADE: 32 shares of AAPL at $104.21
TRADE: 32 shares of AAPL at $104.21
TRADE: 23 shares of AAPL at $100.62
TRADE: 11 shares of AAPL at $100.62
TRADE: 12 shares of AAPL at $100.62
TRADE: 22 shares of AAPL at $100.62
TRADE: 1 shares of AAPL at $100.62
TRADE: 23 shares of AAPL at $100.62
TRADE: 16 shares of AAPL at $104.21
TRADE: 69 shares of AAPL at $104.21
TRADE: 15 shares of AAPL at $104.21
TRADE: 69 shares of AAPL at $104.79
TRADE: 1 shares of AAPL at $104.79
TRADE: 68 shares of AAPL at $104.79
TRADE: 17 shares of AAPL at $104.79
TRADE: 52 shares of AAPL at $104.79
TRADE: 33 shares of AAPL at $104.79
TRADE: 36 shares of AAPL at $104.79
TRADE: 41 shares of AAPL at $102.27
TRADE: 26 shares of AAPL at $102.27
TRADE: 51 shares of AAPL at $102.27
TRADE: 16 shares of AAPL at $102.27
TRADE: 61 shares of AAPL at $102.27
TRADE: 6 shares of AAPL at $102.27
TRADE: 67 shares of AAPL at $102.27
TRADE: 4 shares of AAPL at $103.07
TRADE: 19 shares of AAPL at $103.07
TRADE: 23 shares of AAPL at $103.07
TRADE: 23 shares of AAPL at $103.07
TRADE: 23 shares of AAPL at $103.07
TRADE: 14 shares of AAPL at $104.27
TRADE: 14 shares of AAPL at $104.27
TRADE: 14 shares of AAPL at $104.27
TRADE: 14 shares of AAPL at $104.27
TRADE: 14 shares of AAPL at $101.78
TRADE: 14 shares of AAPL at $101.78
TRADE: 13 shares of AAPL at $101.78
TRADE: 1 shares of AAPL at $101.78
TRADE: 14 shares of AAPL at $101.78
TRADE: 5 shares of AAPL at $101.78
TRADE: 21 shares of AAPL at $101.78
TRADE: 41 shares of AAPL at $101.78
TRADE: 31 shares of AAPL at $101.78
TRADE: 10 shares of AAPL at $101.78
TRADE: 49 shares of AAPL at $100.32
TRADE: 34 shares of AAPL at $100.32
TRADE: 15 shares of AAPL at $100.32
TRADE: 49 shares of AAPL at $100.32
TRADE: 29 shares of AAPL at $100.32
TRADE: 10 shares of AAPL at $100.32
TRADE: 10 shares of AAPL at $100.32
TRADE: 18 shares of AAPL at $100.55
TRADE: 6 shares of AAPL at $100.55
TRADE: 12 shares of AAPL at $100.55
TRADE: 18 shares of AAPL at $100.55
TRADE: 18 shares of AAPL at $100.55
TRADE: 9 shares of AAPL at $104.27
TRADE: 23 shares of AAPL at $104.27
TRADE: 32 shares of AAPL at $104.27
TRADE: 10 shares of AAPL at $104.27
TRADE: 22 shares of AAPL at $104.27
TRADE: 32 shares of AAPL at $104.27
TRADE: 21 shares of AAPL at $103.13
TRADE: 16 shares of AAPL at $103.13
TRADE: 37 shares of AAPL at $103.13
TRADE: 16 shares of AAPL at $103.13
TRADE: 21 shares of AAPL at $103.13
TRADE: 37 shares of AAPL at $103.13
TRADE: 11 shares of AAPL at $104.27
TRADE: 36 shares of AAPL at $104.27
TRADE: 29 shares of AAPL at $104.27
TRADE: 3 shares of AAPL at $107.61
TRADE: 3 shares of AAPL at $107.61
TRADE: 3 shares of AAPL at $107.61
TRADE: 3 shares of AAPL at $107.61
TRADE: 6 shares of AAPL at $103.24
TRADE: 41 shares of AAPL at $103.24
TRADE: 6 shares of AAPL at $103.24
TRADE: 41 shares of AAPL at $103.24
TRADE: 6 shares of AAPL at $103.24
TRADE: 41 shares of AAPL at $103.24
TRADE: 38 shares of AAPL at $103.24
TRADE: 9 shares of AAPL at $103.24
TRADE: 21 shares of AAPL at $106.29
TRADE: 5 shares of AAPL at $106.29
TRADE: 16 shares of AAPL at $106.29
TRADE: 10 shares of AAPL at $106.29
TRADE: 11 shares of AAPL at $106.29
TRADE: 15 shares of AAPL at $106.29
TRADE: 6 shares of AAPL at $106.29
TRADE: 20 shares of AAPL at $106.29
TRADE: 25 shares of AAPL at $106.92
TRADE: 25 shares of AAPL at $106.92
TRADE: 20 shares of AAPL at $106.92
TRADE: 30 shares of AAPL at $106.92
TRADE: 15 shares of AAPL at $106.92
TRADE: 35 shares of AAPL at $106.92
TRADE: 10 shares of AAPL at $106.92
TRADE: 35 shares of AAPL at $106.92
TRADE: 5 shares of AAPL at $106.92
TRADE: 30 shares of AAPL at $101.58
TRADE: 31 shares of AAPL at $101.58
TRADE: 4 shares of AAPL at $101.58
TRADE: 35 shares of AAPL at $101.58
TRADE: 22 shares of AAPL at $101.58
TRADE: 48 shares of AAPL at $101.58
TRADE: 13 shares of AAPL at $101.58
TRADE: 61 shares of AAPL at $101.58

===== BENCHMARK RESULTS =====
Total Orders: 1000
Total Time: 319 ms
Throughput: 3134.8 orders/sec
=============================

Enter Command (buy/sell/show/sim/replay/strat/exit): replay
Enter file path: sample_replay.txt

Enter Command (buy/sell/show/sim/replay/strat/exit): strat

Enter Command (buy/sell/show/sim/replay/strat/exit): exit

Process finished with exit code 0

*/
