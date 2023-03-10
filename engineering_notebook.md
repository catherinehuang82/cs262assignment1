Log of ideas/updates!

Feb 8, 2023
=============================================================================
- Got a basic connection/messaging service working!
    - Beween one server and one client
- Questions
    - How to know which ports are available to connect to?
    - What kind of text wildcards do we need to support? (Wrt #2 on the spec)
        - Are prefixes/suffixes/substrings enough? Or do we need to support more complicated stuff like regexp?
        - One group is using tries, but these only support prefix/suffix
    - How to exit from chat — is newline “\n” fine?
    - When can the user list accounts? Should the user be able to trigger this feature anytime using a certain message string? Or does the user only need to be able to do this before they connect to a client/server?
- Things to add/fix
    - Currently assumes server and client must message back and forth:
        - one side waits until the other responds to be able to send a message.
        - We should allow people to message without waiting.
    - Need to keep track of multiple users with usernames and make threads
      for each one. Be able to list accounts. Specify which user you want to
      send a message to when sending messages.
    - Implement ability to delete accounts.
    - Implement queueing messages for users that are not currently logged in.
    - Implement error for nonexistent users.

Feb 11/12, 2023
=============================================================================
Progress:
- implemented forking so that multiple clients can talk to the server
- tested three clients talking to the server at the same time
- created original_client.cc and original_server.cc to "save" our baseline client/server implementations (without forking, only supports one client connected to the server at once)

Bugs:
- the server cannot use the same port twice in the same session. In other words, it cannot hold a complete conversation with port 6000 and then use port 6000 again. 
    - if it does, we get a "Error binding socket to local address" error message, which prints out when the std::bind(serverSd, (struct sockaddr*) &servAddr, sizeof(servAddr)); function call status is < 0.
    - however, if we have the server run on port 6001, there is no bug, and we get the desired "Waiting for a client to connect..." Functionality
    - maybe this could help? https://stackoverflow.com/questions/548879/releasing-bound-ports-on-process-exit
- issues with the server closing off ports
    - when the server is down or operating on a different port (e.g. 6001) but the client tries to connect to a port that the server previously opened (e.g. 6000), trouble arises: the client gets "Connected to the server! \n >"
    - but the server cannot talk to the client
    - the client has no choice but to ^C out of the program, but when it does that, the server enters into an infinite loop, where the message sent to the server is "Awaiting client response..."
    - something like this: ">Awaiting client response..." "Client:" ">Awaiting client response..." "Client:" etc.
    - so I'm thinking we need to properly close server ports when the server shuts down <- there is probably an issue here
    - this could be tied to the bug above where the server somehow cannot use a port that it previously used but is now done using

Missing features:
- server cannot choose which client recipient to send message to, so it just cycles between each client when sending messages
- we do not ask clients for their usernames
- we do not track which clients (and usernames) are associated with which socket IDs

Things I've tried:
- I started work on tracking client info on server.hh (which defines a struct to store client info). We want to threads to read from and write to this struct safely, so we need a mutex and condition variable.

TODOs:
- resolve the bugs mentioned above
- keep working on the client info implementation

Misc:
- Karly's IP: 10.250.109.84
- Catherine's IP: 10.250.69.72
- Shell command to get your IP: ipconfig getifaddr en0

Useful links:
- "Client-Server chat in C++ using sockets": https://simpledevcode.wordpress.com/2016/06/16/client-server-chat-in-c-using-sockets/
- "Design a concurrent server for handling multiple clients using fork()" GeeksForGeeks article: https://www.geeksforgeeks.org/design-a-concurrent-server-for-handling-multiple-clients-using-fork/
- (probably useful down the line) "Handling multiple clients on server with multithreading using Socket Programming in C/C++": https://www.geeksforgeeks.org/handling-multiple-clients-on-server-with-multithreading-using-socket-programming-in-c-cpp/
- (not yet used but could be relevant) "Socket Programming in C/C++: Handling multiple clients on server without multi threading": https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/

Feb 13, 2023 morning
=============================================================================
Progress:
- changed the server so that it still runs (and can accept future clients) even when all current clients have exited
- properly updating the number of actively connected clients (i.e., decrementing client count when a client exits)
- typing "exit" into the server does not necessarily shut down the server, though...but Ctrl+C handles this okay
- updated server socket settings through the line setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    - this allows for reuse of local addresses during bind()
    - so that we can use the same port multiple times in the same session

Bugs:
- "Error binding socket to local address" for port 6000 (could be a firewall issue)
- occasionally, we still get an "Error binding socket to local address" error when trying to reuse some ports (e.g. port 3005 raised this issue)

Questions:
- Could we possibly use the c++ select() linux command? This command allows us to monitor multiple file descriptors, waiting until one of the fd's become active. In our case, we can notify the client socket when server chooses to send a message to that particular client.
    - The next best alternative to select() is to create a new thread for every new client connected to the server, but this is difficult to debug...
- How do we reuse the same server port multiple times in the same session? setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, ..., ...); doesn't always work

Feb 13, 2023 afternoon
=============================================================================
Progress:
- put the client/server fork implementation in server_fork.cc and client_fork.cc
- started new implementation using c++ select() linux command in server.cc and client.cc, using https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/

Feb 13, 2023 evening
=============================================================================
Progress:
- we learned that in this client/server model, clients are supposed to send EACH OTHER messages. The server is just a middle man that relays client messages to their desired recipients. To that end, I adjusted implementation so that client 0 can send messages to itself without server intervention (i.e., it can talk to itself)
    - I intended to write a program that has all messages (from all clients) sent to client 0

Bugs:
- two clients were able to connect, but the second client's message wasn't going anywhere (not to the server, nor to client 0)
- I suspect it's a while loop issue, maybe get rid of line 161 on server2.cc? Either way, I'll look at https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/ in more detail

Feb 14, 2023 morning
=============================================================================
Progress:
- server (server2.cc) now supports having each client talk to themselves
- server no longer runs into an infinite loop after client quits, as long as client quits properly ("exit" rather than Ctrl+C)
    - we can probably modify implementation so that the proper way for client to quit is Ctrl+C

Helpful link about recv() vs. read(): https://stackoverflow.com/questions/1790750/what-is-the-difference-between-read-and-recv-and-between-send-and-write

Bugs:
- client currently can't receive any incoming messages until after they send their current message

Next steps:
- have client be able to specify which recipient (client) to send to, and have server correctly send message to the correct client
- have client be able to receive (by listening for) incoming messages at any moment

Feb 15, 2023
=============================================================================
Progress:
- realized that clients cannot listen for new messages at the same time as sending existing messages , and that threading will fix this issue
- implemented threading, where one client process listens to messages from the server, and another process handles sending messages to the server

Bugs:
- only implemented message sending for client 0 -> client 1 and vice versa. We need to be able to have each client specify who to send each message to

Feb 18, 2023
=============================================================================
Progress:
- have clients send a 3-part message: operation, recipient username, and message (each one on a new line)
    - the program properly prompts the client to do this
    - operations: 1 is "send to a user," 2 is "list accounts," 3 is "delete account"
- have server parse this operation + '\n' + username + '\n' + message string, and only send the message part over
- implemented updating the client_table unordered map: std::unordered_map<std::string, int> 
- implemented having client specify their own username on the command line, and server reading this and adding the username to client_table to 
- implemented operation 1: having the server send the right messages to the right recipients (assuming the specified recipient client username exists)
- implemented catching the error where the client wants to send to a recipient username that does not exist 

Bugs:
- hit quite a few bugs in this work session (mostly relating to infinite loops and how to avoid them)
- forgot to track down each issue individually, but they mostly involved not breaking out of while loops or closing sockets properly

TODOs:
- handle account deletion, which is operation 3
- implement accounts listing, which is operation 2 (including wildcard matching)
- handle errors in client input (for example, if operations are not "1", "2", or "3")

Feb 18, 2023 evening
===============================================================================
Progress:
- implemented listing usernames by wildcard matching
- added table for logged out users
- implemented saving messages for logged out users

TODOs:
- implement detection (in the server) of who sent each particular message
- upon client login, differentiate between new user and login of existing user
  (can check client list)
    - if existing login then need to deliver any messages during its
  time logged off
- upon login: check if same username is active in another process and terminate that other process
    - one way this might work is to call std::terminate() inside the client listener thread
- handle the server going down (e.g., Ctrl+C) situation gracefully (right now the client prints out "Server:\n" in an infinite loop)

Feb 19, 2023 afternoon
===============================================================================
Progress:
- implemented detection (in the server) of who sent each particular message
- implemented sending of any undelivered/queued messages to user (if they have logged in previously)
- implemented telling each client who sent each message to it
- tested out the sending of any undelivered/queued messages to user (if they have logged in previously)
- implemented account deletion

TODOs:
- account deletion needs to happen more naturally: right now, the client who wants to delete account must still input a message
- implement logging out an existing login (if the same user logs in twice on two different terminals)
- start on grpc

Feb 19, 2023 late evening
===============================================================================
Progress:
- handled account deleting more naturally (the account is deleted right as client specifies operation=3 instead of the client needing to specify a message to send, since that doesn't make sense)
- installed gRPC and tested out basic client/server framework on this tutorial page: https://grpc.io/docs/languages/cpp/quickstart/#install-grpc
- designed proto for gRPC implementation, see grpc_chat_github/protos/chat.proto

TODOs:
- continue gRPC 
- implement logging out an existing login (if the same user logs in twice on two different terminals)
- handle the server going down (e.g., Ctrl+C) situation gracefully (right now the client prints out "Server:\n" in an infinite loop)
    - this might involve the need to read SIGINT signals: https://www.tutorialspoint.com/how-do-i-catch-a-ctrlplusc-event-in-cplusplus
    
Feb 19, 2023 late evening (2)
===============================================================================
Progress:
- followed a gRPC tutorial on how to build and locally install gRPC and Protocol Buffers
- ran into issues compiling protobufs with cmake (and make)
- tried using bazel and BUILD and WORKSPACE files, but ran into issues there too

Bugs:
- cmake issue compiling protobuf: the grpc_cpp_plugin was not recognized as executable 
    - something like this: https://github.com/faaxm/exmpl-cmake-grpc/issues/1
- bazel issue: in blacklisted_protos attribute of proto_lang_toolchain rule @com_google_protobuf//:cc_toolchain: '@com_google_protobuf//:well_known_protos' does not have mandatory providers: 'ProtoInfo'. Since this rule was created by the macro 'proto_lang_toolchain', the error might have been caused by the macro implementation

Feb 20, 2023 at like 5am
===============================================================================
Progress:
- got gRPC client and server running! although none of it is implemented yet (besides an "#include "chat.grpc.pb.h" and a main function that prints hello world)
- for the user: follow this tutorial (https://grpc.io/docs/languages/cpp/quickstart/#install-grpc)
series of terminal commands to do (from the base directory in the repository, which is grpc_chat in our case):
    - `$ export MY_INSTALL_DIR=$HOME/.local`
    - `$ mkdir -p $MY_INSTALL_DIR`
    - `$ export PATH="$MY_INSTALL_DIR/bin:$PATH"`
    - `$ brew install cmake` (mac)
        - or `$ sudo apt install -y cmake` (linux)
    - `$ cmake --version`
    - `$ brew install autoconf automake libtool pkg-config`
    - `$ mkdir -p cmake/build`
    - `$ pushd cmake/build`
    - `$ cmake -DgRPC_INSTALL=ON `
      `-DgRPC_BUILD_TESTS=OFF`
      `-DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR`
      `../..`
    - `$ make -j 4 `
        - (or change to a bigger number if you want to run with more jobs in parallel)
    - `$ make install`
to run the client and server (note that you should currently be in the build (i.e., repo/cmake/build/ directory):
    - `$ src/chat_client`
    - `$ src/server`

Feb 20, 2023 at like 9am
===============================================================================
Progress:
- implemented gRPC client (but haven't tested)
- updated chat.proto design

Todo:
- implemented gRPC server and test


Feb 20, 2023 afternoon and evening
===============================================================================
Progress:
- started gRPC server implementation
- fixed a segmentation fault issue that comes when more than one client connects to the server and tries to message each other
- the listaccounts functionality works properly now
- handle account deletion (sort of...)

Bugs:
- right now, the server throws a segmentation fault when you run src/client cat, log cat out, and then (in the same terminal window) log another user (or cat herself) back in
    - the server hears the account login and deletion okay
    - it's just that the NEXT time the client program is run on the same terminal window, the server seg faults
- right now, the program doesn't properly handle what happens when the same user logs in twice on two separate windows
    - the correct message is sent to the client, but the client isn't prompted to log in with a different username

Todo:
- address this seg fault somehow 
    - I think the stream->WriteLast function here might help? https://grpc.github.io/grpc/cpp/classgrpc_1_1_write_options.html
        - put this in the server code
        

Feb 20, 2023 late night
===============================================================================
Progress:
- fixed the seg fault put in bugs -- basically, my Session* pointer in server.cc was being deleted from the server program memory once a client logs out
    - so the line session->join(stream, username, true); is problematic because we try to access a session object that does not exist in memory
    - fixed it by making Session* session a global macro that does not get initialized with new Session(); until the main function runs
- implemented logging out a user

TODO and bugs:
- the implementation of logging out a user when a new person with the same username logs in is not really robust -- the client (whose login is booted) does not automatically exit out of the program
    - so we need to fix this
- in gRPC, implement a logged out user's messages getting saved
- gRPC documentation and test

Feb 21, 2023 morning
===============================================================================
Progress:
- in the wire protocol, implemented handling of two cases: (1) logging out an existing user if someone else logs in with their name, (2) having the client gracefully exit if server goes down due to interrupt (e.g., Ctrl + C)

TODO: 
- the above can be done better -- right now, the client just listens to the following strings from the server
    (1) "Server shut down permaturely, so logging you out."
    (2) "Another user logged in as your name, so logging you out."
    but these strings could well be also sent as messages between clients...
- all the TODOs from yesterday

Feb 21, 2023 evening
===============================================================================
Progress:
- modularized wire protocol server so it's easier to test
- testing the modularized version of server to make sure that everything is functioning the same way it was before
    - [x] handling bad operation (an input that's not 1, 2, 3, or 4)
    - [x] sending message
    - [x] sending message to a user that does not exist (handling this error)
    - [x] saving a user's messages when they were gone
        - [x] sending the user these messages when they log back in
    - [x] listing accounts with no wildcard
    - [x] listing accounts with a wildcard
    - [x] logging out (and having accounts list reflect that accurately)
    - [x] deleting an account (and having accounts list reflect that accurately)
    - [x] logging in existing user when same user logs in on another client program
    - [x] client gracefully exits when server shuts down (e.g. due to Ctrl + C)

Feb 21, 2023 late evening 
===============================================================================
Progress:
- in gRPC, implemented a logged out user's messages getting saved, and sending the user these messages when they log back in
- gRPC documentation and test

Bugs:
- we ran into some issues using googletest and mocks to end-to-end test the server, but we are nonetheless continuing to try other ways to unit test
- we plan to make our server's StreamRequest handling (and StreamResponse creating) code more modular, so that we can unit test its individual components

Feb 22, 2023
===============================================================================
Progress:
- modularized gRPC functionality
- implemented unit tests for gRPC server