from __future__ import annotations

import os
from typing import Sequence

from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer
import requests

BASE_URL = "https://gnews.io/api/v4/search"

def get_sentiment_scores(
    tickers: Sequence[str],
    api_key: str | None = None,
    max_articles: int = 5,
    timeout_seconds: int = 10,
) -> dict[str, float | str]:
    """Fetch recent news and compute a compound sentiment score."""
    analyzer = SentimentIntensityAnalyzer()
    scores = {}
    resolved_key = api_key or os.getenv("GNEWS_API_KEY")
    if not resolved_key:
        return {ticker: "Missing API key" for ticker in tickers}

    for ticker in tickers:
        query = f"{ticker} stock"
        url = f"{BASE_URL}?q={query}&lang=en&max={max_articles}&token={resolved_key}"
        try:
            response = requests.get(url, timeout=timeout_seconds)
            response.raise_for_status()
            articles = response.json().get("articles", [])
            titles = [article["title"] for article in articles]
            if not titles:
                scores[ticker] = "No data"
                continue
            score = sum(analyzer.polarity_scores(t)["compound"] for t in titles) / len(titles)
            scores[ticker] = round(score, 3)
        except requests.RequestException:
            scores[ticker] = "N/A"

    return scores
