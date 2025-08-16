
async function simulateArbitrage() {
    try {
        console.log("--- Starting Arbitrage Simulation ---");

        const TOKEN_A_ADDRESS = "0xf1694388F19f5Adc11347B6457C95eA5d2DA121E";
        const TOKEN_B_ADDRESS = "0xd5649c595D092a81d7f26739b45E6B01CFD5352d";
        const DEX1_ADDRESS = "0x0C3749147CccA6096fE7Fd058F432f21e91a7882";
        const DEX2_ADDRESS = "0x7feFcA9413A2A630af854B370CE27892Ad959f34";
        let ARBITRAGE_ADDRESS = "0xf8C196cf5d624a8b65CE4839689B2439fa796e7A";

        // Basic Address Validation
        if (!TOKEN_A_ADDRESS || !TOKEN_B_ADDRESS || !DEX1_ADDRESS || !DEX2_ADDRESS || !ARBITRAGE_ADDRESS ||
            !web3.utils.isAddress(TOKEN_A_ADDRESS) || !web3.utils.isAddress(TOKEN_B_ADDRESS) ||
            !web3.utils.isAddress(DEX1_ADDRESS) || !web3.utils.isAddress(DEX2_ADDRESS) || !web3.utils.isAddress(ARBITRAGE_ADDRESS)) {
            throw new Error("Please paste valid, manually deployed TokenA, TokenB, Dex1, and Dex2 addresses into the script.");
        }

        const ARB_ATTEMPT_AMOUNT_A = web3.utils.toWei('10', 'ether'); // Amount of A to try
        const ARB_ATTEMPT_AMOUNT_B = web3.utils.toWei('10', 'ether'); // Amount of B to try

        console.log("Fetching ABIs...");
        const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
        const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Dex.json'));
        const arbitrageMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Arbitrage.json'));
        const lpMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/LPToken.json'));
        if (!tokenMetadata || !dexMetadata || !arbitrageMetadata || !lpMetadata) throw new Error("Artifacts not found. Compile Token.sol, Dex.sol, Arbitrage.sol");
        const tokenABI = tokenMetadata.abi;
        const dexABI = dexMetadata.abi;
        const lpTokenABI  = lpMetadata.abi;
        const arbitrageABI = arbitrageMetadata.abi;
        console.log("ABIs fetched.");

        console.log("Fetching accounts...");
        const accounts = await web3.eth.getAccounts();
        if (accounts.length < 3) throw new Error("Need at least 3 accounts (Deployer, LP Setup, Arbitrageur)");
        const deployer = accounts[0];
        const arbitrageurAccount = accounts[1];
        const lpAccount = accounts[2];
        console.log("Deployer (Token Owner):", deployer);
        console.log("Arbitrageur Account:", arbitrageurAccount);
        console.log("LP Setup Account:", lpAccount);

        console.log("Connecting to manually deployed contracts...");
        const tokenA = new web3.eth.Contract(tokenABI, TOKEN_A_ADDRESS);
        const tokenB = new web3.eth.Contract(tokenABI, TOKEN_B_ADDRESS);
        const dex1 = new web3.eth.Contract(dexABI, DEX1_ADDRESS);
        const dex2 = new web3.eth.Contract(dexABI, DEX2_ADDRESS);
        const lpTokenAddress = await dex1.methods.lpToken().call();
        const lpToken = new web3.eth.Contract(lpTokenABI, lpTokenAddress);
        const arbitrage = new web3.eth.Contract(arbitrageABI, ARBITRAGE_ADDRESS);
        console.log("Connected to TokenA:", TOKEN_A_ADDRESS);
        console.log("Connected to TokenB:", TOKEN_B_ADDRESS);
        console.log("Connected to Dex1:", DEX1_ADDRESS);
        console.log("Connected to Dex2:", DEX2_ADDRESS);
        console.log("Connected to Arbitrageur", ARBITRAGE_ADDRESS);


        async function setupLiquidity(dexInstance, dexAddr, lpAcc, liquidityRatio) {
            const approvalAmount = web3.utils.toWei('1000000', 'ether');
            const fundAmount = web3.utils.toWei('5000', 'ether'); // 5000 tokens given to lp account by deployer

            console.log(`Ensuring LP account ${lpAcc} is funded...`);
            await tokenA.methods.transfer(lpAcc, fundAmount).send({ from: deployer });
            await tokenB.methods.transfer(lpAcc, fundAmount).send({ from: deployer });

            console.log(`LP ${lpAcc} approving DEX at ${dexAddr}...`);
            await tokenA.methods.approve(dexAddr, approvalAmount).send({ from: lpAcc });
            await tokenB.methods.approve(dexAddr, approvalAmount).send({ from: lpAcc });

            console.log(`LP ${lpAcc} adding liquidity to ${dexAddr}...`);
            const liqA = web3.utils.toWei(liquidityRatio.A, 'ether');
            const liqB = web3.utils.toWei(liquidityRatio.B, 'ether');
            const lpBalA = web3.utils.toBN(await tokenA.methods.balanceOf(lpAcc).call());
            const lpBalB = web3.utils.toBN(await tokenB.methods.balanceOf(lpAcc).call());
            if (lpBalA.lt(web3.utils.toBN(liqA)) || lpBalB.lt(web3.utils.toBN(liqB))) {
                throw new Error(`LP Account ${lpAcc} has insufficient balance to add required liquidity.`);
            }
            await dexInstance.methods.addLiquidity(liqA, liqB).send({ from: lpAcc, gas: 500000 });   //adding basic liquidity as asked in function call
            console.log(`DEX @ ${dexAddr} Liquidity: ${liquidityRatio.A} A / ${liquidityRatio.B} B`);
        }

        async function setupArbitrageur(arbAcc, arbContractAddress) {
            const approvalAmount = web3.utils.toWei('1000000', 'ether');
            const fundAmount = web3.utils.toWei('100', 'ether');  // 100 tokens given to arbitrager by deployer of tokens

            console.log(`Funding Arbitrageur ${arbAcc}...`);
            await tokenA.methods.transfer(arbAcc, fundAmount).send({ from: deployer });
            await tokenB.methods.transfer(arbAcc, fundAmount).send({ from: deployer });

            console.log(`Arbitrageur ${arbAcc} approving Arbitrage contract ${arbContractAddress}...`);
            await tokenA.methods.approve(arbContractAddress, approvalAmount).send({ from: arbAcc });
            await tokenB.methods.approve(arbContractAddress, approvalAmount).send({ from: arbAcc });
        }

        async function getBalances(account) {
            const balA = await tokenA.methods.balanceOf(account).call();
            const balB = await tokenB.methods.balanceOf(account).call();
            return { A: web3.utils.toBN(balA), B: web3.utils.toBN(balB) };
        }

        async function readPrices() {
            const price1 = await dex1.methods.getPriceAInB().call();
            const price2 = await dex2.methods.getPriceAInB().call();
            console.log(`Current Prices (A in B * 1e18): Dex1=${price1}, Dex2=${price2}`);
            return { price1, price2 };
        }

        await setupArbitrageur(arbitrageurAccount, ARBITRAGE_ADDRESS);

        console.log("\n--- SCENARIO 1: Profitable Arbitrage Setup ---");
        await setupLiquidity(dex1, DEX1_ADDRESS, lpAccount, { A: '1000', B: '2000' }); // Dex1 Price ≈ 2 B/A
        await setupLiquidity(dex2, DEX2_ADDRESS, lpAccount, { A: '1000', B: '2000' }); // Dex2 Price ≈ 2 B/A
        await readPrices();

        console.log("\n--- Attempting Arbitrage (Scenario 1) ---");
        const balancesBefore1 = await getBalances(arbitrageurAccount);
        console.log(`Arbitrageur Balances Before: A=${web3.utils.fromWei(balancesBefore1.A)} B=${web3.utils.fromWei(balancesBefore1.B)}`);

        try {
            const txReceipt1 = await arbitrage.methods.executeArbitrage(ARB_ATTEMPT_AMOUNT_A, ARB_ATTEMPT_AMOUNT_B).send({ from: arbitrageurAccount, gas: 5000000 });
            console.log("Arbitrage Tx1 SUCCEEDED (Check Events). Gas used:", txReceipt1.gasUsed);
            const executed = txReceipt1.events.ArbitrageExecuted;
            if (executed) {
                if (Array.isArray(executed)) {
                    executed.forEach((event, idx) => {
                        const values = event.returnValues;
                        const cleaned = Object.fromEntries(
                            Object.entries(values).filter(([key]) => isNaN(Number(key)))
                        );
                        console.log(`>>> ArbitrageExecuted Event ${idx + 1}:`);
                        console.log(JSON.stringify(cleaned, null, 2));
                    });
                } else {
                    const values = executed.returnValues;
                    const cleaned = Object.fromEntries(
                        Object.entries(values).filter(([key]) => isNaN(Number(key)))
                    );
                    console.log(">>> ArbitrageExecuted Event:");
                    console.log(JSON.stringify(cleaned, null, 2));
                }
            }

            const skipped = txReceipt1.events.ArbitrageSkipped;
            if (skipped) {
                if (Array.isArray(skipped)) {
                    skipped.forEach((event, idx) => {
                        const values = event.returnValues;
                        const cleaned = Object.fromEntries(
                            Object.entries(values).filter(([key]) => isNaN(Number(key)))
                        );
                        console.log(`--- ArbitrageSkipped Event ${idx + 1}:`);
                        console.log(JSON.stringify(cleaned, null, 2));
                    });
                } else {
                    const values = skipped.returnValues;
                    const cleaned = Object.fromEntries(
                        Object.entries(values).filter(([key]) => isNaN(Number(key)))
                    );
                    console.log("--- ArbitrageSkipped Event:");
                    console.log(JSON.stringify(cleaned, null, 2));
                }
            }
        } catch (error) { console.error("Arbitrage Tx1 FAILED:", error.message); }

        const balancesAfter1 = await getBalances(arbitrageurAccount);
        console.log(`Arbitrageur Balances After: A=${web3.utils.fromWei(balancesAfter1.A)} B=${web3.utils.fromWei(balancesAfter1.B)}`);
        console.log(`Profit A (Wei): ${balancesAfter1.A.sub(balancesBefore1.A).toString()}`);
        console.log(`Profit B (Wei): ${balancesAfter1.B.sub(balancesBefore1.B).toString()}`);
        await readPrices();

        console.log("\n--- SCENARIO 2: Non-Profitable Arbitrage Setup ---");
        console.log("Performing balancing swap(s) to equalize prices...");
        
        const balancingSwapAmountB = web3.utils.toWei('100', 'ether');  // now the ratio is 2000 2100 in another  // Dex2 Price ≈ 2.1 B/A (A cheaper on Dex2)
        console.log(`LP performing balancing swap: ${web3.utils.fromWei(balancingSwapAmountB)} B for A on DEX2`);
        await dex2.methods.swap(balancingSwapAmountB, TOKEN_B_ADDRESS, lpAccount).send({ from: lpAccount, gas: 500000 });
        console.log("Balancing swap complete.");

        await readPrices();

        console.log("\n--- Attempting Arbitrage (Scenario 2) ---");
        const balancesBefore2 = await getBalances(arbitrageurAccount);
        console.log(`Arbitrageur Balances Before: A=${web3.utils.fromWei(balancesBefore2.A)} B=${web3.utils.fromWei(balancesBefore2.B)}`);

        try {
            const txReceipt2 = await arbitrage.methods.executeArbitrage(ARB_ATTEMPT_AMOUNT_A, ARB_ATTEMPT_AMOUNT_B).send({ from: arbitrageurAccount, gas: 1500000 });
            console.log("Arbitrage Tx2 SUCCEEDED (Check Events). Gas used:", txReceipt2.gasUsed);
            const executed = txReceipt2.events.ArbitrageExecuted;
            if (executed) {
                if (Array.isArray(executed)) {
                    executed.forEach((event, idx) => {
                        const values = event.returnValues;
                        const cleaned = Object.fromEntries(
                            Object.entries(values).filter(([key]) => isNaN(Number(key)))
                        );
                        console.log(`>>> ArbitrageExecuted Event ${idx + 1}:`);
                        console.log(JSON.stringify(cleaned, null, 2));
                    });
                } else {
                    const values = executed.returnValues;
                    const cleaned = Object.fromEntries(
                        Object.entries(values).filter(([key]) => isNaN(Number(key)))
                    );
                    console.log(">>> ArbitrageExecuted Event:");
                    console.log(JSON.stringify(cleaned, null, 2));
                }
            }

            const skipped = txReceipt2.events.ArbitrageSkipped;
            if (skipped) {
                if (Array.isArray(skipped)) {
                    skipped.forEach((event, idx) => {
                        const values = event.returnValues;
                        const cleaned = Object.fromEntries(
                            Object.entries(values).filter(([key]) => isNaN(Number(key)))
                        );
                        console.log(`--- ArbitrageSkipped Event ${idx + 1}:`);
                        console.log(JSON.stringify(cleaned, null, 2));
                    });
                } else {
                    const values = skipped.returnValues;
                    const cleaned = Object.fromEntries(
                        Object.entries(values).filter(([key]) => isNaN(Number(key)))
                    );
                    console.log("--- ArbitrageSkipped Event:");
                    console.log(JSON.stringify(cleaned, null, 2));
                }
            }
        } catch (error) { console.error("Arbitrage Tx2 FAILED:", error.message); }

        const balancesAfter2 = await getBalances(arbitrageurAccount);
        console.log(`Arbitrageur Balances After: A=${web3.utils.fromWei(balancesAfter2.A)} B=${web3.utils.fromWei(balancesAfter2.B)}`);
        console.log(`Profit A (Wei): ${balancesAfter2.A.sub(balancesBefore2.A).toString()}`);
        console.log(`Profit B (Wei): ${balancesAfter2.B.sub(balancesBefore2.B).toString()}`);
        await readPrices();

        console.log("\n--- Arbitrage Simulation Complete ---");

    } catch (error) {
        console.error("\n--- Simulation FAILED ---");
        console.error(error);
    }
}

// Run the simulation
simulateArbitrage();