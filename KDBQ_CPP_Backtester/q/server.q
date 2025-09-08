/
 Optional q server stubs. Run: q -p 5000 server.q
*/
/ define API namespace
.api.getWindow:{[sym;t0;t1;tab:`quotes] select from value tab where sym=sym, ts within (t0;t1) }
.api.bestBidAskAt:{[sym;ts] 1 xdesc select by ts from quotes where sym=sym, ts<=ts }
"server ready"
