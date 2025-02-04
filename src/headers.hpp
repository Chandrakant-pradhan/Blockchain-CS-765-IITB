#ifndef DEFS_H
#define DEFS_H

#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <set>

// Type aliases
typedef long long ll;
typedef unsigned int uint;
typedef unsigned long long ull;
typedef long double ld;

// Forward declarations of classes
class Transaction;
class Block;
class Blockchain;
class Link;
class Event;
class GenerateTransaction : public Event;
class ForwardTransaction : public Event;
class ReceiveTransaction : public Event;
class ReceiveBlock : public Event;
class ForwardBlock : public Event;
class BroadcastBlock : public Event;

#endif // DEFS_H
