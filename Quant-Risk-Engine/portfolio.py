import yfinance as yf
import numpy as np
import pandas as pd

def get_portfolio_metrics(tickers, weights):
    raw = yf.download(tickers, period="3mo", group_by="ticker", auto_adjust=True)

    if isinstance(raw.columns, pd.MultiIndex):
        data = pd.concat([raw[ticker]["Close"] for ticker in tickers], axis=1)
    else:
        data = raw["Close"].to_frame() if len(tickers) == 1 else raw[tickers]

    data.columns = tickers
    returns = data.pct_change().dropna()
    weighted_returns = returns @ np.array(weights)
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