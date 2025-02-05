#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "peer.hpp"
#include "event.hpp"
#include <vector>
#include <queue>

class Simulation {
public:
    Simulation(int n, double z0, double z1, double Ttx, double Tk, int timeLimit);
    void run();

private:
    std::priority_queue<Event*, std::vector<Event*>, Event::Compare> event_queue;
    std::vector<Peer> peers;
    int n;
    double z0, z1;
    double Ttx, Tk;
    int timeLimit;

    void assign_slow_fast();
    void assign_low_high();
    void create_connected_graph();
    void setup_peers();
    void generate_initial_events();
    void process_event(Event* event);
    void generate_other_events(Event* event);
};

#endif // SIMULATION_HPP
