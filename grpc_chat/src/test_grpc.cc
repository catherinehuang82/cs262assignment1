#include <gtest/gtest.h>
#include <string>
#include "server.h"

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

TEST(GRPCTest, ListAccounts) {
    Session *session;
    std::string messageNew = "a";
    std::string recipientUsername = "karly";
    std::set<std::string> allAccounts = {"karly", "cat"};

    StreamRequest message = createChatRequest(2, messageNew, recipientUsername);
    ListAccountsResponse* response = session->listAccounts(message, allAccounts);

    EXPECT_EQ(response.username(), "karly");
    EXPECT_EQ(response.accounts(), "karly\ncat\n");
}

TEST(GRPCTest, DeleteAccount) {
    std::string messageNew = "";
    std::string recipientUsername = "karly";

    StreamResponse responseMessage;
    StreamRequest streamRequest;
    ServerReaderWriter<StreamResponse, StreamRequest> s = (responseMessage, streamRequest);

    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> d_allStreams = {{"karly", s}, {"cat", s}};
    std::set<std::string> allAccounts = {"karly", "cat"};

    StreamRequest message = createChatRequest(3, messageNew, recipientUsername);
    DeleteAccountResponse* response = deleteAccount(message, d_allStreams, allAccounts);

    std::unordered_map<std::string, ServerReaderWriter<StreamResponse, StreamRequest> *> expected_d_allStreams = {{"cat", s}};
    std::set<std::string> expected_allAccounts = {"cat"};

    EXPECT_EQ(response.message(), "You have successfully deleted your account.");
    EXPECT_EQ(d_allStreams, expected_d_allStreams);
    EXPECT_EQ(allAccounts, expected_allAccounts);
}
