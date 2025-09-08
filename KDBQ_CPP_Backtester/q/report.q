/
 Report PnL and basic stats.
 Usage:
   q report.q fills:`../artifact/fills.csv quotes:`../data/sample/quotes.csv out:`../artifact/report.csv
*/
if[not `fills in key `.z; fills:`../artifact/fills.csv];
if[not `quotes in key `.z; quotes:`../data/sample/quotes.csv];
if[not `out in key `.z; out:`../artifact/report.csv];

ft:.Q.f fills;
qt:.Q.f quotes;
ft:update ts:`timestamp$ts, px:`float$px, qty:`int$qty, side:`symbol$side from ft;
qt:update ts:`timestamp$ts, mid:0.5*(`float$bid+`float$ask) from qt;
j:aj[`ts; ft; qt`ts`mid];
/ PnL: side buy -> -qty*px + qty*mid; sell -> qty*px - qty*mid
pnlcalc:{[side;qty;px;mid] $[side=`buy; -qty*px + qty*mid; qty*px - qty*mid]}
j:update rPnL:pnlcalc[side;qty;px;mid] from j;
stats:select trades:count i, pnl:sum rPnL, mean:rPnL avg rPnL, stdev:dev rPnL, sharpe: (rPnL avg rPnL)%dev rPnL from j;
`: ,string out 0: enlist .Q.t j;
show stats;
"done"
