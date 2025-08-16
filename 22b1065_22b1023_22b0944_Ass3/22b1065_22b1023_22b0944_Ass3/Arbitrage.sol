// SPDX-License-Identifier: MIT

pragma solidity ^0.8.20;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol"; // To interact with TokenA/TokenB
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "@openzeppelin/contracts/access/Ownable.sol";
import "./Dex.sol";


contract Arbitrage is Ownable, ReentrancyGuard {

    Dex public immutable dex1;
    Dex public immutable dex2;
    IERC20 public immutable tokenA;
    IERC20 public immutable tokenB;

    // Minimum profit threshold (in wei of the token you start/end with)
    uint256 public constant MINIMUM_PROFIT_THRESHOLD = 50000000000000000; // Set very low for testing  that is 0.05 in 10 *(10^18)

    uint32 private constant FEE_NUMERATOR = 3;
    uint32 private constant FEE_DENOMINATOR = 1000;

    event ArbitrageExecuted(
        address indexed tokenStart,
        uint256 amountIn,
        address indexed dexBuyOn,
        address indexed dexSellOn,
        uint256 amountOutFinal,
        uint256 profit
    );
    event ArbitrageSkipped(
        address indexed tokenStart,
        string reason,
        int256 potentialProfit
    );

    constructor(address _dex1, address _dex2, address _tokenA, address _tokenB) Ownable(msg.sender) {
        require(_dex1 != address(0) && _dex2 != address(0) && _tokenA != address(0) && _tokenB != address(0), "Arbitrage: Zero address");
        require(_dex1 != _dex2, "Arbitrage: DEX addresses same");
        dex1 = Dex(_dex1);
        dex2 = Dex(_dex2);
        tokenA = IERC20(_tokenA);
        tokenB = IERC20(_tokenB);
    }

    function executeArbitrage(uint256 amountAIn, uint256 amountBIn) external onlyOwner nonReentrant {
        // Check A -> B -> A'
        int256 profitA = _calculateAndExecute(tokenA, tokenB, amountAIn);

        // Check B -> A -> B'
        int256 profitB = _calculateAndExecute(tokenB, tokenA, amountBIn);

        if (profitA <= int256(MINIMUM_PROFIT_THRESHOLD) && profitB <= int256(MINIMUM_PROFIT_THRESHOLD)) {
            IERC20 random  = tokenA;
            address rr  = address(random);
            emit ArbitrageSkipped(rr , "Profit below threshold in both directions", profitA > profitB ? profitA : profitB);
        }
    }

    function _calculateAndExecute(
        IERC20 _tokenStartInstance,
        IERC20 _tokenIntermediateInstance,
        uint256 _amountIn
    ) internal returns (int256 profit) {

        if (_amountIn == 0) return 0;
        address tokenStartAddr = address(_tokenStartInstance);
        address tokenIntermediateAddr = address(_tokenIntermediateInstance);
        (uint256 reserve1A, uint256 reserve1B) = dex1.getReserves();
        (uint256 reserve2A, uint256 reserve2B) = dex2.getReserves();

        if (reserve1A == 0 || reserve1B == 0 || reserve2A == 0 || reserve2B == 0) {
            emit ArbitrageSkipped(tokenStartAddr, "Liquidity missing", 0);
            return 0;
        }
        uint256 dex1ReserveStart = (tokenStartAddr == address(tokenA)) ? reserve1A : reserve1B;
        uint256 dex1ReserveIntermediate = (tokenStartAddr == address(tokenA)) ? reserve1B : reserve1A;
        uint256 dex2ReserveStart = (tokenStartAddr == address(tokenA)) ? reserve2A : reserve2B;
        uint256 dex2ReserveIntermediate = (tokenStartAddr == address(tokenA)) ? reserve2B : reserve2A;


        uint256 intermediateOut1 = getAmountOut(_amountIn, dex1ReserveStart, dex1ReserveIntermediate);
        uint256 intermediateOut2 = getAmountOut(_amountIn, dex2ReserveStart, dex2ReserveIntermediate);

        if (intermediateOut1 == 0 && intermediateOut2 == 0) {
            emit ArbitrageSkipped(tokenStartAddr,"Calculated swap output is zero for both DEXes", 0);
            return 0;
        }

        Dex dexBuyOn;
        Dex dexSellOn;
        uint256 expectedIntermediateAmount;

        if (intermediateOut1 > intermediateOut2) {
            dexBuyOn = dex1;
            dexSellOn = dex2;
            expectedIntermediateAmount = intermediateOut1;
        }
        else if (intermediateOut2 > intermediateOut1) {
            dexBuyOn = dex2;
            dexSellOn = dex1;
            expectedIntermediateAmount = intermediateOut2;
        }
        else {
            emit ArbitrageSkipped(tokenStartAddr,"Prices are equal", 0);
            return 0;
        }

        (uint256 sellDexReserveA, uint256 sellDexReserveB) = dexSellOn.getReserves();
        uint256 sellDexReserveIntermediate = (tokenIntermediateAddr == address(tokenA)) ? sellDexReserveA : sellDexReserveB;
        uint256 sellDexReserveFinal = (tokenIntermediateAddr == address(tokenA)) ? sellDexReserveB : sellDexReserveA;

        uint256 expectedFinalAmountOut = getAmountOut(expectedIntermediateAmount, sellDexReserveIntermediate, sellDexReserveFinal);

        profit = int256(expectedFinalAmountOut) - int256(_amountIn);
        if (profit <= int256(MINIMUM_PROFIT_THRESHOLD)) {
            emit ArbitrageSkipped(tokenStartAddr,"Profit below threshold", profit);
            return profit; // Don't execute
        }
        require(_tokenStartInstance.transferFrom(owner(), address(this), _amountIn), "Arbitrage: Failed pull initial funds");

        require(_tokenStartInstance.approve(address(dexBuyOn), _amountIn), "Arbitrage: Approve dexBuyOn failed");
        require(_tokenIntermediateInstance.approve(address(dexSellOn), expectedIntermediateAmount), "Arbitrage: Approve dexSellOn failed");

        uint actualIntermediateAmount = dexBuyOn.swap(_amountIn, tokenStartAddr, address(this));
        require(actualIntermediateAmount > 0, "Arbitrage: First swap returned zero"); // Sanity check

        uint actualFinalAmountOut = dexSellOn.swap(actualIntermediateAmount, tokenIntermediateAddr, address(this));
        require(actualFinalAmountOut > 0, "Arbitrage: Second swap returned zero");

        uint finalBalance = _tokenStartInstance.balanceOf(address(this));
        require(finalBalance >= actualFinalAmountOut, "Arbitrage: Final balance calculation error");
        int256 actualProfit = int256(finalBalance) - int256(_amountIn); // Profit = final balance - initial pull

        if (actualProfit <= 0) {
            if(finalBalance > 0) require(_tokenStartInstance.transfer(owner(), finalBalance), "Arbitrage: Failed return non-profit funds");
            emit ArbitrageSkipped(tokenStartAddr, "Actual execution resulted in no profit", actualProfit);
            _tokenStartInstance.approve(address(dexBuyOn), 0);
            _tokenIntermediateInstance.approve(address(dexSellOn), 0);
            return actualProfit;
        }

        require(_tokenStartInstance.transfer(owner(), finalBalance), "Arbitrage: Failed return funds");
        require(_tokenStartInstance.approve(address(dexBuyOn), 0), "Arbitrage: Reset1 failed");
        require(_tokenIntermediateInstance.approve(address(dexSellOn), 0), "Arbitrage: Reset2 failed");

        emit ArbitrageExecuted(tokenStartAddr, _amountIn, address(dexBuyOn), address(dexSellOn), actualFinalAmountOut, uint256(actualProfit));
        return actualProfit;
    }

    function getAmountOut(uint256 amountIn, uint256 reserveIn, uint256 reserveOut)
        internal pure returns (uint256 amountOut)
    {
        if (amountIn == 0 || reserveIn == 0 || reserveOut == 0) return 0;
        uint amountInWithFee = (amountIn * (FEE_DENOMINATOR - FEE_NUMERATOR)) / FEE_DENOMINATOR;
        if (amountInWithFee == 0) return 0;
        uint denominator = reserveIn + amountInWithFee;
        if (denominator == 0) return 0;
        amountOut = (reserveOut * amountInWithFee) / denominator;
        return amountOut;
    }
}