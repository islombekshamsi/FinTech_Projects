import streamlit as st
from portfolio import get_portfolio_metrics
from sentiment import get_sentiment_scores

st.set_page_config(page_title="Quant Risk Engine", layout="wide")
st.title("Real-Time Risk & Sentiment Engine")

tickers = st.text_input("Enter tickers (comma-separated): ", "AAPL, TSLA, MSFT")
weights = st.text_input("Enter weights (comma-separated):", "0.3, 0.5, 0.2")

if st.button("Analyze"):
    tickers = [t.strip().upper() for t in tickers.split(",")]
    weights = [float(w.strip()) for w in weights.split(",")]

    st.subheader("Portfolio Risk Metrics")
    metrics = get_portfolio_metrics(tickers, weights)
    st.write(metrics)

    st.subheader("Sentiment Analysis")
    sentiment = get_sentiment_scores(tickers)
    st.write(sentiment)