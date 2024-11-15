#include "client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <string.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

#define SUCCESS 0
#define UNSUCCESS 1
#define WARNING 2

void print_message(string &x, int level) {
    if (level == SUCCESS)
        cout << ANSI_COLOR_GREEN ANSI_STYLE_BOLD "[Success] " ANSI_RESET_ALL;
    else if (level == UNSUCCESS)
        cout << ANSI_COLOR_RED ANSI_STYLE_BOLD "[Unsuccess] " ANSI_RESET_ALL;
    else if (level == WARNING)
        cout << ANSI_COLOR_YELLOW ANSI_STYLE_BOLD "[Warning] " ANSI_RESET_ALL;
    else exit(1);

    cout << x << endl << endl;
}

void print_available_command_msg_logout(void) {
    cout << "Available commands: "\
        ANSI_STYLE_UNDERLINE "register" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "login" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "available command" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "clear" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "exit" ANSI_RESET_ALL << endl << endl;
}

void print_available_command_msg_login(void){
    cout << "Available commands: "\
        ANSI_STYLE_UNDERLINE "logout" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "registered user" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "online user" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "available command" ANSI_RESET_ALL ", "\
        ANSI_STYLE_UNDERLINE "clear" ANSI_RESET_ALL << endl << endl;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: client [port]";
        exit(1);
    }

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(socketfd >= 0);

    sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(std::stoi(argv[1]));
    
    assert(inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) > 0);

    assert(connect(socketfd, (sockaddr *) &serverAddress, sizeof(sockaddr)) >= 0);

    print_available_command_msg_logout();

    bool is_login = false;
    string login_account;

    while (true) {
        string command;
        if (!is_login) {
            cout << ANSI_STYLE_BOLD "COMMAND:  " ANSI_RESET_ALL;
            getline(cin, command);
            if (command.compare("register") == 0) {
                Request request;
                strcpy(request.command, "register");

                cout << ANSI_STYLE_BOLD "ACCOUNT:  " ANSI_RESET_ALL;
                string account;
                getline(cin, account);
                strcpy(request.account, account.c_str());

                cout << ANSI_STYLE_BOLD "PASSWORD: " ANSI_RESET_ALL;
                string password;
                getline(cin, password);
                strcpy(request.password, password.c_str());
    
                assert(send(socketfd, &request, sizeof(request), 0) != -1);

                Response response;
                assert(recv(socketfd, &response, sizeof(response), RCV_FLAGS) != -1);
    
                string status(response.status);
                if (status.compare("successful") == 0) {
                    string msg = "Successfully register account [" + account + "]";
                    print_message(msg, SUCCESS);
                }
                else {
                    string msg = "Account [" + account + "] already exists";
                    print_message(msg, WARNING);
                }
            }
            else if (command.compare("login") == 0) {
                Request request;
                strcpy(request.command, "login");

                cout << ANSI_STYLE_BOLD "ACCOUNT:  " ANSI_RESET_ALL;
                string account;
                getline(cin, account);
                strcpy(request.account, account.c_str());

                cout << ANSI_STYLE_BOLD "PASSWORD: " ANSI_RESET_ALL;
                string password;
                getline(cin, password);
                strcpy(request.password, password.c_str());

                assert(send(socketfd, &request, sizeof(request), 0) != -1);

                Response response;
                assert(recv(socketfd, &response, sizeof(response), RCV_FLAGS) != -1);

                string status(response.status);
                if (status.compare("successful") == 0) {
                    string msg = "Successfully login";
                    print_message(msg, SUCCESS);
                    is_login = true;
                    login_account = account;
                    print_available_command_msg_login();
                }
                else {
                    string msg = "Login fails, account or password incorrect";
                    print_message(msg, UNSUCCESS);
                }
            }
            else if (command.compare("exit") == 0) {
                Request request;
                strcpy(request.command, "exit");
                send(socketfd, &request, sizeof(request), 0);
                close(socketfd);
                break;
            }
            else if (command.compare("available command") == 0) {
                cout << endl;
                print_available_command_msg_logout();
            }
            else if (command.compare("clear") == 0) {
                cout << "\033[2J\033[H";
                cout << endl;
                print_available_command_msg_logout();
            }
            else {
                string msg = "Command does not exist";
                print_message(msg, UNSUCCESS);
                cout << endl;
                print_available_command_msg_logout();
            }
        }
        else {
            cout << ANSI_COLOR_CYAN ANSI_STYLE_BOLD "(" << login_account << ") " ANSI_RESET_ALL;
            getline(cin, command);
            if (command.compare("logout") == 0) {
                Request request;
                strcpy(request.command, "logout");
                send(socketfd, &request, sizeof(request), 0);

                is_login = false;
                login_account = "";
                cout << endl;
                print_available_command_msg_logout();
            }
            else if (command.compare("registered user") == 0) {
                Request request;
                strcpy(request.command, "registered user");
                send(socketfd, &request, sizeof(request), 0);

                Response response;
                assert(recv(socketfd, &response, sizeof(response), RCV_FLAGS) != -1);

                char buf[response.subsequent_data_size];

                assert(recv(socketfd, buf, response.subsequent_data_size, RCV_FLAGS) != -1);

                cout << buf;
            }
            else if (command.compare("online user") == 0) {
                Request request;
                strcpy(request.command, "online user");
                send(socketfd, &request, sizeof(request), 0);

                Response response;
                assert(recv(socketfd, &response, sizeof(response), RCV_FLAGS) != -1);

                char buf[response.subsequent_data_size];

                assert(recv(socketfd, buf, response.subsequent_data_size, RCV_FLAGS) != -1);

                cout << buf;
            }
            else if (command.compare("available command") == 0) {
                cout << endl;
                print_available_command_msg_login();
            }
            else if (command.compare("clear") == 0) {
                cout << "\033[2J\033[H";
                cout << endl;
                print_available_command_msg_login();
            }
            else {
                cout << endl;
                print_available_command_msg_login();
            }
        }
    }
}