
# Blockchain-CS-765-IITB

## Running the Simulation
1. To generate the output files, run the following command:

    ```bash
    bash script.sh -n 20 -z0 50 -z1 0 -Ttx 50000 -Tty 0.5 -I 10 -timeLimit 1000 -simtype 1

2. The output files generated would be as follows
   - peer_id_tree.txt files containing tree structure
   - block_tree_id.png representing the blockchain for peer
   - out.txt which will contain information about the ratio of malicious nodes

3. On the terminal following things will be printed
    1. for each of the block the number of blocks mined by other peers in its blockchain