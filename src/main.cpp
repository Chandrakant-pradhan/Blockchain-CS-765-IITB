#include "defs.hpp"

int main(int argc, char* argv[]) {
    int n = 5;
    ld z0 = 20, z1 = 20;
    ld Ttx = 5.0, I = 20;
    int timeLimit = 360;

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
    for(int i=0; i<n; i++){
        Peer* node = (&simulation)->peer_map[i];
        for(int j=0; j<n; j++){
            cout<<node->balance[j]<<"**";
        }
        cout<<endl;
    }
    simulation.run();
    for(int i=0; i<n; i++){
        Peer* node = (&simulation)->peer_map[i];
        for(int j=0; j<n; j++){
            cout<<node->balance[j]<<"**";
        }
        cout<<endl;
    }

    return 0;
}
