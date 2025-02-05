#ifndef HEADERS_HPP
#define HEADERS_HPP

#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <set>
#include <vector>  

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

// Corrected forward declarations for derived classes
class GenerateTransaction;  
class ForwardTransaction;   
class ReceiveTransaction;   
class ReceiveBlock;   
class ForwardBlock;   
class BroadcastBlock;   

#endif // HEADERS_HPP
