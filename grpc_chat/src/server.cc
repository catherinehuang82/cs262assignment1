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
#include "chat.grpc.pb.h"

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

	std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> d_allStreams;
	std::mutex d_mutex;
	std::string d_id;
	int d_userCount;

public:
	Session(const std::string &id) : d_id(id), d_mutex(), d_userCount(0) {}

	// Return a modifiable reference to the session id.
	std::string &id()
	{
		return d_id;
	}

	// Return a non-modifiable reference to the sessions id.
	const std::string &id() const
	{
		return d_id;
	}

	void broadcast(StreamResponse message)
	{
		for (auto each : d_allStreams)
		{
			(each.second)->Write(message);
		}
	}

	void join(ServerReaderWriter<StreamResponse, StreamRequest> *stream, const std::string &username, bool isNewSession = false)
	{
		StreamResponse responseMessage;
		if (isNewSession)
		{
			d_allStreams[username] = stream;
			++d_userCount;
			return;
		}

		if (d_allStreams.find(username) != d_allStreams.end())
		{
			JoinSessionResponse *join_session_response = new JoinSessionResponse();
			join_session_response->set_message("Username " + username + " is already taken.");
			responseMessage.set_allocated_joinsession_response(join_session_response);
			stream->Write(responseMessage);
		}
		else
		{
			std::lock_guard<std::mutex> lg(d_mutex);
			LoginResponse *newLogin = new LoginResponse();
			newLogin->set_username(username);
			d_allStreams[username] = stream;
			responseMessage.set_allocated_login_response(newLogin);
			++d_userCount;
			broadcast(responseMessage);
		}
	}

	void leave(const std::string &username)
	{
		std::lock_guard<std::mutex> lg(d_mutex);
		--d_userCount;
	}

	int userCount()
	{
		return d_userCount;
	}
};

class ChatImpl final : public Chat::Service
{

	std::unordered_map<std::string, std::unique_ptr<Session>> d_sessions;
	std::mutex d_mutex;

public:
	Status ChatStream(ServerContext *context, ServerReaderWriter<StreamResponse, StreamRequest> *stream) override
	{
		StreamRequest message;

		while (stream->Read(&message))
		{
			StreamResponse responseMessage;
			std::string id = "";
			std::string username = "";
			if (message.has_login_request())
			{
				std::cout << "Login request received\n";

				username = message.login_request().username();

				d_mutex.lock();
				while (true)
				{
					id = generateRandomId();
					if (d_sessions.find(id) == d_sessions.end())
					{
						break;
					}
				}
				std::unique_ptr<Session> newSession(new Session(id));
				d_sessions[id] = std::move(newSession);
				d_mutex.unlock();

				StartSessionResponse *start_session_response = new StartSessionResponse();
				start_session_response->set_session_id(id);
				responseMessage.set_allocated_startsession_response(start_session_response);
				d_sessions[id]->join(stream, username, true);
				d_sessions[id]->broadcast(responseMessage);
			}
			else if (message.has_joinsession_request())
			{
				std::cout << "Join session request received\n";

				id = message.joinsession_request().session_id();
				username = message.joinsession_request().username();

				auto session = d_sessions.find(id);
				if (session != d_sessions.end())
				{
					session->second->join(stream, username);
					continue;
				}
				JoinSessionResponse *join_session_response = new JoinSessionResponse();
				join_session_response->set_message(id + " is not a valid session_id.");
				responseMessage.set_allocated_joinsession_response(join_session_response);
				stream->Write(responseMessage);
			}
			else if (message.has_message_request())
			{
				std::cout << "New message received\n";
				username = message.message_request().username();

				Message *newMessage = new Message();
				newMessage->set_username(username);
				newMessage->set_message(message.message_request().message());
				responseMessage.set_allocated_message_response(newMessage);

				auto session = d_sessions.find(id);
				if (session == d_sessions.end())
				{
					continue;
				}
				session->second->broadcast(responseMessage);
			}
			else if (message.has_logout_request())
			{
				std::cout << "Logout request received\n";

				// id = message.quitsession_request().session_id();
				username = message.logout_request().username();

				LogoutResponse *newLogout = new LogoutResponse;
				newLogout->set_username(message.logout_request().username());
				responseMessage.set_allocated_logout_response(newLogout);

				auto session = d_sessions.find(id);
				if (session == d_sessions.end())
				{
					continue;
				}
				session->second->leave(username);
				if (session->second->userCount() == 0)
				{
					std::cout << "Erasing session: " << session->second->id() << std::endl;
					d_sessions.erase(session->second->id());
					continue;
				}
				session->second->broadcast(responseMessage);
			}
			else
			{
				std::cout << "Unsupported message type\n";
			}
		}

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
