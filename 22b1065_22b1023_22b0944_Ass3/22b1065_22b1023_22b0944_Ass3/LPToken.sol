// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "@openzeppelin/contracts/access/Ownable.sol";

contract LPToken is ERC20, Ownable {
    struct RewardSnapshot {
        address tokenAddress;
        uint256 amount;
    }

    struct LPInfo {
        uint256 liquidity;
        RewardSnapshot[] rewardsA;
        RewardSnapshot[] rewardsB;
    }

    mapping(address => LPInfo) public lpInfo;
    address[] public allLPs;

    uint256 public accFeePerShareA;
    uint256 public accFeePerShareB;
    uint256 public totalLiquidity;

    address public TokenA;
    address public TokenB;

    constructor() ERC20("DEX LP Token", "DLP") Ownable(msg.sender) {}

    function mint(address account, uint256 amount) external onlyOwner {
        _mint(account, amount);

        if (lpInfo[account].liquidity == 0) {
            allLPs.push(account);
        }
        storeHistory();

        lpInfo[account].liquidity += amount;
        totalLiquidity += amount;
    }

    function burnFrom(address account, uint256 amount) external onlyOwner {
        _burn(account, amount);

        storeHistory();

        lpInfo[account].liquidity -= amount;
        totalLiquidity -= amount;
    }

    function storeHistory() internal {
        for (uint i = 0; i < allLPs.length; i++) {
            address user = allLPs[i];
            uint256 rewardA = (lpInfo[user].liquidity * accFeePerShareA) / 1e18;
            lpInfo[user].rewardsA.push(RewardSnapshot(TokenA, rewardA));
        }
        for (uint i = 0; i < allLPs.length; i++) {
            address user = allLPs[i];
            uint256 rewardB = (lpInfo[user].liquidity * accFeePerShareB) / 1e18;
            lpInfo[user].rewardsB.push(RewardSnapshot(TokenB, rewardB));
        }
        accFeePerShareA = 0;
        accFeePerShareB = 0;
    }

    function distributeFees(address feeToken, uint256 feeAmount) external onlyOwner {
        require(totalLiquidity > 0, "No liquidity");
        if (TokenA == address(0) && TokenB == address(0)) {
            TokenA = feeToken;
            accFeePerShareA += (feeAmount * 1e18) / totalLiquidity;
        } else if (TokenB == address(0)) {
            TokenB = feeToken;
            accFeePerShareB += (feeAmount * 1e18) / totalLiquidity;
        } else {
            if (feeToken == TokenA) {
                accFeePerShareA += (feeAmount * 1e18) / totalLiquidity;
            } else {
                accFeePerShareB += (feeAmount * 1e18) / totalLiquidity;
            }
        }
    }

    function claimRewards(address lp, address tokenA_, address tokenB_) external onlyOwner returns (uint256 tokenAEarned, uint256 tokenBEarned) {
        uint256 totalA = 0;
        uint256 totalB = 0;

        RewardSnapshot[] storage rewardsA = lpInfo[lp].rewardsA;
        RewardSnapshot[] storage rewardsB = lpInfo[lp].rewardsB;

        for (uint256 i = 0; i < rewardsA.length; i++) {
            if (rewardsA[i].tokenAddress == tokenA_) {
                totalA += rewardsA[i].amount;
            }
        }
        for (uint256 i = 0; i < rewardsB.length; i++) {
            if (rewardsB[i].tokenAddress == tokenB_) {
                totalB += rewardsB[i].amount;
            }
        }

        delete lpInfo[lp].rewardsA;
        delete lpInfo[lp].rewardsB;

        return (totalA, totalB);
    }
}
