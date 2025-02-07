#include "defs.hpp"

int main(int argc, char* argv[]) {
    int n = 50;
    ld z0 = 20, z1 = 20;
    ld Ttx = 5.0, I = 600;
    int timeLimit = 3600;

    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            cerr << "Error: Missing value for " << argv[i] << "\n";
            return 1;
        }

        string arg = argv[i];
        if (arg == "-n") n = stoi(argv[i + 1]);
        else if (arg == "-z0") z0 = stold(argv[i + 1]);
        else if (arg == "-z1") z1 = stold(argv[i + 1]);
        else if (arg == "-Ttx") Ttx = stold(argv[i + 1]);
        else if (arg == "-I") I = stold(argv[i + 1]);
        else if (arg == "-timeLimit") timeLimit = stoi(argv[i + 1]);
        else {
            cerr << "Warning: Unknown argument " << arg << "\n";
        }
    }

    Simulation simulation(n, z0, z1, Ttx, I, timeLimit);
    simulation.run();
    return 0;
}
