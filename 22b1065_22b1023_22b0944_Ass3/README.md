# Running the Ballot Contract on Remix IDE

## Prerequisites
1. Ensure you have a web3-compatible browser (Chrome/Firefox).
2. Access the [Remix IDE](https://remix.ethereum.org/).

---

## Steps to Compile and Deploy the Contracts

### 1. Load the `Token.sol`, `LPToken.sol`, `Dex.sol`,`Arbitrage.sol` Contract in Remix
- Open Remix IDE.
- In the **File Explorer**, upload or create a new file named these files.
- Copy and paste the contents of these files directly into created files.

### 2. Compile the Contract
- Navigate to the **Solidity Compiler** tab (on the left sidebar).
- Ensure the compiler version matches the Solidity version specified in .sol files.
- To compile go under Advanced Configurations:
      - click on optimization check box (Default value is 200)
      - change in configuration file
      - ensure the checkbox is selected then click on change button
      - ensure the configuration file is like this
      {
        ....
        "settings": {
          ....
          },
          "viaIR": true,
          .....
      }
- **Compile .sol files**.
- If successful, you will see a green checkmark.

### 3. Deploy the Contract
- Go to the **Deploy & Run Transactions** tab.
- Select an environment:
  - **Remix VM** (for local testing) under Shanghai
- Select account[0] for Token.sol and Dex.sol
- Select account[1] for arbitrage.sol
- Select contract from Contract dropdown menu
- Click **Deploy**.
- The contract should be deployed, and its address will appear in the "Deployed Contracts" section.

- **for deploying Token.sol**
     - Click on same configuration as discussed above two times
     - Two Tokens will be deployed under "Deployed Contracts" section
- **for deploying Dex.sol**
     - Here, one have to fill Token_A and Token_B addresses to deploy
     - The addresses are same as in "Deployed Contracts" section
     - Based on the type of experiments: do it two times for Dex1 and Dex2 or one for Dex
- **for deploying Arbitrage.sol** (if doing that experiment)
     - Here , just fill all the deployed addresses accordingly
     - But change the account for this as this will be owned by account[1]
---

## Running `simulate_arbitrage.js` or `simulate_DEX.js` to Interact with the Contract

### 1. Load the Script in Remix
- Open the **File Explorer** and upload/create `simulate_arbitrage.js` and `simulate_DEX.js`.
- Copy and paste the contents of these files in the .js files created.

### 2. Connect to the Deployed Contract
- In `simulate_arbitrage.js`, replace corresponding `Address` with the actual contract address deployed in Remix. You can find the address of the deployed contract in the console.
- In `simulate_DEX.js`, replace corresponding `Address` with the actual contract address deployed in Remix. You can find the address of the deployed contract in the console.

### 3. Execute the Script
- Open the **Terminal** in Remix.
- Press the play button on the top toolbar after opening the javascript file to run it.
- Observe the logs showing contract interactions, including proposal listings, voting, and results.

### 4. To generate graphs in simulate_DEX.js
- copy the statistics portion from the **Terminal** at last in Testing/main_data.csv 
- run the generate_table.py as `python generate_table.py`
- Other data one can keep in transaction main data in Testing/Transactions_data.txt
- for simulate_arbitrage.js just paste the data in arbitrage_data.txt and analyze.

---


