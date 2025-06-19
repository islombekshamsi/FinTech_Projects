import numpy as np
import matplotlib.pyplot as plt

def monte_carlo_call_option_price(S, K, T, r, sigma, num_simulations = 10000):
    np.random.seed(42)

    Z = np.random.standard_normal(num_simulations)

    ST = S * np.exp((r- 0.5 * sigma**2) * T + sigma * np.sqrt(T) * Z)

    payoffs = np.maximum(ST - K, 0)

    call_price = np.exp(-r*T) *np.mean(payoffs)

    return call_price, ST

S = 100
K = 105
T = 0.5
r = 0.05
sigma = 0.20

mc_price, stock_paths = monte_carlo_call_option_price(S, K, T, r, sigma)
print(f"Monte Carlo Estimated Call Price: ${mc_price: .2f}")

# Sample Output:
# Monte Carlo Estimated Call Price: $ 4.58