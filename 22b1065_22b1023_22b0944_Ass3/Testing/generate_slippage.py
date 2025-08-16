import numpy as np
import matplotlib.pyplot as plt

# Extend trade fractions from 0.001 up to 5 (to allow f > 1)
f_values = np.linspace(0.001, 30, 1000)

# Compute slippage using the formula: S = -f / (1 + f)
slippage_percent = -f_values / (1 + f_values) * 100  # convert to percentage

# Plot
plt.figure(figsize=(10, 6))
plt.plot(f_values, slippage_percent, label='Slippage (%)', color='purple')
plt.title("Slippage vs Trade Fraction (f) for Constant Product AMM")
plt.xlabel("Trade Fraction (f)")
plt.ylabel("Slippage (%)")
plt.grid(True)
plt.axhline(0, color='black', linewidth=0.8, linestyle='--')
plt.legend()
plt.tight_layout()
plt.savefig("slippage vs fraction.png")
plt.show()
