# CS 262 Design Exercise 1: Wire Protocols

Below is a manual with setup instructions, functionality, and engineering decisions of both our wire protocol and gRPC implementations.
For a day-by-day tracker of our bugs and progress, see our [engineering notebook](https://github.com/catherinehuang82/cs262assignment1/blob/main/engineering_notebook.md).

# File Structure
```
cs262assignment1
│   README.md                       # setup instructions and explanations of design and engineering decisions
│   engineering_ledger.md           # engineering notebook keeping track of progress, bugs, and decisions  
│
└───wire_protocol
│   │   server.h                    # wire protocol server header file
│   │   server.cc                   # wire protocol server code
|   |   client.cc                   # wire protocol client code
|   |   test_wire_protocol.cc       # wire protocol chat unit tests
│   │   CMakeLists.txt              # makefile
│   
└───grpc_chat
    │   CMakeLists.txt              # makefile
    └───protos
    |   │   chat.proto              # proto definitions
    └───src
    |   │   server.h                    # gRPC server header file
    |   │   server.cc                   # gRPC server code
    |   │   client.cc                   # gRPC client code
    |   │   client.cc                   # gRPC client code
    |   │   server_test.cc              # gRPC chat unit tests
    └───cmake
    |   │   downloadproject-template.cmake  # template makefile
    |   │   downloadproject.cmake           # template makefile
```

# Wire Protocol Usage and Functionality

## Compiling files
Run `g++ -o server server.cc --std=c++11` and `g++ -o client client.cc`.
This should provide the two executable files for everything below.

## Setting up the server
On one terminal window, run `./server port` where `port` is any four-digit number above 1024. An example port is `6000`. 
The server is running and waiting for clients to connect.

## Connecting a client to the server
From another terminal window, run `./client ip port username` where `ip` should be the ip of the server, `port` is the one specified by the server, and `username` is your username. If the username has never been used before, an account will be created. If it has been used before, you will be logged into your account, and any messages received while logged out will be delivered. In other words, _logging in_ and _creating your account_ happen through the same process.

Note: you should not log in with the same username on two clients at the same time.

You can find the server's ip address by reading from its terminal (for example, mine reads like
`karlyh@dhcp-10-250-200-218`, which means the ip is 10.2250.200.218). You can also run the terminal command `ipconfig getifaddr en0`.

## Communicating between clients & other functionality
After each action (and at the beginning of a session), the server will prompt you for an operation. 
You can choose:
- 1: send message
    - then you'll be prompted for a username to send to
    - lastly, you'll be prompted for the message
    - if the user doesn't exist, you'll have to try again from the operation step
- 2: list accounts
    - you'll be prompted for a wildcard
    - all accounts, both active and logged out, containing the wildcard (i.e. substring) will be listed
- 3: delete account
    - your account will automatically be deleted from the system and from server records
    - your process will terminate with a specified status (generally "OK status")
- 4: log out
    - you will be logged out, but your account stays in the system and in server records
    - the server will queue any messages sent to you while you were away, and send you these messages upon your next login
    - your process will terminate with a specified status (generally "OK status")
If you do not specify 1, 2, 3, or 4 as an operation, you will be asked to try again

## Notes on expected functionality
- Up to 10 clients can connect to the server at a time
- A message can be up to 1500 chars long
- Messages can be sent to both active and logged out users
- Any messages sent to a logged out client will be queued...
- ...and upon the next login, any messages sent in a client's absence will be delivered to them
- If you delete your account, any messages you sent (to other clients) previously that have
  not yet been delivered will still be delivered to them
    - It is up to the client to detect that you deleted your account, either through listing accounts or through erroneously sending a message (to your deleted username)
- Recall that account creation and login are the same process. However, usernames are unique, and if an existing client session already exists with the username I specify, that existing client session gets logged out, and my new session becomes active in that session's place. Any messages that the previous login sent to any logged out clients are still queued.
- When the server abnormally shuts down (e.g. through Ctrl+c), all remaining client programs also shut down after displaying the message "Server shut down permaturely, so logging you out."
- If the client connects before the server, or the client has the incorrect IP address or port, (or for whatever reason the socket does not bind successfully), then the client receives an "Error connecting to socket" from the server

## Engineering Decisions
We used the `select()` Linux call to manage multiple client socket descriptors, so that multiple clients can connect to the server and talk to one another at the same time. `select()` is effective because it allows multiple processes to run concurrently (accessing shared memory) without the need for a new thread per client. Since `select()` operates on a fixed-size `fd_set client_fds` (a set of file descriptors), we limited the number of clients connecting at the same time to 10 to account for this. (If we wanted to, we could have increased this number to, say, 100+. The implementation does not change.) 

In our array of client socket descriptors, we can listen for activity on all of the sockets at the same time with a combination of (1) `FD_ISSET(server_sd, &client_fds);` and (2) `activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);` until one of the client sockets get activity (e.g. an incoming message). Since we have a blocking implementation, we use while loops to listen.

We still use threading, though, on top of `select()`: in our client code, there is a second thread that listens for incoming server activity (while the main program handles user input). If a client program is forced to quit (either because of a forced log out by another login of the same username, or by abnormal server exit), the listener thread receives that and is responsible for alerting the main thread and having that thread quit as well.

## Testing
We implemented unit tests with the help of GoogleTest. You can find installation instructions [here](https://google.github.io/googletest/quickstart-cmake.html) and introduction to writing GoogleTests [here](https://google.github.io/googletest/primer.html). 

# gRPC Usage and Functionality

## Installation and Setup Instructions
Go to the `/grpc_chat` folder. The following gRPC setup process assumes you have Homebrew. If you do not, [install Homebrew here](https://brew.sh/). 

Run the following commands to install gRPC:
* `$ export MY_INSTALL_DIR=$HOME/.local`
* `$ mkdir -p $MY_INSTALL_DIR`
* `$ export PATH="$MY_INSTALL_DIR/bin:$PATH"`
* `$ brew install cmake` (mac)
    * or `$ sudo apt install -y cmake` (linux)
* `$ cmake --version` (check your version; you need version 3.5.1 or later.)
* `$ brew install autoconf automake libtool pkg-config` (mac) (install other tools required to build gRPC)
    * or `$ sudo apt install -y build-essential autoconf libtool pkg-config` (linux)

Run the following commands to build and locally compile gRPC and Protocol Buffers:
* `$ mkdir -p cmake/build`
* `$ pushd cmake/build`
* `$ cmake -DgRPC_INSTALL=ON `* `-DgRPC_BUILD_TESTS=OFF`
    `-DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR`
    `../..`
* `$ make -j 4 `
    - (or change to a bigger number if you want to run with more jobs in parallel)
* `$ make install`

To install a more recent version of `cmake` using the instructions [here](https://cmake.org/install/).

These commands above are highly similar to the commands in [this gRPC quick start tutorial](https://grpc.io/docs/languages/cpp/quickstart/#install-grpc)—however, without the part about running the gRPC example..

## Compiling Files
After you finish running the above, you should be in the `grpc_chat/cmake/build/` directory (`build/` directory). In that directory are executables `src/server` and `src/client`.
* To run the server, type `src/server`.
* To run the client, type `src/client <username>`. If you forget to specify a username, you get a `"Usage: username"` error message back.

## Communication between clients & other functionality
The gRPC-based application client experience is similar to that of the wire protocol application.

After each action (and at the beginning of a session), the server will prompt you for an operation. The interface looks like this:

```
(You have successfully logged in.)

##################################
Welcome to Chat. Your username is cat.
##################################

1) To send a new message, type 1 and press Enter.
2) To list accounts, type 2 and press Enter.
3) To delete your account, type 3 and press Enter.
4) To log out, type 4 and press Enter.

>>
```

You can choose:
- 1: send message
    - then you'll be prompted for a username to send to
    - lastly, you'll be prompted for the message
    - if the user doesn't exist, you'll have to try again from the operation step
- 2: list accounts
    - you'll be prompted for a wildcard
    - all accounts, both active and logged out, containing the wildcard (i.e. substring) will be listed
- 3: delete account
    - your account will automatically be deleted from the system and from server records
    - your process will terminate after a status message is sent back to you (generally `"(You have successfully deleted your account.)"`)
- 4: log out
    - you will be logged out, but your account stays in the system and in server records
    - the server will queue any messages sent to you while you were away, and send you these messages upon your next login
    - your process will terminate after a status message is sent back to you (generally `"(You have successfully logged out.)"`)
If you do not specify 1, 2, 3, or 4 as an operation, you will be asked to try again. 

Note that you need to wait 2 seconds before the end of one operation to begin the next.

## Notes on expected functionality
- There is no limit to how many clients can connect to the server at a time
- A message has no charater limit
- Messages can be sent to both active and logged out users
- Any messages sent to a logged out client will be queued...
- ...and upon the next login, any messages sent in a client's absence will be delivered to them
- If you delete your account, any messages you sent (to other clients) previously that have
  not yet been delivered will still be delivered to them
    - It is up to the client to detect that you deleted your account, either through listing accounts or through erroneously sending a message (to your deleted username)
- Recall that account creation and login are the same process. However, usernames are unique, and if an existing client session already exists with the username I specify, that existing client session gets logged out, and my new session becomes active in that session's place. 
    - Any messages that the previous login sent to any logged out clients are still queued to their recipients.

# Engineering Decisions
Our `chat.proto` Protocol Buffer design is below. In our implementation, the server is responsible for handling StreamRequests that come from the client, and sending StreamResponses back to the appropriate clients. The client is responsible for sending StreamRequests to the server (upon and based on user input), and responding to StreamResponses that come from the server (either by printing received messages to the console, exiting the program, or more).

From the Protocol Buffer perpsective, each StreamRequest and StreamResponse is one of several types, depending on the operation (1-4) that the client desires. Each StreamRequest contains at least a username (and possibly other fields, such as a wildcard for accounts listing). The cares about the message to know which user triggered the action. In the case of logging a user out or deleting an account, the server wants to record that a certain username is no longer active or no longer in existence in the system. In the case of a chat message, the sender wants to be able to tell the recipient client who sent them the message.

Each StreamResponse generally contains a username and a message. The username is so that the server knows who to send the response to--in the case of a message, that would be whoever is on the receiving end of that message. In the case of listing accounts, that would be whoever requested to list accounts. The message field is there either to send information (a chat message body or a list of accounts), or to tell the client that they have successfully logged out or deleted their account.


```
syntax = "proto3";

package chat;

service Chat {
	rpc ChatStream(stream StreamRequest) returns  (stream StreamResponse) {}
}

message StreamRequest {
    oneof RequestEvent {
        LoginRequest login_request = 1;
		Message message_request  = 2;
        ListAccountsRequest listAccounts_request = 3;
        DeleteAccountRequest deleteAccount_request = 4;
        LogoutRequest logout_request = 5;
    }
}

message StreamResponse {
     oneof ResponseEvent {
        LoginResponse login_response = 1;
        Message message_response = 2;
		ListAccountsResponse listAccounts_response = 3;
        DeleteAccountResponse deleteAccount_response  = 4;
        LogoutResponse logout_response = 5;
    }
}

message Message {
    string sender_username = 1;
    string recipient_username = 2;
    string message = 3;
}

message MessageList {
    repeated Message messages = 1;
}

message ListAccountsRequest {
    string username = 1;
    string wildcard = 2;
}

message DeleteAccountRequest {
    string username = 1;
}

message LogoutRequest {
    string username = 1;
}

message LoginRequest {
    string username = 1;
}

message LoginResponse {
    string username = 1;
    string message = 2;
}

message ListAccountsResponse {
    string username = 1;
    string accounts = 2;
}

message DeleteAccountResponse {
    // delete account message
    string username = 1;
    string message = 2;
}

message LogoutResponse {
    // logout message
    string username = 1;
    string message = 2;
}
```

Our client has three main methods, run in two different threads: 
* a one-time `StreamRequest createChatRequest(int type, std::string messageNew = "", std::string recipientUsername = "")`
* a continual `void makeRequests(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)`, which calls `createChatRequest` (run in separate thread)
* a continual `void processResponses(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)`. (run in main thread)

Our server does its job mostly through the `class ChatImpl final : public Chat::Service` inherited class. There is a `Session` class that the server instantiates once (used by all client streams), which serves helper functions to help the server process various types of client requests.

In the `Session` class, we store (1) a mapping of active clients to their streams, (2) a mapping of logged out users to a `MessageList` with a repeated `Message` field, storing the messages they received while they were gone, and (3) a set of both active and inactive usernames. We also keep track of the number of active users. We modify these storage data structures and values thread-safely, wrapping write operations with a `mutex` that is a member of the `Session` class.

## Testing

## Performance observations
- Comparisons over complexity of code
- Performance differences
- Size of buffers being sent back and forth between client/server
