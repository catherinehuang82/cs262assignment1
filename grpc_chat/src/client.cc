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
using chat::Login;
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

// class ChatClient
// {

// public:
// 	ChatClient(std::shared_ptr<Channel> channel) : stub_(Chat::NewStub(channel)), session_id("0"), session_join_flag(false) {}

// 	StreamRequest createChatRequest(int type, std::string messageNew = "")
// 	{
// 		StreamRequest n;
// 		if (type == 1)
// 		{   // new message request
//             Message *msg = new Message();
// 			msg->set_username(username);
// 			msg->set_message(messageNew);
// 			n.set_allocated_message_request(msg);
// 		}
// 		else if (type == 2)
// 		{   // list accounts request
// 			ListAccountsRequest *listAccounts_request = new ListAccountsRequest();
// 			listAccounts_request->set_username(username);
//             listAccounts_request->set_wildcard(messageNew);
// 			n.set_allocated_listaccounts_request(listAccounts_request);
// 		}
// 		else if (type == 3)
// 		{   // delete account request
// 			// TODO: implement!
//             DeleteAccountRequest *deleteAccount_request = new DeleteAccountRequest();
//             deleteAccount_request->set_username(username);
//             n.set_allocated_deleteaccount_request(deleteAccount_request);
// 		}
// 		else
// 		{ // log out request
// 			LogoutRequest *logout_request = new LogoutRequest();
//             logout_request->set_username(username);
//             n.set_allocated_logout_request(logout_request);
// 		}
// 		return n;
// 	}

// 	void parse(std::string msg, std::vector<std::string> &ret)
// 	{
// 		std::stringstream temp(msg);
// 		std::string temp2;
// 		while (std::getline(temp, temp2, ' '))
// 			ret.push_back(temp2);
// 	}

// 	/*Client Requests*/
// 	void makeRequests(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)
// 	{
// 		std::cout << "\n\n";
// 		std::cout << "                               ##################################\n";
// 		std::cout << "                               # Welcome to Asynchronous Chat.. #\n";
// 		std::cout << "                               ##################################\n\n";
// 		std::cout << "           1) To create a new session, type 'create <your_user_name>' and press Enter.\n";
// 		std::cout << "           2) To join an existing session, type 'join <session_id> <your_user_name>' and press Enter. \n";
// 		std::cout << "           3) To send a new message, type the message and press Enter.\n";
// 		std::cout << "           4) To quit the session, type 'quit' and press Enter.\n\n";

// 		std::string msg;
// 		//  Start or Join a session.
// 		while (true)
// 		{
// 			std::cout << ">>  ";
// 			std::getline(std::cin, msg);
// 			std::vector<std::string> ret;
// 			parse(msg, ret);
// 			if (ret.size() == 2 && ret[0] == "create")
// 			{
// 				username = ret[1];
// 				stream->Write(createChatRequest(1));
// 				std::this_thread::sleep_for(std::chrono::seconds(2));
// 				break;
// 			}
// 			else if (ret.size() == 3 && ret[0] == "join")
// 			{
// 				username = ret[2];
// 				session_id = ret[1];
// 				session_join_flag = true;
// 				stream->Write(createChatRequest(2));
// 				std::this_thread::sleep_for(std::chrono::seconds(2));
// 				if (session_join_flag)
// 					break;
// 			}
// 			else
// 				std::cout << "Incorrect command!\n";
// 		}

// 		// Write messages to session
// 		termios oldt;
// 		tcgetattr(STDIN_FILENO, &oldt);
// 		termios newt = oldt;
// 		newt.c_lflag &= ~ECHO;
// 		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
// 		while (true)
// 		{
// 			std::getline(std::cin, msg);
// 			if (msg == "quit")
// 			{
// 				stream->Write(createChatRequest(4));
// 				break;
// 			}
// 			stream->Write(createChatRequest(3, msg));
// 		}
// 		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
// 		stream->WritesDone();
// 	}

// 	/*Server Responses*/
// 	void processResponses(std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream)
// 	{
// 		StreamResponse serverResponse;
// 		while (stream->Read(&serverResponse))
// 		{
// 			if (serverResponse.has_startsession_response())
// 			{ //Start Session Response
// 				session_id = serverResponse.startsession_response().session_id();
// 				std::cout << "\r(Your session_id is " << session_id << ")\n";
// 			}
// 			else if (serverResponse.has_joinsession_response())
// 			{ //Join Session Response
// 				session_join_flag = false;
// 				std::cout << "\r(Could not connect to the session. " << serverResponse.joinsession_response().message() << " Try again)\n";
// 			}
// 			else if (serverResponse.has_login_response())
// 			{ //User Login Notification Response
// 				if (serverResponse.login_response().username() == username)
// 					std::cout << "\r("
// 							  << "You are now logged in)\n";
// 				else
// 					std::cout << "\r(" << serverResponse.login_response().username() << " has logged in)\n";
// 			}
// 			else if (serverResponse.has_logout_response())
// 			{ //User Logout Notification Response
// 				std::cout << "\r(" << serverResponse.logout_response().username() << " has logged out)\n";
// 			}
// 			else if (serverResponse.has_message_response())
// 			{ //New Message Notification Response
// 				std::cout << serverResponse.message_response().username() << ": " << serverResponse.message_response().message() << "\n";
// 			}
// 			else
// 			{
// 				std::cout << "\r(Invalid response received from server)\n";
// 			}
// 		}
// 	}

// 	void RouteChat()
// 	{
// 		ClientContext context;
// 		std::shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> stream(stub_->ChatStream(&context));
// 		std::thread writer(&ChatClient::makeRequests, this, stream); // Separate thread to make Requests to Server.

// 		processResponses(stream); //Process Response from Server
// 		writer.join();
// 		Status status = stream->Finish();
// 		if (status.ok())
// 			std::cout << "(You have successfully logged out)" << std::endl;
// 		else
// 			std::cout << "(There was a server error)" << std::endl;
// 	}

// private:
// 	std::unique_ptr<Chat::Stub> stub_;
// 	std::string username;
// 	std::string session_id;
// 	std::atomic<bool> session_join_flag;
// };

// int main(int argc, char **argv)
// {
// 	ChatClient newClient(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
// 	newClient.RouteChat();
// 	return 0;
// }

int main(int argc, char **argv)
{
	printf("Hello world!\n");
	return 0;
}