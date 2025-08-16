import networkx as nx
import matplotlib.pyplot as plt

def read_graph_structure(filename):
    """ Reads attack_list and adjacency_list from the given file. """
    attack_edges = set()
    adjacency_list = {}

    with open(filename, "r") as file:
        lines = file.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        if line == "attack_list":
            i += 1
            while i < len(lines) and lines[i].strip():
                parts = lines[i].split(":")
                if len(parts) == 2 and parts[1].strip():
                    node = int(parts[0].strip())
                    neighbors = list(map(int, parts[1].split()))
                    for neighbor in neighbors:
                        attack_edges.add(tuple(sorted((node, neighbor))))  # Store attack edges properly
                i += 1
        elif line == "adjacent_list":
            i += 1
            while i < len(lines) and lines[i].strip():
                parts = lines[i].split(":")
                if len(parts) == 2 and parts[1].strip():
                    node = int(parts[0].strip())
                    neighbors = list(map(int, parts[1].split()))
                    adjacency_list[node] = neighbors
                i += 1
        i += 1

    return attack_edges, adjacency_list

def draw_network_topology(filename):
    """ Draws and saves a network topology graph with different edge styles and node colors. """
    attack_edges, adjacency_list = read_graph_structure(filename)
    
    G = nx.Graph()
    
    normal_edges = []
    malicious_edges = list(attack_edges)  # Convert set to list for graph visualization
    
    # Adding nodes and edges from adjacency_list
    for node, neighbors in adjacency_list.items():
        for neighbor in neighbors:
            edge = tuple(sorted((node, neighbor)))  # Ensure undirected edge consistency
            
            if edge not in G.edges:
                G.add_edge(node, neighbor)
            
            # Classify edges
            if edge in attack_edges:
                malicious_edges.append(edge)  # Malicious group edges (black)
            else:
                normal_edges.append(edge)  # Normal adjacent edges (dashed)

    # Assign colors to nodes
    malicious_nodes = set(n for edge in malicious_edges for n in edge)  # Nodes appearing in attack_list
    node_colors = ["red" if node in malicious_nodes else "lightgreen" for node in G.nodes()]
    
    # Draw graph
    plt.figure(figsize=(8, 6))
    pos = nx.spring_layout(G, seed=42)  

    # Draw malicious edges in black
    nx.draw_networkx_edges(G, pos, edgelist=malicious_edges, edge_color="black", width=4)

    # Draw normal adjacent edges in dashed style
    nx.draw_networkx_edges(G, pos, edgelist=normal_edges, edge_color="blue", width=4 )

    # Draw nodes and labels
    nx.draw_networkx_nodes(G, pos, node_color=node_colors, node_size=800)
    nx.draw_networkx_labels(G, pos, font_size=10, font_color="black")
    
    # Save and display the graph
    output_filename = "network_topology_fixed.png"
    # plt.title("Network Topology (Red = Malicious Nodes, Black = Malicious Edges, Blue Dashed = Adjacent Edges)")
    plt.savefig(output_filename)
    print(f"Network topology graph saved as {output_filename}")
    plt.close()

if __name__ == "__main__":
    filename = "graph_struct.txt"
    draw_network_topology(filename)
