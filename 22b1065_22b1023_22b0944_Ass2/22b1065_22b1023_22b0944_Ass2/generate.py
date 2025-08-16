import sys
import re
import networkx as nx
import matplotlib.pyplot as plt
from collections import Counter

def read_blocks_from_file(filename):
    """
    Reads the block data from the file and extracts:
      - Edges (parent_id, block_id) for the tree
      - Miner counts
      - Mapping of block_id -> is_malicious (if Block Miner == Ringmaster)
    """
    edges = []
    miner_counts = Counter()
    block_status = {}  # block_id -> True (malicious) or False (honest)

    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue  # Skip empty lines

            parts = line.split(', ')
            try:
                block_id = int(parts[0].split(': ')[1])
                parent_id = int(parts[1].split(': ')[1])
            except Exception:
                print(f"Error parsing block id or parent id in line: {line}")
                continue

            edges.append((parent_id, block_id))

            # Extract Block Miner and Ringmaster
            block_miner = None
            ringmaster = None
            for part in parts:
                if part.startswith("Block Miner"):
                    block_miner = int(part.replace("Block Miner ", "").strip())
                elif part.startswith("ringmaster"):
                    ringmaster = int(part.replace("ringmaster ", "").strip())

            # Mark the block as malicious if Block Miner == Ringmaster
            if block_miner is not None and ringmaster is not None:
                block_status[block_id] = (block_miner == ringmaster)

            # Update miner count
            if block_miner is not None:
                miner_counts[block_miner] += 1

    return edges, miner_counts, block_status

def draw_block_tree(edges, block_status, filename):
    """
    Draws and saves the block tree using NetworkX.
    - Malicious blocks are colored RED.
    - Honest blocks are colored BLUE.
    """
    G = nx.DiGraph()
    G.add_edges_from(edges)

    # Assign colors based on block type
    node_colors = ["#FF3333" if block_status.get(node, False) else "#3366FF" for node in G.nodes()]
    
    plt.figure(figsize=(15, 5))
    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog="dot", args="-Grankdir=LR")
    except Exception:
        print("Graphviz layout not available, using spring layout instead.")
        pos = nx.spring_layout(G)

    nx.draw(G, pos, with_labels=True, node_size=1000,
            node_color=node_colors, font_color='white', edge_color='black', font_size=10)
    
    plt.title(f"Block Tree Structure: {filename}", fontsize=12)
    
    output_filename = f"block_tree_{filename.split('_')[1].split('.')[0]}.png"
    plt.savefig(output_filename)
    print(f"Block tree saved as {output_filename}")
    plt.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 generate_block_tree.py <filename>")
        sys.exit(1)
    
    filename = sys.argv[1]
    edges, miner_counts, block_status = read_blocks_from_file(filename)

    print("\nCounts of blocks mined by each miner:")
    for miner, count in miner_counts.items():
        print(f"  Miner {miner}: {count} block(s)")

    # Draw and save the block tree
    draw_block_tree(edges, block_status, filename)
