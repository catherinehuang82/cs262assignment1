#include "chat.grpc.pb.h"
#include <string>
#include <thread>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using chat::Chat;
using chat::Message;
using chat::ListAccountsRequest;
using chat::DeleteAccountRequest;
using chat::LogoutRequest;
using chat::LoginRequest;
using chat::LoginResponse;
using chat::ListAccountsResponse;
using chat::DeleteAccountResponse;
using chat::LogoutResponse;
using chat::StreamRequest;
using chat::StreamResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class ChatClient
{

public:
	ChatClient(std::shared_ptr<Channel> channel, std::string user) : stub_(Chat::NewStub(channel)), username(user) {}

	StreamRequest createChatRequest(int type, std::string messageNew = "", std::string recipientUsername = "")
	{
		StreamRequest n;
		if (type == 1) {
			// login request
			// TODO: consider getting rid of this case and just parsing login info on command line
			LoginRequest *login_request = new LoginRequest();
			login_request->set_username(username);
			n.set_allocated_login_request(login_request);
		}
		else if (type == 2)
		{   // new message request
            Message *msg = new Message();
			msg->set_sender_username(username);
			msg->set_recipient_username(recipientUsername);
			msg->set_message(messageNew);
			n.set_allocated_message_request(msg);
		}
		else if (type == 3)
		{   // list accounts request
			ListAccountsRequest *listAccounts_request = new ListAccountsRequest();
			listAccounts_request->set_username(username);
            listAccounts_request->set_wildcard(messageNew);
			n.set_allocated_listaccounts_request(listAccounts_request);
		}
		else if (type == 4)
		{   // delete account request
			// TODO: implement!
            DeleteAccountRequest *deleteAccount_request = new DeleteAccountRequest();
            deleteAccount_request->set_username(username);
            n.set_allocated_deleteaccount_request(deleteAccount_request);
		}
		else
		{ // log out request
			LogoutRequest *logout_request = new LogoutRequest();
            logout_request->set_username(username);
            n.set_allocated_logout_request(logout_request);
		}
		return n;
	}

	void parse(std::string msg, std::vector<std::string> &ret)
	{
		std::stringstream temp(msg);
		std::string temp2;
		while (std::getline(temp, temp2, ' '))
			ret.push_back(temp2);
	}

	/*Client Requests*/
	void makeRequests(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)
	{

		// handle login
		stream->Write(createChatRequest(1, username));
		std::this_thread::sleep_for(std::chrono::seconds(2));

		std::cout << "\n\n";
		std::cout << "##################################\n";
		std::cout << "Welcome to Chat. Your username is " << username << ".\n";
		std::cout << "##################################\n\n";
		std::cout << "1) To send a new message, type 1 and press Enter.\n";
		std::cout << "2) To list accounts, type 2 and press Enter.\n";
		std::cout << "3) To delete your account, type 3 and press Enter.\n";
		std::cout << "4) To log out, type 4 and press Enter.\n\n";

		std::string msg;

		// Write messages to session
		termios oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		termios newt = oldt;
		newt.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		while (true)
		{
			std::cout << ">>  ";
			std::getline(std::cin, msg);
			// std::vector<std::string> ret;
			// parse(msg, ret);
			if (msg == "2")
			{
				std::cout << "Enter wildcard: >> ";
				std::string wildcard;
				std::getline(std::cin, wildcard);
				stream->Write(createChatRequest(3, wildcard));
				std::this_thread::sleep_for(std::chrono::seconds(2));
			}
			// if logging out...
			else if (msg == "4")
			{
				stream->Write(createChatRequest(5));
				break;
			}
			// if deleting account...
			else if (msg == "3")
			{
				stream->Write(createChatRequest(4));
				break;
			} else if (msg == "1") {
				// send messages
				std::cout << "Enter recipient username: >>  ";
				std::string recipient;
				std::getline(std::cin, recipient);
				std::cout << "Recipient username: " << recipient << std::endl;
				std::cout << "Enter message: >>  ";
				std::string message_body;
				std::getline(std::cin, message_body);
				std::cout << "Message body: " << message_body << std::endl;
				stream->Write(createChatRequest(2, message_body, recipient));
			} else {
				std::cout << "Invalid input. Please type one of the following.\n\n";
				std::cout << "1) To send a new message, type 1 and press Enter.\n";
				std::cout << "2) To list accounts, type 2 and press Enter.\n";
				std::cout << "3) To delete your account, type 3 and press Enter.\n";
				std::cout << "4) To log out, type 4 and press Enter.\n\n";
			}
		}
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		stream->WritesDone();
	}

	/*Server Responses*/
	void processResponses(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)
	{
		StreamResponse serverResponse;
		while (stream->Read(&serverResponse))
		{
			if (serverResponse.has_listaccounts_response())
			{
				// list accounts reponse
				std::cout << "\r(" << serverResponse.listaccounts_response().username() << " has requested to list accounts)\n";
				std::cout << "Accounts list:\n" + serverResponse.listaccounts_response().accounts() << std::endl;
			} else if (serverResponse.has_login_response())
			{ 
				// notify the user that they have logged in
				std::cout << "\r(" << serverResponse.login_response().message() << ")\n";
			}
			else if (serverResponse.has_logout_response())
			{
				// notify the user that they have logged out
				std::cout << "\r(" << serverResponse.logout_response().message() << ")\n";
			}
			else if (serverResponse.has_deleteaccount_response()) {
				std::cout << "\r(" << serverResponse.deleteaccount_response().message() << ")\n";
			}
			else if (serverResponse.has_message_response())
			{ 
				// send the incoming message to the recipient client
				std::cout << serverResponse.message_response().sender_username() << ": " << serverResponse.message_response().message() << "\n";
			}
			else
			{
				std::cout << "\r(Invalid response received from server)\n";
			}
		}
	}

	void Chat()
	{
		ClientContext context;
		std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream(stub_->ChatStream(&context));
		
		std::thread writer(&ChatClient::makeRequests, this, stream); // Separate thread to make Requests to Server.

		processResponses(stream); //Process Response from Server
		writer.join();
		Status status = stream->Finish();
		if (status.ok())
			std::cout << "(Client exited with OK status)" << std::endl;
		else
			std::cout << "(There was a server error)" << std::endl;
	}

private:
	std::unique_ptr<Chat::Stub> stub_;
	std::string username;
	std::string wildcard;
	std::atomic<bool> session_join_flag;
};

int main(int argc, char **argv)
{
	if(argc != 2)
    {
        std::cerr << "Usage: username" << std::endl;
		exit(0); 
    } //grab the IP address and port number 
    char *username_command_line = argv[1];
	std::string username = username_command_line;
	ChatClient newClient(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), username);
	newClient.Chat();
	return 0;
}
