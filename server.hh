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

struct client_info {
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
};


#endif