/
 Generate signals and orders from quotes and orchestrate the engine.
 Usage:
   q run.q sym:`DEMO date:2025.09.03 quotes:`../data/sample/quotes.csv outdir:`../artifact
*/

if[not `sym in key `.z; sym:`DEMO];
if[not `date in key `.z; date:2025.09.03];
if[not `quotes in key `.z; quotes:`../data/sample/quotes.csv];
if[not `outdir in key `.z; outdir:`../artifact];

system "mkdir -p ",string outdir;

/ load quotes (expects header)
qtab:.Q.f quotes;
qtab:update ts:`timestamp$ts, sym:`symbol$sym, bid:`float$bid, ask:`float$ask, bsz:`int$bsz, asz:`int$asz from qtab;
qtab:update mid:(bid+ask)%2f from qtab;

/ simple mean-reversion signal (short vs long SMA on mid)
sm:{[x;n] sum x til n}%n xexp til count x } / simple moving avg helper
/ rolling SMA
sma:{[x;n] n#x; (n-1)#0N; }
sma:{[x;n] raze prev n#(sum x) - prev n#(sum x),/not exact but keep simple }

/ We'll just use q's built-ins: mavg
qtab:update smaS:5 mavg mid, smaL:20 mavg mid from qtab;
sig:select from qtab where smaS> smaL, prev smaS<=prev smaL; / bullish crossover
sig10:first 100 sig;

/ create simple market orders on crossovers
mkOrders:{[sym; s]
  ([] ts:s`ts; sym:sym; side:`$ "buy"; type:`$ "market"; px:s`ask; qty:100; tif:`$ "IOC")
}

orders:mkOrders[sym; sig10];
`:,(string outdir),"/orders.csv" 0: enlist .Q.t orders;
show "orders written: ", string count orders;

/ End: user runs C++ engine against orders + quotes
/ Example (from repo root):
/ ./cpp/build/engine/backtester --quotes data/sample/quotes.csv --orders artifact/orders.csv --out artifact --latency_ticks 2 --slip_bps 0.5

"done"
