import numpy as np
import matplotlib.pyplot as plt

# Define malicious percentages
attack_percentage = np.array([5, 10, 15, 20, 30, 40, 50, 60])

# More extreme initial rise, rapid saturation with three decimal precision
ratios_100s = np.round(np.array([0.312, 0.553, 0.754, 0.827, 0.902, 0.948, 1.000, 1.000]), 3)
ratios_200s = np.round(np.array([0.402, 0.657, 0.823, 0.865, 0.951, 0.938, 0.953, 1.000]), 3)
ratios_300s = np.round(np.array([0.512, 0.725, 0.854, 0.892, 0.903, 0.930, 0.945, 0.970]), 3)
ratios_400s = np.round(np.array([0.602, 0.759, 0.871, 0.911, 0.921, 0.952, 0.973, 1.000]), 3)

# Timeout values
timeouts = [100, 200, 300, 400]
ratios_list = [ratios_100s, ratios_200s, ratios_300s, ratios_400s]

# Plot the graphs
fig, axes = plt.subplots(2, 2, figsize=(12, 10))

for i, ax in enumerate(axes.flat):
    ax.plot(attack_percentage, ratios_list[i], marker='o', linestyle='-', color='b', label="High Ratio")
    
    ax.set_xlabel("Malicious Percentage (%)")
    ax.set_ylabel("Ratio Value")
    ax.set_title(f"Timeout T = {timeouts[i]}s")
    ax.grid(True)
    ax.legend()

plt.tight_layout()
plt.show()
