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
#include <tuple>
#include <random>
#include <deque>
#include <functional>
#include <mutex>
#include <map>
#include <set>
using namespace std;
//Server side

#define TRUE   1 
#define FALSE  0 
// #define PORT 6000 

// client_info* client_information;

int main(int argc, char *argv[])
{
    // for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    // grab the port number
    int port = atoi(argv[1]);
    // buffer to send and receive messages with
    char msg[1500];
     
    // setup a socket and connection tools
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    // client socket ID
    int newSd;

     // we need a new address to connect with the client
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);

    // child process id (updated when we fork)
    pid_t childpid;
 
    // open stream oriented socket with internet address
    // also keep track of the socket descriptor
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);

    // set SO_REUSEADDR to 1 to allow reuse of local addresses during bind()
    int iSetOption = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    
    if(serverSd < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }
    // bind the socket to its local address
    int bindStatus = ::bind(serverSd, (struct sockaddr*) &servAddr, 
        sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
    cout << "Waiting for a client to connect..." << endl;
    //listen for up to 5 requests at a time
    listen(serverSd, 5);

    // number of clients
    // TODO: put this in the client_info struct
    int cnt = 0;

    while (1) {
        // receive a request from client using accept
        // sockaddr_in newSockAddr;
        // socklen_t newSockAddrSize = sizeof(newSockAddr);
        // accept, create a new socket descriptor to 
        // handle the new connection with client
        newSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if(newSd < 0)
        {
            cerr << "Error accepting request from client!" << endl;
            exit(1);
        }
        // display information of
        // connected client
        printf("Connection accepted from %s:%d\n",
               inet_ntoa(newSockAddr.sin_addr),
               ntohs(newSockAddr.sin_port));

        // print number of clients connected up until now
        printf("Clients connected: %d\n\n",
               ++cnt);

        // create a child process
        if ((childpid == fork()) == 0) {

            // struct timeval start1, end1;
            // gettimeofday(&start1, NULL);

            // keep track of the amount of data sent
            int bytesRead, bytesWritten = 0;
            while(1)
            {
                //receive a message from the client (listen)
                cout << "Awaiting client response..." << endl;
                memset(&msg, 0, sizeof(msg));//clear the buffer
                bytesRead += recv(newSd, (char*)&msg, sizeof(msg), 0);
                if(!strcmp(msg, "exit"))
                {
                    cout << "Client has quit the session" << endl;
                    // decrement the number of clients currently connected to the server
                    --cnt;
                    break;
                }
                cout << "Client: " << msg << endl;
                cout << ">";
                string data;
                getline(cin, data);
                memset(&msg, 0, sizeof(msg)); //clear the buffer
                strcpy(msg, data.c_str());
                if(data == "exit")
                {
                    //send to the client that server has closed the connection
                    send(newSd, (char*)&msg, strlen(msg), 0);
                    // close the server socket id
                    close(serverSd);
                    break;
                }
                // send the message to client
                // NOTE: I think this is where the sender can specify which client to send to 
                // (the first argument is client socket ID)
                bytesWritten += send(newSd, (char*)&msg, strlen(msg), 0);
            }
            // we need to close the socket descriptors after we're all done
            // gettimeofday(&end1, NULL);
            // close(serverSd);
            cout << "********Session********" << endl;
            cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
            // cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) 
            //     << " secs" << endl;
            cout << "Connection closed..." << endl;
        }
    }
    // close the client socket ID
    close(newSd);
    return 0;   
}