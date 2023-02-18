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

clientInfo* client_information;

int main(int argc, char *argv[]) {

    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    //grab the port number
    int port = atoi(argv[1]);
    //buffer to send and receive messages with
    char msg[1500];

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
    for (i = 0; i < max_clients; i++)  
    {  
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
            //socket descriptor 
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

        // create map of all users
        clientInfo* client_map;

        if (FD_ISSET(serverSd, &clientfds)) {
            // accept a new client
            if ((new_socket = accept(serverSd, 
                        (sockaddr *)&newSockAddr, (socklen_t*)&newSockAddrSize))<0)  
            {  
                cerr << "Error accepting request from client!" << endl;
                exit(EXIT_FAILURE);  
            }

            //ask for username, add to client map
            // char* message = "Hi! Enter your username: ";
            // if (send(new_socket, message, strlen(message), 0) != strlen(message) )  
            // {  
            //     perror("Did not properly send client the username requesting message");  
            // } 

            //inform server of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" ,
            new_socket , inet_ntoa(newSockAddr.sin_addr) , ntohs
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
        // while(1) {
            //receive a message from a client (listen)
            // cout << "Awaiting client response..." << endl;
            memset(&msg, 0, sizeof(msg));//clear the buffer
            for (i = 0; i < max_clients; i++) {

                sd = client_socket[i];

                if (FD_ISSET( sd , &clientfds))  {
                    //Check if it was for closing , and also read the 
                    //incoming message 
                    bytesRead = recv(sd, (char*)&msg, sizeof(msg), 0);
                    string msg_string(msg);
                    printf("msg string: %s\n", msg_string.c_str());
                    // operation, username, message
                    size_t pos2 = msg_string.find('\n', 2);
                    printf("pos2: %zu\n", pos2);
                    char operation = msg_string[0];
                    string username = msg_string.substr(2, pos2 - 2);
                    printf("username: %s\n", username.c_str());
                    printf("username length: %lu\n", strlen(username.c_str()));
                    string message = msg_string.substr(pos2 + 1, msg_string.length() - pos2);
                    printf("message: %s\n", message.c_str());
                    printf("message length: %lu\n", strlen(message.c_str()));
                    if (!strcmp(message.c_str(), "exit"))  
                    {
                        //Somebody disconnected , get his details and print 
                        getpeername(sd , (sockaddr*)&newSockAddr , \
                            (socklen_t*)&newSockAddrSize);  
                        printf("Client %d disconnected , ip %s , port %d \n" , i, 
                            inet_ntoa(newSockAddr.sin_addr) , ntohs(newSockAddr.sin_port));  
                            
                        //Close the socket and mark as 0 in list for reuse 
                        close( sd );  
                        client_socket[i] = 0;
                    }  else if (!strcmp(message.c_str(), "listaccounts")) {
                        // TODO: implement fetching the wildcard (maybe have the wildcard be the thing after the whitespace)
                        std::string wildcard = "";
                        std::set<std::string> accounts_list = client_information->listAccounts(wildcard);
                        std::string accounts_list_str;
                        for (std::string a : accounts_list)
                        {
                            accounts_list_str += a;
                            accounts_list_str += '\n';
                        }

                        accounts_list_str.pop_back();
                        printf("listing accounts...\n");

                        // send accounts list to client that requested it
                        bytesWritten = send(client_socket[i], (char*)&accounts_list_str,
                        strlen((char*)&accounts_list_str), 0);
                    }
                    // send the message
                    else 
                    {
                        //set the string terminating NULL byte on the end 
                        //of the data read 
                        // printf("we're here now\n");
                        printf("Client %d: %s\n", i, message.c_str());
                        // bytesWritten = send(sd, (char*)&msg, strlen(msg), 0);
                        if (client_socket[1-i] != 0) {
                            printf("here now\n");
                            bytesWritten = send(client_socket[1-i], message.c_str(), strlen(message.c_str()), 0);
                        }
                        // msg[valread] = '\0';  
                        // send(sd , msg , strlen(msg) , 0 );  
                    }  
                    // bytesRead = recv(client_socket[i], (char*)&msg, sizeof(msg), 0);
                    // if(!strcmp(msg, "exit"))
                    // {
                    //     printf("Client %d has quit the session\n", i);
                    //     //Somebody disconnected , get his details and print 
                    //     getpeername(sd, (sockaddr*)&newSockAddr , \
                    //         (socklen_t*)&newSockAddr);  
                    //     printf("Host disconnected , ip %s , port %d \n" , 
                    //         inet_ntoa(newSockAddr.sin_addr) , ntohs(newSockAddr.sin_port));  
                            
                    //     //Close the socket and mark as 0 in list for reuse 
                    //     close(sd);  
                    //     client_socket[i] = 0; 
                    //     break;
                    // }
                    // printf("Client %d: %s\n", i, msg);

                    // //send the message to client
                    // bytesWritten = send(client_socket[0], (char*)&msg, strlen(msg), 0);
                }
            }
        // }
        // cout << "********Session********" << endl;
        // cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << endl;
        // // cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) 
        //     // << " secs" << endl;
        // cout << "Connection closed..." << endl;

    }

}