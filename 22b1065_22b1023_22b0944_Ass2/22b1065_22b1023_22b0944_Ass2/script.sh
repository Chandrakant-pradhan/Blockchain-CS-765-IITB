#!/bin/bash

# Stop script if any command fails
set -e

# Clean and compile the project
echo "Cleaning and building the project..."
make clean && make

while [[ $# -gt 0 ]]; do
    case "$1" in
        -n) n="$2"; shift 2;;
        -z0) z0="$2"; shift 2;;
        -z1) z1="$2"; shift 2;;
        -Ttx) Ttx="$2"; shift 2;;
        -Tty) Tty="$2"; shift 2;;
        -I) I="$2"; shift 2;;
        -timeLimit) timeLimit="$2"; shift 2;;
        -simtype) simtype="$2"; shift 2;;
        *) echo "Warning: Unknown argument $1"; shift;;
    esac
done


# Run the simulation
echo "Running simulation..."
./simulation -n "$n" -z0 "$z0" -z1 "$z1" -Ttx "$Ttx" -Tty "$Tty" -I "$I" -timeLimit "$timeLimit" -simtype "$simtype" > out.txt 2>&1

# Process all .txt files with generate.py
echo "Processing .txt files..."
found_txt_files=false
for file in peer_*_tree.txt; do
    if [[ -f "$file" ]]; then
        found_txt_files=true
        echo "Processing $file..."
        python3 generate.py "$file"
    fi
done


# Process graph_struct.txt separately for network topology
if [[ -f "graph_struct.txt" ]]; then
    echo "Processing graph_struct.txt for network topology..."
    python3 generate_topology.py
else
    echo "graph_struct.txt not found. Skipping network topology generation."
fi

# Check if no .txt files were found
if [[ "$found_txt_files" == false ]]; then
    echo "No .txt files found!"
    exit 1
fi

# Run Gnuplot script
echo "Generating plots..."
gnuplot plot_script.plt

echo "All tasks completed successfully!"
