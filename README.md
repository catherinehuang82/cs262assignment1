# CS 262 Design Exercise 1: Wire Protocols

## Wire Protocol Usage and Functionality

### Compiling files:
Run `g++ -o server server2.cc --std=c++11` and `g++ -o client client.cc --std=c++11`.
This should provide the two executable files for everything below.

### Setting up the server:
On one terminal window, run `./server port` where an example port is `6000`. 
The server is running and waiting for clients to connect.

### Connecting a client to the server:
From another terminal window, run `./client ip port username` where `ip` should be the ip of the server, `port` is the one specified by the server, and `username` is your username. If the username has never been used before, an account will be created. If it has been used before, you will be logged into your account, and any messages received while logged out will be delivered.

Note: you should not log in with the same username on two clients at the same time.

You can find the server's ip address by reading from its terminal (for example, mine reads like
`karlyh@dhcp-10-250-200-218`, which means the ip is 10.2250.200.218).

### Communicating between clients & other functionality:
After each action, the server will prompt you for an operation. You can choose:
- 1: send message
    - then you'll be prompted for a username to send to
    - lastly, you'll be prompted for the message
    - if the user doesn't exist, you'll have to try again
- 2: list accounts
    - you'll be prompted for a wildcard
    - all accounts, active and logged out, containing the wildcard will be listed
- 3: delete account
    - you'll be prompted to hit enter to confirm deletion
- 4: log out

### Notes on expected functionality
- Up to 10 clients can connect to the server at a time
- A message can be up to 1500 chars long
- Messages can be sent to active and logged out users
- Upon login, any messages sent in your absence will be delivered
- If you delete your account, any messages you sent previously that have
  not yet been delivered will still be delivered

### Testing
We implemented unit tests with the help of GoogleTest. You can find installation instructions [here](https://google.github.io/googletest/quickstart-cmake.html) and introduction to writing GoogleTests [here](https://google.github.io/googletest/primer.html). 

## gRPC Usage and Functionality

### Installation and Setup Instructions

### Compiling Files

### Setting up and connecting to server

### Communication between clients & other functionality

### Notes on expected functionality

### Testing

### Performance observations
- Comparisons over complexity of code
- Performance differences
- Size of buffers being sent back and forth between client/server