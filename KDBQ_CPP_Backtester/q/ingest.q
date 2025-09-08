/
 Ingest CSV or generate synthetic quotes/trades into kdb+.
 Usage:
   q ingest.q sym:`DEMO date:2025.09.03 quotes:`../data/sample/quotes.csv db:`../db
 If quotes is omitted, synthetic L1 quotes are generated and saved both to kdb and CSV.
*/

\l .Q.q

/ ensure args
if[not `sym in key `.z; sym:`DEMO];
if[not `date in key `.z; date:2025.09.03];
if[not `db in key `.z; db:`../db];
if[not `quotes in key `.z; quotes:`];

/ util
ensureDir:{[p] system "mkdir -p ",string p; p }

/ table schemas
/ splayed tables under db/date/sym
mkSchema:{
  trades:([] ts:0Nt; sym:`symbol$(); px:0n; sz:0N; side:`$());
  quotes:([] ts:0Nt; sym:`symbol$(); bid:0n; ask:0n; bsz:0N; asz:0N);
  `trades`quotes!(trades;quotes)
}

/ parse CSV (flexible)
readCSV:{[p]
  t:0N!0N; / placeholder
  / Use .Q.f to parse CSV into table (expects header)
  / .Q.f reads file and infers types
  t:.Q.f p;
  t
}

/ synth generator for L1 quotes
synthQuotes:{[sym; n; start; dt]
  / random walk mid; fixed spread
  s:([] ts:start+til n*dt; sym:sym; mid:100f+sv prev 0.01*rand n; spr:0.02+0.02*rand n);
  q:select ts,sym,bid:mid-0.5*spr,ask:mid+0.5*spr,bsz:100+10?500,asz:100+10?500 from s;
  q
}

/ write splayed
writeSplayed:{[db;date;tab;tbl]
  p:db,date;
  ensureDir p;
  @[hsym p set enlist[`sym]!enlist asc distinct tbl`sym; ::];
  `sv[hsym p],string tab set tbl
  / partitioned by date folder; simple demo splay
}

/ main
tabs:mkSchema[];
qtab:$[quotes~`; synthQuotes[sym; n:10000; start:date 00:00:00.000000000; dt:0D00:00:00.010]; readCSV quotes];
qtab:update sym:sym from qtab;
writeSplayed[db; string date; `quotes; qtab];
show count qtab;
if[quotes~`; / also dump CSV for convenience
  `:../data/sample/quotes.csv 0: enlist .Q.t qtab
];
"done"
