from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer
import requests

API_KEY = "cf4a82108b0c6c8cc8b2adda697792f0"  # <-- paste your key here
BASE_URL = "https://gnews.io/api/v4/search"

def get_sentiment_scores(tickers):
    analyzer = SentimentIntensityAnalyzer()
    scores = {}

    for ticker in tickers:
        query = f"{ticker} stock"
        url = f"{BASE_URL}?q={query}&lang=en&max=5&token={API_KEY}"
        try:
            response = requests.get(url)
            articles = response.json().get("articles", [])
            titles = [article["title"] for article in articles]
            if not titles:
                scores[ticker] = "No data"
                continue
            score = sum(analyzer.polarity_scores(t)["compound"] for t in titles) / len(titles)
            scores[ticker] = round(score, 3)
        except Exception as e:
            scores[ticker] = "N/A"

    return scores