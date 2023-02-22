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
#include "server.h"
using namespace std;

clientInfo* client_info;
std::unordered_map<std::string, int> active_users; // username : id
std::unordered_map<std::string, std::string> logged_out_users; // username : undelivered messages
std::set<std::string> account_set; // all usernames (both logged in AND not logged in)

void sigintHandler( int signum ) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    char msg[1500];
    for (auto it = active_users.begin(); it != active_users.end(); ++it) {
        int sd = it->second;
        memset(&msg, 0, sizeof(msg));
        const char* server_shutdown_msg = "Server shut down permaturely, so logging you out.";
        strcpy(msg, server_shutdown_msg);
        // send(new_socket, (char*)&msg, strlen(msg), 0);
        send(sd, (char*)&msg, sizeof(msg), 0);
    }
   exit(signum);
}

// this should be handled when a client Ctrl+C's
void sigabrtHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";
}

int main(int argc, char *argv[]) {
    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    int port = atoi(argv[1]);
    //buffer to send and receive messages with
    char msg[1500];

    // register signal handler
    signal(SIGINT, sigintHandler);
    signal(SIGABRT, sigabrtHandler);

    //setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    // serverSd: master socket
    int serverSd, addrlen, new_socket , client_socket[10] , 
          max_clients = 10 , curr_clients = 0, activity, i , valread , sd;

    int max_sd; 
    
    //set of socket descriptors 
    fd_set clientfds;

    //initialize all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++) {  
        client_socket[i] = 0;  
    }  

    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    //set master socket to allow multiple connections 
    int iSetOption = 1;
    if( setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, 
          sizeof(iSetOption)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }

    //bind the socket to its local address
    int bindStatus = ::bind(serverSd, (sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }

    printf("Waiting for a client to connect on port %d...\n", port);
    
    //listen for up to 10 requests at a time
    listen(serverSd, 10);
    //receive a request from client using accept
    //we need a new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    while (1) {
        //clear the socket set 
        FD_ZERO(&clientfds);  
     
        //add master socket to set 
        FD_SET(serverSd, &clientfds);  
        max_sd = serverSd;  

        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            sd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &clientfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }

        //wait for an activity on one of the sockets , timeout is NULL, so wait indefinitely 
        activity = select( max_sd + 1 , &clientfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }

        // listen for accepting a new client
        if (FD_ISSET(serverSd, &clientfds)) {
            // accept a new client
            if ((new_socket = accept(serverSd, 
                        (sockaddr *)&newSockAddr, (socklen_t*)&newSockAddrSize))<0)  
            {  
                cerr << "Error accepting request from client!" << endl;
                exit(EXIT_FAILURE);  
            }


            // ask client for username (client will send upon connecting to the server)
            memset(&msg, 0, sizeof(msg)); //clear the buffer
            recv(new_socket, (char*)&msg, sizeof(msg), 0);
            std::string new_client_username(msg);

            if (active_users.find(new_client_username) != active_users.end()) {
                int existing_login_sd = active_users[new_client_username];
                memset(&msg, 0, sizeof(msg));
                const char* force_logout_msg = "Another user logged in as your name, so logging you out.";
                strcpy(msg, force_logout_msg);
                send(existing_login_sd, (char*)&msg, sizeof(msg), 0);
            }
            active_users[new_client_username] = new_socket;
            account_set.insert(new_client_username);

            // check whether this user has any undelivered messages to it
            // if so, send these messages, and remove user from mapping of logged-out users
            // this means the user has logged in previously
            auto it_check_undelivered = logged_out_users.find(new_client_username);
            if (it_check_undelivered != logged_out_users.end()) {
                string undelivered_messages = it_check_undelivered->second;
                memset(&msg, 0, sizeof(msg)); //clear the buffer
                strcpy(msg, undelivered_messages.c_str());
                send(new_socket, undelivered_messages.c_str(), strlen(undelivered_messages.c_str()), 0);
            }

            //inform server of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d, username is %s, ip is: %s, port: %d\n",
            new_socket, msg, inet_ntoa(newSockAddr.sin_addr), ntohs
                  (newSockAddr.sin_port));

            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                         
                    break;
                }  
            }
        }

        int bytesRead, bytesWritten = 0;

        // clear the buffer
        memset(&msg, 0, sizeof(msg));
        for (i = 0; i < max_clients; i++) {

            sd = client_socket[i];

            if (FD_ISSET( sd , &clientfds))  {
                //Check if it was for closing , and also read the incoming message 
                bytesRead = recv(sd, (char*)&msg, sizeof(msg), 0);
                string msg_string(msg);
                printf("msg string: %s\n", msg_string.c_str());
                
                char operation = msg_string[0];

                // handle finding the username of the sender (of message or operation)
                string sender_username;
                for (auto it_find_sender = active_users.begin(); it_find_sender != active_users.end(); ++it_find_sender)  {
                    // if the username's corresponding socketfd is the same as sd
                    if (it_find_sender->second == sd) {
                        sender_username = it_find_sender->first;
                    }
                }

                if (operation == '4'){ //quit 
                    quitUser(sd, client_socket, newSockAddr, newSockAddrSize, sender_username, i);
                    continue;
                } else if (operation == '3') { //delete account
                    deleteAccount(sd, client_socket, sender_username, i);
                    continue;
                }

                // operation, username, message
                size_t pos2 = msg_string.find('\n', 2);
                printf("pos2: %zu\n", pos2);

                string username = msg_string.substr(2, pos2 - 2);
                printf("username: %s\n", username.c_str());
                printf("username length: %lu\n", strlen(username.c_str()));
                string message = msg_string.substr(pos2 + 1, msg_string.length() - pos2);
                printf("message: %s\n", message.c_str());
                printf("message length: %lu\n", strlen(message.c_str()));

                if (operation == '2') { // list accounts
                    listAccounts(message, client_socket, bytesWritten, account_set, i);
                } else if (operation == '1') { // send message
                    sendMessage(username, message, sender_username, client_socket, bytesWritten, i);
                }
            }
        }
    }
}
