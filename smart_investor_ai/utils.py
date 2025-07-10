import requests
import pandas as pd
import numpy as np
import random

API_KEY = "9c0c2644a3d74d56b8ef7fe4d2a5279f"  # 🔁 Replace this with your actual Twelve Data API key

# Ticker pools by risk level
STOCK_POOLS = {
    "Low": [
        "AAPL", "MSFT", "JNJ", "PG", "KO", "PEP", "WMT", "V", "MA", "COST",
        "HD", "UNH", "MRK", "ABT", "LLY", "MCD", "T", "VZ", "CSCO", "MDT"
    ],
    "Medium": [
        "GOOGL", "AMZN", "META", "DIS", "NVDA", "ADBE", "CRM", "INTC", "PYPL", "NFLX",
        "AVGO", "QCOM", "TXN", "SBUX", "BKNG", "NOW", "UBER", "AMAT", "ORCL", "IBM"
    ],
    "High": [
        "TSLA", "AMD", "PLTR", "NIO", "COIN", "RIVN", "SQ", "ROKU", "SPOT", "AFRM",
        "UPST", "BIDU", "SNOW", "SHOP", "ARKK", "DOCN", "TWLO", "NET", "CRWD", "DDOG"
    ]
}

FALLBACK_TICKERS = ['AAPL', 'MSFT', 'GOOGL', 'AMZN', 'NVDA']

def fetch_twelve_data(tickers, interval="1day", outputsize=100):
    base_url = "https://api.twelvedata.com/time_series"
    prices = {}

    for ticker in tickers:
        params = {
            "symbol": ticker,
            "interval": interval,
            "outputsize": outputsize,
            "apikey": API_KEY,
        }
        response = requests.get(base_url, params=params)
        data = response.json()

        if "values" in data:
            df = pd.DataFrame(data["values"])
            df["datetime"] = pd.to_datetime(df["datetime"])
            df.set_index("datetime", inplace=True)
            df = df.sort_index()
            df["close"] = pd.to_numeric(df["close"])
            prices[ticker] = df["close"]
        else:
            print(f"⚠️ Failed to fetch {ticker}: {data.get('message')}")

    if not prices:
        raise ValueError("❌ Could not fetch data for selected stocks.")

    return pd.DataFrame(prices)

def generate_portfolio(investment, risk, strategy, num_stocks):
    # Select tickers
    if strategy == "Stability":
        pool = STOCK_POOLS["Low"]
    elif strategy == "Balanced":
        pool = STOCK_POOLS["Low"] + STOCK_POOLS["Medium"]
    else:
        pool = STOCK_POOLS["Medium"] + STOCK_POOLS["High"]

    selected = random.sample(pool, min(num_stocks, len(pool)))
    tickers = selected + [t for t in FALLBACK_TICKERS if t not in selected]

    price_df = fetch_twelve_data(tickers)
    if price_df.empty:
        raise ValueError("❌ No valid data returned.")

    selected_final = list(price_df.columns)[:num_stocks]
    prices = price_df[selected_final]

    # Calculate returns
    daily_returns = prices.pct_change().dropna()
    mean_returns = daily_returns.mean()
    volatility = daily_returns.std()

    # ✅ Smart weighting by expected return
    clean_returns = mean_returns.clip(lower=0.0001)  # avoid division by zero
    weights = clean_returns / clean_returns.sum()
    allocations = weights * investment

    # Portfolio DataFrame
    portfolio_df = pd.DataFrame({
        "Ticker": selected_final,
        "Expected Daily Return (%)": np.round(mean_returns.values * 100, 2),
        "Volatility (%)": np.round(volatility.values * 100, 2),
        "Weight (%)": np.round(weights.values * 100, 2),
        "Allocation ($)": np.round(allocations.values, 2)
    })

    # Forecast
    expected_return = (mean_returns @ weights) * 252
    std_dev = np.sqrt(weights.T @ daily_returns.cov().values @ weights) * np.sqrt(252)
    sharpe_ratio = expected_return / std_dev if std_dev > 0 else 0

    summary = f"""
    - 📈 **Expected Annual Return:** `{expected_return * 100:.2f}%`  
    - ⚖️ **Expected Annual Volatility:** `{std_dev * 100:.2f}%`  
    - 🧪 **Sharpe Ratio (simplified):** `{sharpe_ratio:.2f}`
    """

    return portfolio_df, summary