import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# --- Load data using Pandas (from previous step) ---
try:
    df = pd.read_csv('main_data.csv')
    numeric_cols = ['ReserveA', 'ReserveB', 'SpotPriceA_In_B(Wei)',
                    'LP_1_Share(Wei)', 'LP_2_Share(Wei)', 'LP_3_Share(Wei)',
                    'LP_4_Share(Wei)', 'LP_5_Share(Wei)', 'Slippage(%)']
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    # --- Create Plots ---
    plt.style.use('seaborn-v0_8-darkgrid') # Use a nice style

    # 1. Reserves Plot
    plt.figure(figsize=(12, 6)) # Make the plot larger
    plt.plot(df['Tx'], df['ReserveA'], label='Reserve A (Wei)', marker='.')
    plt.plot(df['Tx'], df['ReserveB'], label='Reserve B (Wei)', marker='.')
    plt.xlabel("Transaction Number")
    plt.ylabel("Reserve Amount (Wei)")
    plt.title("DEX Reserves vs. Transaction Number")
    # Use ScalarFormatter for large numbers on y-axis
    plt.gca().yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
    plt.ticklabel_format(style='plain', axis='y') # Prevent scientific notation if possible
    plt.legend()
    plt.tight_layout() # Adjust layout
    plt.savefig("dex_reserves_plot.png") # Save the plot
    plt.show()

    # 2. Spot Price Plot
    plt.figure(figsize=(12, 6))
    plt.plot(df['Tx'], df['SpotPriceA_In_B(Wei)'], label='Spot Price (Token B per Token A * 1e18)', marker='.', color='green')
    plt.xlabel("Transaction Number")
    plt.ylabel("Spot Price (Scaled by 1e18)")
    plt.title("DEX Spot Price (A in B) vs. Transaction Number")
    plt.gca().yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
    plt.ticklabel_format(style='plain', axis='y')
    plt.legend()
    plt.tight_layout()
    plt.savefig("dex_spot_price_plot.png")
    plt.show()

    # 3. LP Token Distribution Plot
    plt.figure(figsize=(12, 6))
    for i in range(1, 6): # Assuming 5 LPs
        col_name = f'LP_{i}_Share(Wei)'
        if col_name in df.columns:
            plt.plot(df['Tx'], df[col_name], label=f'LP {i} Share (Wei)', marker='.', linestyle='--')
    plt.xlabel("Transaction Number")
    plt.ylabel("LP Token Share (Wei)")
    plt.title("LP Token Distribution vs. Transaction Number")
    plt.gca().yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
    plt.ticklabel_format(style='plain', axis='y')
    plt.legend()
    plt.tight_layout()
    plt.savefig("dex_lp_shares_plot.png")
    plt.show()

    # 4. Slippage Plot
    plt.figure(figsize=(12, 6))
    plt.plot(df['Tx'], df['Slippage(%)'], label='Slippage (%)', marker='x', linestyle=':', color='red')
    plt.xlabel("Transaction Number")
    plt.ylabel("Slippage (%)")
    plt.title("Swap Slippage vs. Transaction Number")
    plt.legend()
    plt.tight_layout()
    plt.savefig("dex_slippage_plot.png")
    plt.show()

except FileNotFoundError:
    print("Error: dex_simulation_data.csv not found.")
except Exception as e:
    print(f"An error occurred during plotting: {e}")