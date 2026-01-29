from __future__ import annotations

from typing import Iterable, Sequence

import numpy as np
import pandas as pd
import yfinance as yf

def _validate_inputs(tickers: Sequence[str], weights: Sequence[float]) -> None:
    if not tickers:
        raise ValueError("At least one ticker is required.")
    if not weights:
        raise ValueError("At least one weight is required.")
    if len(tickers) != len(weights):
        raise ValueError("Tickers and weights must have the same length.")
    if any(w < 0 for w in weights):
        raise ValueError("Weights must be non-negative.")


def get_portfolio_metrics(tickers: Sequence[str], weights: Sequence[float]) -> dict[str, float]:
    """Compute basic portfolio risk metrics from recent prices."""
    _validate_inputs(tickers, weights)

    raw = yf.download(list(tickers), period="3mo", group_by="ticker", auto_adjust=True)

    if isinstance(raw.columns, pd.MultiIndex):
        data = pd.concat([raw[ticker]["Close"] for ticker in tickers], axis=1)
    else:
        data = raw["Close"].to_frame() if len(tickers) == 1 else raw[tickers]

    data.columns = list(tickers)
    returns = data.pct_change().dropna()
    if returns.empty:
        raise ValueError("Not enough price data to compute returns.")
    weights_array = np.array(weights, dtype=float)
    weighted_returns = returns @ weights_array
    portfolio_returns = pd.Series(weighted_returns)

    mean_return = portfolio_returns.mean()
    std_dev = portfolio_returns.std()
    var_95 = np.percentile(portfolio_returns, 5)
    cvar_95 = portfolio_returns[portfolio_returns <= var_95].mean()
    sharpe = mean_return / std_dev if std_dev else 0

    return {
        "Mean Daily Return": round(mean_return, 5),
        "Volatility": round(std_dev, 5),
        "Value at Risk (95%)": round(var_95, 5),
        "Conditional VaR (95%)": round(cvar_95, 5),
        "Sharpe Ratio": round(sharpe, 3)
    }
