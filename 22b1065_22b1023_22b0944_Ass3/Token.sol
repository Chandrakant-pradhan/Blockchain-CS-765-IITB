// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "@openzeppelin/contracts/token/ERC20/extensions/ERC20Permit.sol";

contract Token is ERC20, ERC20Permit {
    constructor() ERC20("Token", "TK") ERC20Permit("Token") {
        uint256 initialSupply = 1_000_000 * (10**decimals());
        _mint(msg.sender, initialSupply);
    }
}