function getRandomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function spotPrice(reserveA, reserveB){
    const price = reserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(reserveA);
    return price
}

async function simulateDex() {
    console.log("--- Starting DEX Simulation ---");

    const N_TRANSACTIONS = 75; // Choose N between 50 and 100dex
    const NUM_LPS = 5;
    const NUM_TRADERS = 8;
    const TOTAL_USERS = NUM_LPS + NUM_TRADERS;

    const TOKEN_A_ADDRESS = "0x7199069245A55f4092037BDDf22A18615e8FCa10";
    const TOKEN_B_ADDRESS = "0x5D496b0Dad76628df71Ba7D3BAcdDB9fD2358DC1";
    const DEX_ADDRESS = "0xCFb5D91756E1E119cb791CBbC9DB719EF1da656C";

    if (!TOKEN_A_ADDRESS || !TOKEN_B_ADDRESS || !DEX_ADDRESS) {
        throw new Error("Please replace placeholder addresses with your deployed contract addresses.");
    }
    if (!web3.utils.isAddress(TOKEN_A_ADDRESS) || !web3.utils.isAddress(TOKEN_B_ADDRESS) || !web3.utils.isAddress(DEX_ADDRESS)) {
        throw new Error("One or more provided addresses are invalid.");
    }

    console.log("Fetching ABIs...");
    const tokenMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Token.json'));
    const dexMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/Dex.json'));
    const lpMetadata = JSON.parse(await remix.call('fileManager', 'getFile', 'browser/contracts/artifacts/LPToken.json'));

    if (!tokenMetadata || !dexMetadata || !lpMetadata) {
        throw new Error("Could not find contract artifacts. Please compile TokenA, TokenB, and Dex first.");
    }

    const tokenABI = tokenMetadata.abi;
    const dexABI = dexMetadata.abi;
    const lpTokenABI  = lpMetadata.abi;
    console.log("ABIs fetched.");

    console.log("Fetching accounts...");
    const accounts = await web3.eth.getAccounts();
    if (accounts.length < TOTAL_USERS + 1) {
        throw new Error(`Need at least ${TOTAL_USERS + 1} accounts in Remix, found ${accounts.length}`);
    }

    const deployer = accounts[0];
    const lps = accounts.slice(1, 1 + NUM_LPS);
    const traders = accounts.slice(1 + NUM_LPS, 1 + TOTAL_USERS);
    console.log("Deployer:", deployer);
    console.log("LPs:", lps);
    console.log("Traders:", traders);

    console.log("Connecting to contracts...");
    const tokenA = new web3.eth.Contract(tokenABI, TOKEN_A_ADDRESS);
    const tokenB = new web3.eth.Contract(tokenABI, TOKEN_B_ADDRESS);
    const dex = new web3.eth.Contract(dexABI, DEX_ADDRESS);
    const lpTokenAddress = await dex.methods.lpToken().call();
    const lpToken = new web3.eth.Contract(lpTokenABI, lpTokenAddress);
    console.log("Connected to TokenA:", TOKEN_A_ADDRESS);
    console.log("Connected to TokenB:", TOKEN_B_ADDRESS);
    console.log("Connected to Dex:", DEX_ADDRESS);

    console.log("\n--- Initial Setup Phase ---");
    const initialTokenAmount = web3.utils.toWei('10000', 'ether');
    const approvalAmount = web3.utils.toWei('1000000000', 'ether');

    console.log("Funding LPs...");
    for (const lp of lps) {
        await tokenA.methods.transfer(lp, initialTokenAmount).send({ from: deployer });
        await tokenB.methods.transfer(lp, initialTokenAmount).send({ from: deployer });
        await tokenA.methods.approve(DEX_ADDRESS, approvalAmount).send({ from: lp });
        await tokenB.methods.approve(DEX_ADDRESS, approvalAmount).send({ from: lp });
    }
    console.log("LPs funded and approved Dex.");

    console.log("Funding Traders...");
    for (const trader of traders) {
        await tokenA.methods.transfer(trader, initialTokenAmount).send({ from: deployer });
        await tokenB.methods.transfer(trader, initialTokenAmount).send({ from: deployer });
        await tokenA.methods.approve(DEX_ADDRESS, approvalAmount).send({ from: trader });
        await tokenB.methods.approve(DEX_ADDRESS, approvalAmount).send({ from: trader });
    }
    console.log("Traders funded and approved Dex.");

    console.log("Adding initial liquidity...");
    const initialLiquidityA = web3.utils.toWei('5000', 'ether');
    const initialLiquidityB = web3.utils.toWei('5000', 'ether');
    await dex.methods.addLiquidity(initialLiquidityA, initialLiquidityB).send({ from: lps[0] });
    console.log(`Initial liquidity added by ${lps[0]}.`);

    console.log("\n--- Simulation Loop Starting ---");
    let timestamps = [];
    let reserveAHistory = [];
    let reserveBHistory = [];
    let spotPriceHistory = [];
    let lpShares = {};
    lps.forEach(lp => lpShares[lp] = []);
    let totalSwapVolumeA = web3.utils.toBN(0);
    let totalSwapVolumeB = web3.utils.toBN(0);
    let feeAAccumulated = web3.utils.toBN(0);
    let feeBAccumulated = web3.utils.toBN(0);
    let slippageHistory = [];

    for (let i = 0; i < N_TRANSACTIONS; i++) {
        console.log(`\n--- Transaction ${i + 1}/${N_TRANSACTIONS} ---`);
        timestamps.push(i + 1); // Use index as 'time'

        // Randomly choose action type (e.g., 70% swap, 15% add, 15% remove)
        const actionRoll = getRandomInt(1, 100);
        let currentAction = '';

        const reservesBefore = await dex.methods.getReserves().call();
        const reserveABefore = web3.utils.toBN(reservesBefore._reserveA);
        const reserveBBefore = web3.utils.toBN(reservesBefore._reserveB);

        if (reserveABefore.isZero() || reserveBBefore.isZero()) {
            console.log("Pool empty, skipping action.");
            reserveAHistory.push(reserveABefore.toString());
            reserveBHistory.push(reserveBBefore.toString());
            spotPriceHistory.push('0'); // Or NaN
            for(const lp of lps) {
                lpShares[lp].push('0');
            }
            slippageHistory.push(0);
            continue; // Skip to next iteration
        }


        try {
            if (actionRoll <= 30) {
                currentAction = 'Add Liquidity';
                const lp = lps[getRandomInt(0, NUM_LPS - 1)];
                console.log(`Action: ${currentAction} by ${lp}`);

                const lpBalanceA = web3.utils.toBN(await tokenA.methods.balanceOf(lp).call());
                const lpBalanceB = web3.utils.toBN(await tokenB.methods.balanceOf(lp).call());

                if (lpBalanceA.isZero() || lpBalanceB.isZero()) {
                    console.log("LP has zero balance of A or B, skipping add.");
                    slippageHistory.push(NaN); 

                    console.log("Collecting metrics...");
                    const currentReserves = await dex.methods.getReserves().call();
                    const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
                    const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
                    reserveAHistory.push(currentReserveA.toString());
                    reserveBHistory.push(currentReserveB.toString());

                    // Spot Price (Price of A in terms of B) = reserveB / reserveA
                    if (!currentReserveA.isZero()) {
                        const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
                        spotPriceHistory.push(price.toString());
                    } else {
                        spotPriceHistory.push('0'); // Or NaN
                    }

                    // LP Token Distribution
                    for(const lp of lps) {
                        const lpBal = await lpToken.methods.balanceOf(lp).call();
                        lpShares[lp].push(lpBal.toString());
                    }
                    console.log("Metrics collected.");
                    continue;
                }

                const percentA = web3.utils.toBN(getRandomInt(1, 10)); // 1-10%
                let amountA_desired = lpBalanceA.mul(percentA).div(web3.utils.toBN(100));
                let amountB_required = amountA_desired.mul(reserveBBefore).div(reserveABefore);

                if (amountB_required.gt(lpBalanceB)) {
                    console.log("Scaling down liquidity add based on TokenB balance.");
                    amountB_required = lpBalanceB.mul(percentA).div(web3.utils.toBN(100));
                    amountA_desired = amountB_required.mul(reserveABefore).div(reserveBBefore);
                }

                if (amountA_desired.isZero() || amountB_required.isZero()) {
                    console.log("Calculated add amounts are zero, skipping.");
                    slippageHistory.push(NaN); 
                    console.log("Collecting metrics...");
                    const currentReserves = await dex.methods.getReserves().call();
                    const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
                    const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
                    reserveAHistory.push(currentReserveA.toString());
                    reserveBHistory.push(currentReserveB.toString());

                    // Spot Price (Price of A in terms of B) = reserveB / reserveA
                    if (!currentReserveA.isZero()) {
                        const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
                        spotPriceHistory.push(price.toString());
                    } else {
                        spotPriceHistory.push('0'); // Or NaN
                    }

                    // LP Token Distribution
                    for(const lp of lps) {
                        const lpBal = await lpToken.methods.balanceOf(lp).call();
                        lpShares[lp].push(lpBal.toString());
                    }
                    console.log("Metrics collected.");
                    continue;
                }

                console.log(`Attempting to add A: ${web3.utils.fromWei(amountA_desired)} B: ${web3.utils.fromWei(amountB_required)}`);
                slippageHistory.push(NaN); 
                await dex.methods.addLiquidity(amountA_desired, amountB_required).send({ from: lp, gas: 3000000 }); // Add gas limit

            } else if (actionRoll <= 60) { // Remove Liquidity (15%)
                currentAction = 'Remove Liquidity';
                const lp = lps[getRandomInt(0, NUM_LPS - 1)];
                console.log(`Action: ${currentAction} by ${lp}`);

                const lpTokenBalance = web3.utils.toBN(await lpToken.methods.balanceOf(lp).call());
                if (lpTokenBalance.isZero()) {
                    console.log("LP has no LP tokens, skipping remove.");
                    slippageHistory.push(NaN); 
                    console.log("Collecting metrics...");
                    const currentReserves = await dex.methods.getReserves().call();
                    const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
                    const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
                    reserveAHistory.push(currentReserveA.toString());
                    reserveBHistory.push(currentReserveB.toString());

                    // Spot Price (Price of A in terms of B) = reserveB / reserveA
                    if (!currentReserveA.isZero()) {
                        const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
                        spotPriceHistory.push(price.toString());
                    } else {
                        spotPriceHistory.push('0'); // Or NaN
                    }

                    // LP Token Distribution
                    for(const lp of lps) {
                        const lpBal = await lpToken.methods.balanceOf(lp).call();
                        lpShares[lp].push(lpBal.toString());
                    }
                    console.log("Metrics collected.");
                    continue;
                }

                const percentLP = web3.utils.toBN(getRandomInt(1, 50));
                let amountToRemove = lpTokenBalance.mul(percentLP).div(web3.utils.toBN(100));

                if (amountToRemove.isZero()) {
                    amountToRemove = web3.utils.toBN(1);
                }
                if (amountToRemove.gt(lpTokenBalance)) { // Safety check
                    amountToRemove = lpTokenBalance;
                }

                console.log(`Attempting to remove ${web3.utils.fromWei(amountToRemove)} LP tokens`);
                slippageHistory.push(NaN); 
                await dex.methods.removeLiquidity(amountToRemove).send({ from: lp, gas: 3000000 });

            } else {
                currentAction = 'Swap';
                const trader = traders[getRandomInt(0, NUM_TRADERS - 1)];
                const swapAtoB = Math.random() < 0.5;
                const tokenIn = swapAtoB ? tokenA : tokenB;
                const tokenInAddress = swapAtoB ? TOKEN_A_ADDRESS : TOKEN_B_ADDRESS;
                const tokenOutAddress = swapAtoB ? TOKEN_B_ADDRESS : TOKEN_A_ADDRESS;
                const reserveIn = swapAtoB ? reserveABefore : reserveBBefore;
                const reserveOut = swapAtoB ? reserveBBefore : reserveABefore;

                console.log(`Action: ${currentAction} by ${trader} (${swapAtoB ? 'A->B' : 'B->A'})`);

                const traderBalanceIn = web3.utils.toBN(await tokenIn.methods.balanceOf(trader).call());
                if (traderBalanceIn.isZero()) {
                    console.log("Trader has no input token, skipping swap.");
                    slippageHistory.push(NaN); 

                    console.log("Collecting metrics...");
                    const currentReserves = await dex.methods.getReserves().call();
                    const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
                    const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
                    reserveAHistory.push(currentReserveA.toString());
                    reserveBHistory.push(currentReserveB.toString());

                    // Spot Price (Price of A in terms of B) = reserveB / reserveA
                    if (!currentReserveA.isZero()) {
                        const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
                        spotPriceHistory.push(price.toString());
                    } else {
                        spotPriceHistory.push('0'); // Or NaN
                    }

                    // LP Token Distribution
                    for(const lp of lps) {
                        const lpBal = await lpToken.methods.balanceOf(lp).call();
                        lpShares[lp].push(lpBal.toString());
                    }
                    console.log("Metrics collected.");
                    continue;
                }

                const maxFromReserve = reserveIn.div(web3.utils.toBN(10));
                const maxAmountIn = traderBalanceIn.lt(maxFromReserve) ? traderBalanceIn : maxFromReserve;

                if (maxAmountIn.isZero()) {
                    console.log("Calculated max swap amount is zero, skipping.");
                    slippageHistory.push(NaN); 

                    console.log("Collecting metrics...");
                    const currentReserves = await dex.methods.getReserves().call();
                    const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
                    const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
                    reserveAHistory.push(currentReserveA.toString());
                    reserveBHistory.push(currentReserveB.toString());

                    // Spot Price (Price of A in terms of B) = reserveB / reserveA
                    if (!currentReserveA.isZero()) {
                        const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
                        spotPriceHistory.push(price.toString());
                    } else {
                        spotPriceHistory.push('0'); // Or NaN
                    }

                    // LP Token Distribution
                    for(const lp of lps) {
                        const lpBal = await lpToken.methods.balanceOf(lp).call();
                        lpShares[lp].push(lpBal.toString());
                    }
                    console.log("Metrics collected.");
                    continue;
                }

                const randomFraction = web3.utils.toBN(getRandomInt(1, 100)); // e.g. 1-100% of max
                let amountIn = maxAmountIn.mul(randomFraction).div(web3.utils.toBN(100));
                if (amountIn.isZero()) amountIn = web3.utils.toBN(1); // Minimum 1 wei

                console.log(`Attempting to swap ${web3.utils.fromWei(amountIn)} ${swapAtoB ? 'A' : 'B'}`);

                const amountInWithFee = amountIn.mul(web3.utils.toBN(997)).div(web3.utils.toBN(1000)); // 0.3% fee
                const expectedNumerator = reserveOut.mul(amountInWithFee);
                const expectedDenominator = reserveIn.add(amountInWithFee);
                const expectedAmountOut = expectedNumerator.div(expectedDenominator);

                // Execute Swap
                const receipt = await dex.methods.swap(amountIn, tokenInAddress, trader).send({ from: trader, gas: 3000000 }); // Add gas limit

                // Get actual amount out from event logs (more reliable than return value in scripts)
                let actualAmountOut = web3.utils.toBN(0);
                if(receipt.events.Swapped) {
                    // Handle single or array of events
                    const swapEvent = Array.isArray(receipt.events.Swapped) ? receipt.events.Swapped[0] : receipt.events.Swapped;
                    if (swapEvent && swapEvent.returnValues.tokenOut.toLowerCase() === tokenOutAddress.toLowerCase()) {
                        actualAmountOut = web3.utils.toBN(swapEvent.returnValues.amountOut);
                    }
                }
                if (actualAmountOut.isZero()){
                    console.warn("Could not reliably get actualAmountOut from swap event.");
                }


                // Update Swap Volume
                if (swapAtoB) {
                    totalSwapVolumeA = totalSwapVolumeA.add(amountIn);
                    totalSwapVolumeB = totalSwapVolumeB.add(actualAmountOut); // Use actual out
                    feeAAccumulated = feeAAccumulated.add(amountIn.mul(web3.utils.toBN(3)).div(web3.utils.toBN(1000))); // Fee approx
                } else {
                    totalSwapVolumeB = totalSwapVolumeB.add(amountIn);
                    totalSwapVolumeA = totalSwapVolumeA.add(actualAmountOut); // Use actual out
                    feeBAccumulated = feeBAccumulated.add(amountIn.mul(web3.utils.toBN(3)).div(web3.utils.toBN(1000))); // Fee approx
                }

                if (!amountIn.isZero() && !reserveIn.isZero() && !expectedAmountOut.isZero()) {
                    const scale = web3.utils.toBN(10).pow(web3.utils.toBN(18)); // Scale factor for precision

                    const actualRateScaled = actualAmountOut.mul(scale).div(amountIn);
                    const expectedRateScaled = reserveOut.mul(scale).div(reserveIn);

                    let slippage = 0;
                    if (!expectedRateScaled.isZero()) {
                        // slippage = ( (actualRateScaled / expectedRateScaled) - 1 ) * 100
                        // Calculate (actualRateScaled * scale / expectedRateScaled) to maintain precision
                        const ratioScaled = actualRateScaled.mul(scale).div(expectedRateScaled);
                        // Subtract scale (which represents 1*scale)
                        const diff = ratioScaled.sub(scale);
                        slippage = diff.mul(web3.utils.toBN(100)).div(scale); // Result is scaled by 100
                    }
                    console.log(`Slippage: ${slippage.toNumber() / 100}%`); // Display percentage
                    slippageHistory.push(slippage.toNumber() / 100); // Store percentage
                } else {
                    slippageHistory.push(0); // Or NaN if calculation not possible
                }
            }
        } catch (error) {
            console.error(`Transaction ${i + 1} (${currentAction}) failed:`, error.message);
            slippageHistory.push(NaN); // Push NaN if action failed before slippage calc
        }

        // --- Collect Metrics (After each transaction) ---
        console.log("Collecting metrics...");
        const currentReserves = await dex.methods.getReserves().call();
        const currentReserveA = web3.utils.toBN(currentReserves._reserveA);
        const currentReserveB = web3.utils.toBN(currentReserves._reserveB);
        reserveAHistory.push(currentReserveA.toString());
        reserveBHistory.push(currentReserveB.toString());

        // Spot Price (Price of A in terms of B) = reserveB / reserveA
        if (!currentReserveA.isZero()) {
            const price = currentReserveB.mul(web3.utils.toBN(10).pow(web3.utils.toBN(18))).div(currentReserveA);
            spotPriceHistory.push(price.toString());
        } else {
            spotPriceHistory.push('0'); // Or NaN
        }

        // LP Token Distribution
        for(const lp of lps) {
            const lpBal = await lpToken.methods.balanceOf(lp).call();
            lpShares[lp].push(lpBal.toString());
        }
        console.log("Metrics collected.");

    } // --- End of Simulation Loop ---


    // --- Display Results ---
    console.log("\n--- Simulation Finished ---");
    console.log("Total Transactions:", N_TRANSACTIONS);

    // Format data for easy plotting (e.g., CSV like)
    console.log("\n--- Results (Copy/Paste for Plotting) ---");

    // Header
    let header = "Tx";
    header += ",ReserveA,ReserveB,SpotPriceA_In_B(Wei)";
    lps.forEach((lp, idx) => header += `,LP_${idx+1}_Share(Wei)`);
    header += ",Slippage(%)";
    console.log(header);

    // Data Rows
    for (let i = 0; i < N_TRANSACTIONS; i++) {
        let row = timestamps[i];
        row += `,${reserveAHistory[i] || 'N/A'},${reserveBHistory[i] || 'N/A'},${spotPriceHistory[i] || 'N/A'}`;
        lps.forEach(lp => row += `,${lpShares[lp][i] || 'N/A'}`);
        row += `,${slippageHistory[i] === undefined || isNaN(slippageHistory[i]) ? 'N/A' : slippageHistory[i].toFixed(4)}`; // Format slippage
        console.log(row);
    }

        // Other aggregate metrics
    console.log("\n--- Aggregate Metrics ---");
    console.log("Total Swap Volume A (Wei):", totalSwapVolumeA.toString());
    console.log("Total Swap Volume B (Wei):", totalSwapVolumeB.toString());
    console.log("Accumulated Fees A (Approx Wei):", feeAAccumulated.toString());
    console.log("Accumulated Fees B (Approx Wei):", feeBAccumulated.toString());
    const finalReserveA = reserveAHistory[reserveAHistory.length - 1];
    const finalReserveB = reserveBHistory[reserveBHistory.length - 1];
    console.log("Final Reserve A (Wei):", finalReserveA);
    console.log("Final Reserve B (Wei):", finalReserveB);
}

simulateDex();