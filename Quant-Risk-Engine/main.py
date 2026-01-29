from __future__ import annotations

import streamlit as st

from portfolio import get_portfolio_metrics
from sentiment import get_sentiment_scores

st.set_page_config(page_title="Quant Risk Engine", layout="wide")
st.title("Real-Time Risk & Sentiment Engine")

tickers = st.text_input("Enter tickers (comma-separated): ", "AAPL, TSLA, MSFT")
weights = st.text_input("Enter weights (comma-separated):", "0.3, 0.5, 0.2")
st.caption("Tip: weights are normalized if they don't sum to 1.0.")

if st.button("Analyze"):
    try:
        tickers_list = [t.strip().upper() for t in tickers.split(",") if t.strip()]
        weights_list = [float(w.strip()) for w in weights.split(",") if w.strip()]
    except ValueError:
        st.error("Please enter valid numeric weights.")
        st.stop()

    if not tickers_list:
        st.error("Please enter at least one ticker.")
        st.stop()
    if len(tickers_list) != len(weights_list):
        st.error("Tickers and weights must have the same count.")
        st.stop()
    weight_sum = sum(weights_list)
    if weight_sum <= 0:
        st.error("Weights must sum to a positive number.")
        st.stop()
    if abs(weight_sum - 1.0) > 1e-6:
        st.warning("Weights were normalized to sum to 1.0.")
        weights_list = [w / weight_sum for w in weights_list]

    st.subheader("Portfolio Risk Metrics")
    try:
        metrics = get_portfolio_metrics(tickers_list, weights_list)
        st.write(metrics)
    except ValueError as exc:
        st.error(str(exc))
        st.stop()

    st.subheader("Sentiment Analysis")
    sentiment = get_sentiment_scores(tickers_list)
    st.write(sentiment)
