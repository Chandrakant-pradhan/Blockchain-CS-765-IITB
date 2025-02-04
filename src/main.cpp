#include "headers.hpp"
using namespace std;

int main(int argc, char* argv[]) {
    int n = 50;
    double z0 = 20, z1 = 20;
    double Ttx = 5.0 , Tk = 5.0;
    unsigned int seed = 42;
    int timeLimit = 3600;


    for (int i = 1; i < argc; i += 2) {  
        if (i + 1 >= argc) {  
            cerr << "Error: Missing value for " << argv[i] << "\n";
            return 1;
        }

        string arg = argv[i];
        if (arg == "-n") n = stoi(argv[i + 1]);
        else if (arg == "-z0") z0 = stod(argv[i + 1]);
        else if (arg == "-z1") z1 = stod(argv[i + 1]);
        else if (arg == "-Ttx") Ttx = stod(argv[i + 1]);
        else if (arg == "-Tk") Tk = stod(argv[i+1]);
        else if (arg == "-seed") seed = stoul(argv[i + 1]);
        else if (arg == "-timeLimit") timeLimit = stoi(argv[i + 1]);
        else {
            cerr << "Warning: Unknown argument " << arg << "\n";
        }
    }

    //initialize simulation
    //TODO
    //run the simulation
    //TODO

    return 0;
}
