#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include "Engine.h"

using namespace std;

static vector<string> splitCSV(const string& line) {
    vector<string> out; string cur; bool inq=false;
    for (char c: line) {
        if (c=='"') inq=!inq;
        else if (c==',' && !inq){ out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

static vector<BookTick> loadQuotes(const string& path) {
    ifstream f(path);
    if(!f) throw runtime_error("cannot open quotes: "+path);
    string line; getline(f, line); // header
    vector<BookTick> v; v.reserve(1<<20);
    while (getline(f, line)) {
        auto cols = splitCSV(line);
        if (cols.size()<6) continue;
        BookTick b;
        b.ts = cols[0];
        b.bid = stod(cols[2]);
        b.ask = stod(cols[3]);
        b.bsz = stoi(cols[4]);
        b.asz = stoi(cols[5]);
        v.push_back(b);
    }
    return v;
}

static vector<Order> loadOrders(const string& path) {
    ifstream f(path);
    if(!f) throw runtime_error("cannot open orders: "+path);
    string line; getline(f, line); // header
    vector<Order> v;
    int id=1;
    while (getline(f, line)) {
        auto cols = splitCSV(line);
        if (cols.size()<7) continue;
        Order o;
        o.ts = cols[0];
        o.sym = cols[1];
        o.side = cols[2];
        o.type = cols[3];
        o.px = stod(cols[4]);
        o.qty = stoi(cols[5]);
        o.tif = cols[6];
        o.id = id++;
        v.push_back(o);
    }
    return v;
}

static void ensureDir(const string& p) {
    string cmd = "mkdir -p \""+p+"\"";
    system(cmd.c_str());
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    string quotesPath, ordersPath, outdir="artifact";
    EngineParams params;
    for (int i=1;i<argc;i++){
        string a=argv[i];
        auto get=[&](string k){ if(i+1>=argc) throw runtime_error("missing "+k); return string(argv[++i]); };
        if (a=="--quotes") quotesPath=get("--quotes");
        else if (a=="--orders") ordersPath=get("--orders");
        else if (a=="--out") outdir=get("--out");
        else if (a=="--latency_ticks") params.latency_ticks=stoi(get("--latency_ticks"));
        else if (a=="--slip_bps") params.slip_bps=stod(get("--slip_bps"));
        else cerr<<"unknown arg "<<a<<"\n";
    }
    if (quotesPath.empty()||ordersPath.empty()) {
        cerr<<"Usage: backtester --quotes <quotes.csv> --orders <orders.csv> --out <dir> --latency_ticks N --slip_bps B\n";
        return 2;
    }
    ensureDir(outdir);

    auto quotes = loadQuotes(quotesPath);
    auto orders = loadOrders(ordersPath);

    // index quotes by "ts" string into position
    unordered_map<string,int> qidx;
    qidx.reserve(quotes.size()*2);
    for (int i=0;i<(int)quotes.size();++i) qidx[quotes[i].ts]=i;

    vector<Fill> fills;
    fills.reserve(orders.size()*2);

    auto slip = [&](double px, bool isBuy){
        double s = params.slip_bps/10000.0 * px;
        return isBuy ? px + s : px - s;
    };

    for (auto &o: orders){
        auto it = qidx.find(o.ts);
        if (it==qidx.end()) continue; // skip if timestamp not found
        int arrival = it->second + params.latency_ticks;
        if (arrival >= (int)quotes.size()) continue;

        if (o.type=="market"){
            auto &qt = quotes[arrival];
            if (o.side=="buy"){
                int qty = min(o.qty, qt.asz);
                if (qty>0){
                    Fill f{ o.id, qt.ts, slip(qt.ask,true), qty, o.side, "taker" };
                    fills.push_back(f);
                }
            } else {
                int qty = min(o.qty, qt.bsz);
                if (qty>0){
                    Fill f{ o.id, qt.ts, slip(qt.bid,false), qty, o.side, "taker" };
                    fills.push_back(f);
                }
            }
        } else if (o.type=="limit"){
            // Scan forward until condition met or end (GFD) / immediate (IOC)
            bool filled=false;
            int start = arrival;
            int end = (o.tif=="IOC" ? arrival : (int)quotes.size()-1);
            for (int i=start;i<=end;i++){
                auto &qt = quotes[i];
                if (o.side=="buy"){
                    if (qt.ask <= o.px){
                        int qty = min(o.qty, qt.asz);
                        if (qty>0){
                            Fill f{ o.id, qt.ts, qt.ask, qty, o.side, "taker" };
                            fills.push_back(f); filled=true; break;
                        }
                    }
                } else {
                    if (qt.bid >= o.px){
                        int qty = min(o.qty, qt.bsz);
                        if (qty>0){
                            Fill f{ o.id, qt.ts, qt.bid, qty, o.side, "taker" };
                            fills.push_back(f); filled=true; break;
                        }
                    }
                }
            }
            (void)filled;
        }
    }

    // Write fills.csv and simple pnl.csv (mark to mid at same timestamp if available)
    string fillsPath = outdir + "/fills.csv";
    ofstream ff(fillsPath);
    ff<<"ts,order_id,side,px,qty,liq\n";
    for (auto &f: fills){
        ff<<f.ts<<","<<f.order_id<<","<<f.side<<","<<fixed<<setprecision(8)<<f.px<<","<<f.qty<<","<<f.liq<<"\n";
    }
    ff.close();

    cerr<<"Wrote "<<fills.size()<<" fills to "<<fillsPath<<"\n";
    return 0;
}
