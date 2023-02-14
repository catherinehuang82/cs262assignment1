#ifndef SERVER_HH
#define SERVER_HH

#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <set>
#include <map>
#include <tuple>
#include <thread>

struct clientInfo {
    // mutex handles mutual exclusion of threads that read or write client information
    std::mutex m;

    // set of accounts, used for more efficient account listing
    std::set <std::string> accounts;

    // condition variable
    std::condition_variable_any wakeup;

    // key: client username
    // value: socket ID for the client process
    // NOTE: consider unordered maps
    // NOTE: when an account gets deleted, its entry in this table should get deleted too
    // using the erase() method
    std::map<std::string, int> client_table;

    void addClient(std::string username, int socketId) {
        // client_table.insert({username, socketId});
        accounts.insert(username);
    }

    void deleteClient(std::string username) {
        client_table.erase(username);
        accounts.erase(username);
    }

    // int getSocketID(std::string username) {
    //     return client_table.find(username);
    // }

    std::set<std::string> listAccounts(std::string wildcard) {
        // TODO: implement wildcard matching
        return accounts;
    }
};

struct socketMessage {
    // message string
    std::string data;
    // recipient socket
    int receipient_sd;
};

#endif