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

//Client side

#define TRUE   1 
#define FALSE  0 
// #define PORT 6000 

void listener_thread(int sd, int bytes_read) {
    while(1) {
        //create a message buffer for the message to be received
        char msg_recv[1500]; 
        // reading from server
        memset(&msg_recv, 0, sizeof(msg_recv)); //clear the buffer
        bytes_read += recv(sd, (char*)&msg_recv, sizeof(msg_recv), 0);
        if(!strcmp(msg_recv, "exit")) {
            printf("Server has quit the session.\n");
            break;
        } else {
            // TODO: make this a printf statement
            printf("\nServer: %s\n", msg_recv);
        }
    }
}

int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 4)
    {
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
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    //set of socket descriptors 
    // fd_set clientfd;

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    //try to connect...
    int status = connect(clientSd,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0)
    {
        cout<<"Error connecting to socket"<<endl;
        // this line below was previously a break; line, but that caused an error
        exit(0);
    }

    memset(&msg, 0, sizeof(msg)); //clear the buffer
    strcpy(msg, username_self);

    // send client username to server
    int usernameBytes = send(clientSd, (char*)&msg, strlen(msg), 0);

    cout << "Connected to the server!" << endl;



    int bytesRead, bytesWritten = 0;
    std::thread t(listener_thread, clientSd, bytesRead);
    t.detach();

    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    while(1)
    {
        //clear the socket set 
        // FD_ZERO(&clientfd);  
     
        // //add master socket to set 
        // FD_SET(clientSd, &clientfd);  
        // int max_sd = clientSd;

        // sending to the server

        printf("[Enter operation] >");
        string operation;
        getline(cin, operation);
        // TODO: handle operation
        // 1: send to a user
        // 2: list accounts
        // 3: delete account
        // throw an error if operation is not 1-3
        // in the error, use continue; and not break;

        printf("[Enter recipient username] >");
        string username;
        getline(cin, username);
    
        // TODO: handle bad username

        printf("[Enter message] >");
        string data;
        getline(cin, data);

        //wait for an activity on one of the sockets , timeout is NULL, so wait indefinitely 
        // activity = select( max_sd + 1 , &clientfd , NULL , NULL , NULL);  
       
        // if ((activity < 0) && (errno!=EINTR))  
        // {  
        //     printf("select error");  
        // } 

        // if (FD_ISSET(clientSd , &clientfd)) {
        // sending to server
        // socketMessage* socket_message;
        // socket_message->recipient_username = username;
        // socket_message->message = data;
        string msg_str = operation + '\n' + username + '\n' + data;
        
        memset(&msg, 0, sizeof(msg)); //clear the buffer
        strcpy(msg, msg_str.c_str());
        if(data == "exit")
        {
            send(clientSd, (char*)&msg, strlen(msg), 0);
            break;
        }
        bytesWritten = send(clientSd, (char*)&msg, strlen(msg), 0);
        // }
    }
    gettimeofday(&end1, NULL);
    // close(clientSd);
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    cout << "Connection closed" << endl;
    t.join();
    return 0;    
}