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
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

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
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

const std::string SERVER_ADDRESS = "0.0.0.0:50051";

// A object of this class represents a chat session.
// in our implementation, there is only one chat session where all repicients participate
class Session
{
    // perhaps? maybe we need one chat stream per username though
	// std::shared_ptr<std::string, ServerReaderWriter<StreamResponse, StreamRequest>> chat_stream;
	std::mutex d_mutex;
	// std::string d_user;

public:
    // session constructor
    // we no longer need a session ID because we assume only one session
	Session() :  d_mutex(), d_userCount(0) {
    }

    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> d_allStreams;
    std::set<std::string> allAccounts;
    int d_userCount;

	void broadcast(StreamResponse message)
	{
		for (auto each : d_allStreams)
		{
			(each.second)->Write(message);
		}
	}

    void send_message(std::string recipient_username, StreamResponse message) {
        d_allStreams[recipient_username]->Write(message);
    }

	void join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username, bool isNewSession = false)
	{
        // the isNewSession argument is unused!
		StreamResponse responseMessage;

		if (d_allStreams.find(username) != d_allStreams.end())
        // if user is already logged in
		{
			LoginResponse *failed_login_response = new LoginResponse();
			failed_login_response->set_message("Username " + username + " is already logged in.");
			responseMessage.set_allocated_login_response(failed_login_response);
			stream->Write(responseMessage);
		}
		else
		{
            std::cout << "Successful login for user: " << username << std::endl;
			// std::lock_guard<std::mutex> lg(d_mutex);
			LoginResponse *newLogin = new LoginResponse();
			newLogin->set_username(username);

            // "record" this stream in the allStreams dictionary
            allAccounts.insert(username);
            d_allStreams[username] = stream;

            // for debugging purposes
            std::cout << "Accounts list: " << std::endl;
            for (std::string a: allAccounts) {
                std::cout << a << std::endl;
            }

            newLogin->set_message("You have successfully logged in.");
			responseMessage.set_allocated_login_response(newLogin);

            // tell client they have successfully logged in
            stream->Write(responseMessage);
            std::cout << "Debug 2" << std::endl;
			++d_userCount;
			// broadcast(responseMessage);
            std::cout << "Debug 3" << std::endl;
		}
	}

	void leave(const std::string &username)
	{
		// std::lock_guard<std::mutex> lg(d_mutex);
		--d_userCount;
	}

	int userCount()
	{
		return d_userCount;
	}
};

class ChatImpl final : public Chat::Service
{
	// std::unordered_map<std::string, std::unique_ptr<Session>> d_sessions;
    Session *session = new Session();
	std::mutex d_mutex;

public:
	Status ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) override
	{
        // std::unique_ptr<Session> newSession(new Session());
		// session = std::move(newSession);
		StreamRequest message;

		while (stream->Read(&message))
		{
			StreamResponse responseMessage;
			// std::string id = "";
			std::string username = "";
			if (message.has_login_request())
			{
				std::cout << "Login request received\n";

				username = message.login_request().username();

				d_mutex.lock();

                session->join(stream, username, true);
                d_mutex.unlock();
			}
			else if (message.has_listaccounts_request())
			{
				std::cout << "List accounts request received\n";

				// id = message.joinsession_request().session_id();
				username = message.listaccounts_request().username();
                std::string wildcard = message.listaccounts_request().wildcard();

                std::string matching_accounts;
                // iterate through keys (usernames)
                for (auto it = session->d_allStreams.begin(); it != session->d_allStreams.end(); ++it)
                {
                    std::string a = it->first;
                    // if a username contains wildcard, add to string
                    if (a.find(wildcard) != std::string::npos) {
                        matching_accounts += a;
                        matching_accounts += '\n';
                    }
                }
                // accounts_list_str.pop_back();
                printf("listing accounts...\n");

                d_mutex.lock();
                // send accounts list to client that requested it
                ListAccountsResponse *listAccounts_response = new ListAccountsResponse();
                listAccounts_response->set_username(username);
                listAccounts_response->set_accounts(matching_accounts);

				responseMessage.set_allocated_listaccounts_response(listAccounts_response);
				stream->Write(responseMessage);
                d_mutex.unlock();
			}
			else if (message.has_message_request())
			{
				std::cout << "New message received. ";
				username = message.message_request().sender_username();
                std::string recipient_username = message.message_request().recipient_username();

                std::string message_string = message.message_request().message();

                d_mutex.lock();
				Message *newMessage = new Message();
				newMessage->set_sender_username(username);
                std::cout << "Sender: " << username;
                std::cout << ". Intended recipient: " << recipient_username;
				
                if (session->d_allStreams.find(recipient_username) != session->d_allStreams.end()) {
                    newMessage->set_recipient_username(recipient_username);
                    newMessage->set_message(message_string);
                    std::cout << ". Message: " << message_string << std::endl;
				    responseMessage.set_allocated_message_response(newMessage);
                    session->d_allStreams[recipient_username]->Write(responseMessage);
                } else {
                    // throw an error if sender inputs a recipient username that does not exist
                    newMessage->set_sender_username("Server");
                    newMessage->set_message("The recipient you specified does not exist. Please try again.");
                    // end the error message back to the client itself
                    responseMessage.set_allocated_message_response(newMessage);
                    stream->Write(responseMessage);
                }
                d_mutex.unlock();
                std::cout << "Debug 1" << std::endl;
                std::cout << "Debug 4" << std::endl;
			}
			else if (message.has_logout_request())
			{
                // user to be logged out
				username = message.logout_request().username();

                d_mutex.lock();
				LogoutResponse *newLogout = new LogoutResponse;
				newLogout->set_username(username);
                newLogout->set_message("You have successfully logged out.");
				responseMessage.set_allocated_logout_response(newLogout);

				session->leave(username);

                std::cout << "Logout request received from " << username << ". Now, there are " << session->d_userCount << " users active." << std::endl;
				// session->broadcast(responseMessage);
                stream->Write(responseMessage);

                d_mutex.unlock();

                // for debugging purposes
                std::cout << "Accounts list: " << std::endl;
                for (std::string a: session->allAccounts) {
                    std::cout << a << "\n";
                }
			}
            else if (message.has_deleteaccount_request())
			{
                // user to be logged out
				username = message.deleteaccount_request().username();

                d_mutex.lock();
				DeleteAccountResponse *newDeleteAccount = new DeleteAccountResponse;
				newDeleteAccount->set_username(username);
                newDeleteAccount->set_message("You have successfully deleted your account.");
				responseMessage.set_allocated_deleteaccount_response(newDeleteAccount);

                session->d_allStreams.erase(username);
                session->allAccounts.erase(username);

				session->leave(username);

                std::cout << "Delete account request received from " << username << ". Now, there are " << session->d_userCount << " users active." << std::endl;
				// session->broadcast(responseMessage);
                stream->Write(responseMessage);
                d_mutex.unlock();

                // for debugging purposes
                std::cout << "Accounts list: " << std::endl;
                for (std::string a: session->allAccounts) {
                    std::cout << a << "\n";
                }
			}
			else
			{
				std::cout << "Unsupported message type\n";
			}
		}

        delete session;
		return Status::OK;
	}

private:
	std::string generateRandomId()
	{
		std::string generatedStr = "";
		int i = 0;
		while (++i <= 8)
		{
			generatedStr.append(std::to_string(rand() % 10));
		}
		return generatedStr;
	}
};

void runServer()
{
	ChatImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << SERVER_ADDRESS << std::endl;
	server->Wait();
}

int main(int argc, char **argv)
{
	std::srand(std::time(nullptr)); // Use current time as seed for random number generator.
	runServer();
	return 0;
}
