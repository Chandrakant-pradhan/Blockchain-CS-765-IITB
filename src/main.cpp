#include "simulation.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

int main(int argc, char* argv[]) {
    int n = 50;
    double z0 = 20, z1 = 20;
    double Ttx = 5.0, Tk = 5.0;
    unsigned int seed = 42;
    int timeLimit = 3600;

    // Parse command-line arguments
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            std::cerr << "Error: Missing value for " << argv[i] << "\n";
            return 1;
        }

        std::string arg = argv[i];
        if (arg == "-n") n = std::stoi(argv[i + 1]);
        else if (arg == "-z0") z0 = std::stod(argv[i + 1]);
        else if (arg == "-z1") z1 = std::stod(argv[i + 1]);
        else if (arg == "-Ttx") Ttx = std::stod(argv[i + 1]);
        else if (arg == "-Tk") Tk = std::stod(argv[i + 1]);
        else if (arg == "-seed") seed = std::stoul(argv[i + 1]);
        else if (arg == "-timeLimit") timeLimit = std::stoi(argv[i + 1]);
        else {
            std::cerr << "Warning: Unknown argument " << arg << "\n";
        }
    }

    // Set random seed
    srand(seed);

    // Create and run simulation
    Simulation sim(n, z0, z1, Ttx, Tk, timeLimit);
    sim.run();

    return 0;
}
