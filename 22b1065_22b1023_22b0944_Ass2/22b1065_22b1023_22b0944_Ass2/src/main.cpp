#include "defs.hpp"


int main(int argc, char* argv[]) {

    //default parameters
    int n = 10;
    ld z0 = 0, z1 = 50;
    ld Ttx = 5, Tty = 0.5,  I = 5;
    int timeLimit = 1000;
    sim_flag = 0;
    //0 for eclipse + mining attack
    //1 for mining attack

    //taking command line arguments
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
        else if (arg == "-Tty") Tty = stold(argv[i + 1]);
        else if (arg == "-I") I = stold(argv[i + 1]);
        else if (arg == "-timeLimit") timeLimit = stoi(argv[i + 1]);
        else if (arg == "-simtype") sim_flag = stoi(argv[i + 1]);
        else {
            cerr << "Warning: Unknown argument " << arg << "\n";
        }
    }
    
    //instantiating the simulation
    Simulation simulation(n, z0, z1, Ttx, Tty, I, timeLimit);

    //running the simulation
    simulation.run();
    
    //printing statistics
    cout<<"Peer ID, Fraction of Blocks in Longest Chain"<<endl;
    for(int i=0 ; i<n ; i++){
        Peer* node = (&simulation)->peer_map[i];
        DEBUG_PRINT();
        int denom = node->total_block();
        DEBUG_PRINT();
        int num = node->countMyBlks();
        DEBUG_PRINT();
        cout << "Peer " << i << " "<<(node->low ? "low" : "high")<<" ";
        if (denom == 0) {
            cout << "No Block mined";
        } else {
            cout << "ratio : " << (num / (ld)denom);
        }
        cout << endl;
    }

    return 0;
}
