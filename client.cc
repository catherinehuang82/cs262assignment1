#include "server.hh"
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
#include <atomic>
#include <thread>
#include <random>
#include <deque>
#include <mutex>
using namespace std;

#define TRUE   1 
#define FALSE  0 
// #define PORT 6000 

void listener_thread(int sd, int bytes_read) {
    while(1) {
        // create a message buffer for the message to be received
        char msg_recv[1500]; 
        // reading from server
        memset(&msg_recv, 0, sizeof(msg_recv)); // clear the buffer
        bytes_read += recv(sd, (char*)&msg_recv, sizeof(msg_recv), 0);
        if (!strcmp(msg_recv, "Server shut down permaturely, so logging you out.")) {
            printf("\nServer shut down permaturely, so logging you out.\n");
            exit(1);
        } else if (!strcmp(msg_recv, "Another user logged in as your name, so logging you out.")) {
            char msg[1500]; 
            // clear the buffer
            memset(&msg, 0, sizeof(msg));
            strcpy(msg, "4");
            // tell the server that client has logged out by sending a "4" over
            send(sd, (char*)&msg, strlen(msg), 0);
            // terminate the client process (both this thread and the main thread)
            printf("\nAnother user logged in as your name, so logging you out.\n");
            exit(1);
        } else {
            printf("\n%s\n", msg_recv);
        }
    }
}

int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 4){
        cerr << "Usage: ip_address port username" << endl; exit(0); 
    } //grab the IP address and port number 
    char *serverIp = argv[1]; 
    int port = atoi(argv[2]); 
    char *username_self = argv[3];
    //create a message buffer 
    char msg[1500]; 
    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    // server address
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    //try to connect...
    int status = connect(clientSd,(sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0)
    {
        cout<<"Error connecting to socket"<<endl;
        exit(0);
    }

    memset(&msg, 0, sizeof(msg)); //clear the buffer
    strcpy(msg, username_self);

    // send client username to server
    int usernameBytes = send(clientSd, (char*)&msg, strlen(msg), 0);

    cout << "Connected to the server!" << endl;

    int bytesRead, bytesWritten = 0;
    std::thread t(listener_thread, clientSd, bytesRead);

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    while(1) {
        printf("[Enter operation] >");
        string operation;
        getline(cin, operation);
        if (strcmp(operation.c_str(), "1")
        && strcmp(operation.c_str(), "2")
        && strcmp(operation.c_str(), "3")
        && strcmp(operation.c_str(), "4")
        ) {
            string bad_operation_error = "Please enter 1 (send a message to a use), 2 (list accounts), or 3 (delete account).";
            printf("%s\n", bad_operation_error.c_str());
            continue;
        }

        // handle operations 3 and 4: deleting account and logging out, respectively
        if (!strcmp(operation.c_str(), "4") || !strcmp(operation.c_str(), "3")) {
            memset(&msg, 0, sizeof(msg)); //clear the buffer
            strcpy(msg, operation.c_str());
            send(clientSd, (char*)&msg, strlen(msg), 0);
            if (!strcmp(operation.c_str(), "4")) {
                printf("Logging you out. Goodbye!\n");
            } else {
                printf("Successfully deleted account.\n");
            }
            break;
        }

        string username = "";

        if (!strcmp(operation.c_str(), "1")) {
            printf("[Enter recipient username] >");
            getline(cin, username);
        }

        if (!strcmp(operation.c_str(), "2")) {
            printf("[Enter account matching wildcard] >");
        } else if (!strcmp(operation.c_str(), "1")) {
            printf("[Enter message] >");
        }
        // either the message string (operation 1)
        // or the text wildcard with which we match accounts (operation 2)
        string data;
        getline(cin, data);

        string msg_str = operation + '\n' + username + '\n' + data;
        
        memset(&msg, 0, sizeof(msg)); //clear the buffer
        strcpy(msg, msg_str.c_str());
        bytesWritten = send(clientSd, (char*)&msg, strlen(msg), 0);
    }
    gettimeofday(&end1, NULL);
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    cout << "Connection closed" << endl;
    t.detach();
    return 0;    
}
