import streamlit as st
import numpy as np
from scipy.stats import norm
import matplotlib.pyplot as plt

def monte_carlo_call_option_price(S, K, T, r, sigma, num_simulations=10000):
    Z = np.random.standard_normal(num_simulations)
    ST = S * np.exp((r - 0.5 * sigma**2) * T + sigma * np.sqrt(T) * Z)
    payoffs = np.maximum(ST - K, 0)
    call_price = np.exp(-r * T) * np.mean(payoffs)
    return call_price

def black_scholes_price(S, K, T, r, sigma):
    d1 = (np.log(S / K) + (r + 0.5 * sigma**2) * T) / (sigma * np.sqrt(T))
    d2 = d1 - sigma * np.sqrt(T)
    return S * norm.cdf(d1) - K * np.exp(-r * T) * norm.cdf(d2)

st.title("Monte Carlo Option Pricing")

st.sidebar.header("Option Parameters")

S = st.sidebar.slider("Stock Price (S)", 50, 150, 100)
K = st.sidebar.slider("Strike Price (K)", 50, 150, 105)
T = st.sidebar.slider("Time to Maturity (T in years)", 0.01, 2.0, 0.5, 0.01)
r = st.sidebar.slider("Risk-Free Rate (r)", 0.0, 0.1, 0.05, 0.005)
sigma = st.sidebar.slider("Volatility (σ)", 0.01, 1.0, 0.2, 0.01)
num_sim = st.sidebar.slider("Number of Simulations", 1000, 50000, 10000, 1000)

mc_price = monte_carlo_call_option_price(S, K, T, r, sigma, num_sim)
bsm_price = black_scholes_price(S, K, T, r, sigma)

st.subheader("Results")
st.write(f"Monte Carlo Estimated Call Price: ${mc_price: .2f}")
st.write(f"**Black-Scholes Call Price:** ${bsm_price:.2f}")
st.write(f"**Difference:** ${abs(mc_price - bsm_price):.4f}")

Z = np.random.standard_normal(num_sim)
ST = S * np.exp((r - 0.5 * sigma**2) * T + sigma * np.sqrt(T) * Z)

fig, ax = plt.subplots()
ax.hist(ST, bins=50, color="skyblue", edgecolor="black")
ax.axvline(K, color='red', linestyle='--', label="Strike Price")
ax.set_title("Simulated Stock Prices at Expiration")
ax.set_xlabel("Price")
ax.set_ylabel("Frequency")
ax.legend()
st.pyplot(fig)