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

// allows a user to send a message to another
string sendMessage(string username, string message, string sender_username, int* client_socket, \
    int &bytesWritten, std::unordered_map<std::string, int> active_users, \
    std::unordered_map<std::string, std::string> &logged_out_users, int i) {

    auto active_it = active_users.find(username);
    auto logged_out_it = logged_out_users.find(username);

    // if username is not found at all (regardless of whether the user is logged in or not)
    if (active_it == active_users.end() && logged_out_it == logged_out_users.end()) {  
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

        if (client_socket_fd != 0) {
            printf("sending message to recipient...\n");
            bytesWritten = send(client_socket_fd, message.c_str(), strlen(message.c_str()), 0);
        }
        return message;
    } 
}

// allows a user to check which accounts exist, including logged out ones, that contain a wildcard
string listAccounts(string wildcard, int* client_socket, int &bytesWritten, \
    std::set<std::string> &account_set, int i) {    

    std::string matching_accounts; //string with all matching accounts
    for (std::string a : account_set)
    {
        // if a username contains wildcard, add to string
        if (a.find(wildcard) != std::string::npos) {
            matching_accounts += a;
            matching_accounts += '\n';
        }
    }

    printf("listing accounts...\n");

    // send accounts list to client that requested it
    bytesWritten = send(client_socket[i], (char*)&matching_accounts,
        strlen((char*)&matching_accounts), 0);
    return matching_accounts;
}

// allows a user to delete their own 
void deleteAccount(int sd, int* client_socket, string sender_username, \
    std::unordered_map<std::string, int> &active_users, std::set<std::string> &account_set, \
    std::unordered_map<std::string, std::string> &logged_out_users, int i) {

    // delete user from the active_users mapping AND account_set set
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

// log out a user, end session
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
