#include <gtest/gtest.h>
#include <string>
#include "server.h"

// test message constructed to active user is correct
TEST(OperationTest, SendMessageToActiveUser) {
    string sender = "karly";
    string active_user = "cat";
    string message = "hi";
    int client_socket[2] = {0, 0};
    int bytesWritten = 0;
    std::unordered_map<std::string, int> active_users = {{"karly", 0}, {"cat", 1}};
    std::unordered_map<std::string, std::string> logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}};

    string msg = sendMessage(active_user, message, sender, client_socket, bytesWritten, active_users, logged_out_users, 0);

    EXPECT_EQ(msg, "karly: hi");
}

// test message constructed to logged out user is correct and message is saved for their return
TEST(OperationTest, SendMessageToLoggedOutUser) {
    string sender = "karly";
    string logged_out_user = "bubbles";
    string message = "hi";
    int client_socket[2] = {0, 0};
    int bytesWritten = 0;
    std::unordered_map<std::string, int> active_users = {{"karly", 0}, {"cat", 1}};
    std::unordered_map<std::string, std::string> logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}};

    string msg2 = sendMessage(logged_out_user, message, sender, client_socket, bytesWritten, active_users, logged_out_users, 0);
    std::unordered_map<std::string, std::string> expected_logged_out_users = {{"bubbles", "Messages you missed when you were gone:\nkarly: hi\n"}};
    
    EXPECT_EQ(msg2, "hi");
    EXPECT_EQ(logged_out_users, expected_logged_out_users);
}

// test whether the correct matching accounts are collected when listing accounts
TEST(OperationTest, ListAccounts) {
    std::string wildcard = "a";
    int client_socket[2] = {0, 0};
    std::set<std::string> account_set = {"cath", "karly", "bubbles"};
    int bytesWritten = 0;

    std::string result = listAccounts(wildcard, client_socket, bytesWritten, account_set, 0);

    EXPECT_EQ(result, "cath\nkarly\n");
}

// test whether the user is deleted from the set of accounts
TEST(OperationTest, DeleteAccount) {
    int client_socket[2] = {0, 0};
    string sender_username = "karly";
    std::unordered_map<std::string, int> active_users = {{"karly", 0}, {"cat", 1}};
    std::set<std::string> account_set = {"karly", "cat", "bubbles"};
    std::unordered_map<std::string, std::string> logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}};

    deleteAccount(0, client_socket, sender_username, active_users, account_set, logged_out_users, 0);

    std::unordered_map<std::string, int> expected_active_users = {{"cat", 1}};
    std::set<std::string> expected_account_set = {"cat", "bubbles"};
    std::unordered_map<std::string, std::string> expected_logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}};
    
    EXPECT_EQ(active_users, expected_active_users);
    EXPECT_EQ(account_set, expected_account_set);
    EXPECT_EQ(logged_out_users, expected_logged_out_users);
}

// test whether user is removed from active users and added to logged out users upon quitting
TEST(OperationTest, QuitUser) {
    int client_socket[2] = {0, 0};
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = 0;
    string sender_username = "karly";
    std::unordered_map<std::string, int> active_users = {{"karly", 0}, {"cat", 1}};
    std::unordered_map<std::string, std::string> logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}};

    quitUser(0, client_socket, newSockAddr, newSockAddrSize, sender_username, active_users, logged_out_users, 0);

    std::unordered_map<std::string, int> expected_active_users = {{"cat", 1}};
    std::unordered_map<std::string, std::string> expected_logged_out_users = {{"bubbles", "Messages you missed when you were gone:\n"}, {"karly", "Messages you missed when you were gone:\n"}};

    EXPECT_EQ(active_users, expected_active_users);
    EXPECT_EQ(logged_out_users, expected_logged_out_users);
}

