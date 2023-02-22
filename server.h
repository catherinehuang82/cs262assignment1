#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include "server.hh"
using namespace std;

string sendMessage(string username, string message, string sender_username, int* client_socket, \
    int &bytesWritten, std::unordered_map<std::string, int> active_users, \
    std::unordered_map<std::string, std::string> &logged_out_users, int i) {
    //set the string terminating NULL byte on the end of the data read 
    auto active_it = active_users.find(username);
    auto logged_out_it = logged_out_users.find(username);

    // if username is not found at all (regardless of whether the user is logged in or not)
    // TODO: change this to just iterating through the account_set set of usernames
    if (active_it == active_users.end() && logged_out_it == logged_out_users.end()) {  
        // not found  
        string username_not_found_error = "Username does not exist, try sending to another user.";
        bytesWritten = send(
            client_socket[i],
            username_not_found_error.c_str(),
            strlen(username_not_found_error.c_str()), 0);
        return "";
    }  

    if (logged_out_it != logged_out_users.end()) {
        // user is logged out, save message for them
        logged_out_users[username] = logged_out_users[username] + sender_username + ": " + message + "\n";
        return message;
    }
    else {
        // user is logged in, so just send right now
        int client_socket_fd = active_it->second;
        message = sender_username + ": " + message;
        printf("Client: %d, username: %s, message: %s\n", i, sender_username.c_str(), message.c_str());
        // bytesWritten = send(sd, (char*)&msg, strlen(msg), 0);
        if (client_socket_fd != 0) {
            printf("sending message to recipient...\n");
            bytesWritten = send(client_socket_fd, message.c_str(), strlen(message.c_str()), 0);
        }
        return message;
    } 
}

string listAccounts(string wildcard, int* client_socket, int &bytesWritten, \
    std::set<std::string> &account_set, int i) {    
    // string with all matching accounts 
    std::string matching_accounts;
    for (std::string a : account_set)
    {
        // if a username contains wildcard, add to string
        if (a.find(wildcard) != std::string::npos) {
            matching_accounts += a;
            matching_accounts += '\n';
        }
    }

    // accounts_list_str.pop_back();
    printf("listing accounts...\n");

    // send accounts list to client that requested it
    bytesWritten = send(client_socket[i], (char*)&matching_accounts,
        strlen((char*)&matching_accounts), 0);
    return matching_accounts;
}

void deleteAccount(int sd, int* client_socket, string sender_username, \
    std::unordered_map<std::string, int> &active_users, std::set<std::string> &account_set, \
    std::unordered_map<std::string, std::string> &logged_out_users, int i) {
    // delete the entry from the active_users mapping
    // AND delete the username from the account_set set
    active_users.erase(sender_username);
    account_set.erase(sender_username);
    
    // if this user has ever logged in in the past
    if (logged_out_users.find(sender_username) != logged_out_users.end()) {
        logged_out_users.erase(sender_username);
    }
    printf("%s deleted their account.\n", sender_username.c_str());

    //Close the socket and mark as 0 in list for reuse 
    close( sd );
    client_socket[i] = 0;
}

void quitUser(int sd, int* client_socket, sockaddr_in newSockAddr, socklen_t newSockAddrSize, \
    string sender_username, std::unordered_map<std::string, int> &active_users, \
    std::unordered_map<std::string, std::string> &logged_out_users, int i) {
    //Somebody disconnected , get their details and print 
    getpeername(sd , (sockaddr*)&newSockAddr , (socklen_t*)&newSockAddrSize);  
    printf("Client %d disconnected , ip %s , port %d \n" , i, 
        inet_ntoa(newSockAddr.sin_addr) , ntohs(newSockAddr.sin_port));

    //Close the socket and mark as 0 in list for reuse 
    close( sd );
    client_socket[i] = 0;

    // handle logging out the user by deleting from active senders map
    active_users.erase(sender_username);
    logged_out_users[sender_username] = "Messages you missed when you were gone:\n";
}
