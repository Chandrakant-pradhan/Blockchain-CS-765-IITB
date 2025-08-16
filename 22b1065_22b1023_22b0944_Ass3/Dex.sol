// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/utils/math/Math.sol";
import "@openzeppelin/contracts/security/ReentrancyGuard.sol";
import "./LPToken.sol";

contract Dex is ReentrancyGuard {
    IERC20 public immutable tokenA;
    IERC20 public immutable tokenB;
    LPToken public immutable lpToken;

    uint256 public reserveA;
    uint256 public reserveB;
    uint public constant MINIMUM_LIQUIDITY = 10**3; // 1000 wei

    uint32 private constant FEE_NUMERATOR = 3;
    uint32 private constant FEE_DENOMINATOR = 1000;

    event Swapped(
        address indexed sender,
        uint amountIn,
        address indexed tokenIn,
        uint amountOut,
        address indexed tokenOut,
        address to
    );

    event Sync(uint reserveA, uint reserveB);
    event LiquidityAdded(address indexed provider, uint256 amountA, uint256 amountB, uint256 lpTokensMinted);
    event LiquidityRemoved(address indexed provider, uint256 amountA, uint256 amountB, uint256 lpTokensBurned);
    event RewardCollected(address indexed provider, uint256 rewardA, uint256 rewardB);

    constructor(address _tokenA, address _tokenB) {
        require(_tokenA != address(0) && _tokenB != address(0), "DEX: Zero address");
        require(_tokenA != _tokenB, "DEX: Identical token addresses");
        tokenA = IERC20(_tokenA);
        tokenB = IERC20(_tokenB);
        lpToken = new LPToken();
        require(lpToken.owner() == address(this), "DEX: Not owner of LPToken");
    }

    function _update(uint _reserveA, uint _reserveB) private {
        reserveA = _reserveA;
        reserveB = _reserveB;
        emit Sync(_reserveA, _reserveB);
    }

    function addLiquidity(uint256 amountADesired, uint256 amountBDesired)
        external
        nonReentrant
        returns (uint256 amountA, uint256 amountB, uint256 liquidityMinted)
    {
        require(amountADesired > 0 && amountBDesired > 0, "Dex: Insufficient amounts");

        uint256 _reserveA = reserveA;
        uint256 _reserveB = reserveB;

        if (_reserveA == 0 && _reserveB == 0) {
            amountA = amountADesired;
            amountB = amountBDesired;
        } else {
            uint amountBOptimal = (amountADesired * _reserveB) / _reserveA;
            if (amountBOptimal <= amountBDesired) {
                require(amountBOptimal > 0, "Dex: amountBOptimal is zero");
                amountA = amountADesired;
                amountB = amountBOptimal;
            } else {
                uint amountAOptimal = (amountBDesired * _reserveA) / _reserveB;
                require(amountAOptimal > 0, "Dex: amountAOptimal is zero");
                require(amountAOptimal <= amountADesired, "Dex: Insufficient A for desired B ratio");
                amountA = amountAOptimal;
                amountB = amountBDesired;
            }
        }

        require(tokenA.transferFrom(msg.sender, address(this), amountA), "Dex: TokenA transferFrom failed");
        require(tokenB.transferFrom(msg.sender, address(this), amountB), "Dex: TokenB transferFrom failed");

        uint256 _totalSupply = lpToken.totalSupply();
        if (_totalSupply == 0) {
            uint256 sqrtOutput = Math.sqrt(amountA * amountB);
            require(sqrtOutput > MINIMUM_LIQUIDITY, "Dex: Initial liquidity too small");
            lpToken.mint(msg.sender, sqrtOutput - MINIMUM_LIQUIDITY);
            // lpToken.mint(address(0), MINIMUM_LIQUIDITY); // Lock the minimum liquidity
            liquidityMinted = sqrtOutput - MINIMUM_LIQUIDITY;
        } else {
            uint256 termA = (amountA * _totalSupply) / _reserveA;
            uint256 termB = (amountB * _totalSupply) / _reserveB;
            liquidityMinted = Math.min(termA, termB);
            require(liquidityMinted > 0, "Dex: Insufficient liquidity minted");
            lpToken.mint(msg.sender, liquidityMinted);
        }

        _update(reserveA + amountA, reserveB + amountB);
        emit LiquidityAdded(msg.sender, amountA, amountB, liquidityMinted);
        return (amountA, amountB, liquidityMinted);
    }

    function removeLiquidity(uint256 liquidityToBurn)
        external
        nonReentrant
        returns (uint256 amountA, uint256 amountB)
    {
        require(liquidityToBurn > 0, "Dex: Amount must be positive");
        uint256 lpBalance = lpToken.balanceOf(msg.sender);
        require(lpBalance >= liquidityToBurn, "Dex: Insufficient LP balance");

        uint256 _totalSupply = lpToken.totalSupply();
        uint256 _reserveA = reserveA;
        uint256 _reserveB = reserveB;

        amountA = (liquidityToBurn * _reserveA) / _totalSupply;
        amountB = (liquidityToBurn * _reserveB) / _totalSupply;
        require(amountA > 0 && amountB > 0, "Dex: Insufficient liquidity burned");

        lpToken.burnFrom(msg.sender, liquidityToBurn);
        _update(_reserveA - amountA, _reserveB - amountB);

        // Claim rewards from LPToken contract
        (uint256 rewardA, uint256 rewardB) = lpToken.claimRewards(msg.sender, address(tokenA), address(tokenB));
        require(tokenA.transfer(msg.sender, amountA + rewardA), "Dex: TokenA transfer failed");
        require(tokenB.transfer(msg.sender, amountB + rewardB), "Dex: TokenB transfer failed");

        emit LiquidityRemoved(msg.sender, amountA, amountB, liquidityToBurn);
        emit RewardCollected(msg.sender, rewardA, rewardB);
        return (amountA + rewardA, amountB + rewardB);
    }

    function swap(uint256 amountIn, address tokenIn, address to)
        external
        nonReentrant
        returns (uint256 amountOut)
    {
        require(to != address(0), "Dex: INVALID_RECIPIENT");
        require(amountIn > 0, "Dex: INVALID_INPUT_AMOUNT");
        require(tokenIn == address(tokenA) || tokenIn == address(tokenB), "Dex: INVALID_INPUT_TOKEN");

        uint256 _reserveA = reserveA;
        uint256 _reserveB = reserveB;
        require(_reserveA > 0 && _reserveB > 0, "Dex: INSUFFICIENT_LIQUIDITY");

        IERC20 tokenOut;
        uint256 reserveIn;
        uint256 reserveOut;

        if (tokenIn == address(tokenA)) {
            tokenOut = tokenB;
            reserveIn = _reserveA;
            reserveOut = _reserveB;
        } else {
            tokenOut = tokenA;
            reserveIn = _reserveB;
            reserveOut = _reserveA;
        }



        uint256 amountFee = (amountIn * FEE_NUMERATOR) / FEE_DENOMINATOR;
        uint256 amountInWithFee = amountIn - amountFee;
        require(amountInWithFee > 0, "Dex: Input amount too small after fee");

        require((IERC20(tokenIn)).transferFrom(msg.sender, address(this), amountIn - amountFee), "Transfer failed to pool");
        require((IERC20(tokenIn)).transferFrom(msg.sender, address(lpToken), amountFee), "Transfer failed to LP");
        lpToken.distributeFees(address(tokenIn), amountFee);

        uint256 numerator = reserveOut * amountInWithFee;
        uint256 denominator = reserveIn + amountInWithFee;
        amountOut = numerator / denominator;
        require(amountOut > 0, "Dex: Insufficient output amount calculated");

        if (tokenIn == address(tokenA)) {
            _update(reserveIn + amountIn, reserveOut - amountOut);
        } else {
            _update(reserveOut - amountOut, reserveIn + amountIn);
        }

        require(tokenOut.transfer(to, amountOut), "Dex: Output transfer failed");

        emit Swapped(msg.sender, amountIn, tokenIn, amountOut, address(tokenOut), to);
        return amountOut;
    }

    function getReserves() external view returns (uint256 _reserveA, uint256 _reserveB) {
        return (reserveA, reserveB);
    }

    function getPriceAInB() external view returns (uint256 priceA) {
        require(reserveA > 0 && reserveB > 0, "Dex: NO_LIQUIDITY");
        return (reserveB * 1e18) / reserveA;
    }

    function getPriceBInA() external view returns (uint256 priceB) {
        require(reserveA > 0 && reserveB > 0, "Dex: NO_LIQUIDITY");
        return (reserveA * 1e18) / reserveB;
    }
}
