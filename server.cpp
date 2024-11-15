#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <vector>
#include <set>
#include <map>
#include <string.h>
#include <pthread.h>

#define N_REQUEST_QUEUE 10

using std::cin, std::cout, std::cerr, std::endl, std::string, std::set, std::vector, std::map, std::pair;

pthread_mutex_t registered_account_lock;
map<string, string> registered_account;

pthread_mutex_t online_account_lock;
set<string> online_account;

pthread_mutex_t output_lock;

void *handle_connection_request(void *arg) {
    Thread_Argument *a = (Thread_Argument *) arg;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &a->addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(a->addr.sin_port);

    pthread_mutex_lock(&output_lock);
    cerr << "[Log] connection accepted from " << client_ip << ":" << client_port << endl;
    pthread_mutex_unlock(&output_lock);

    string login_account = "";
    while (true) {
        Request request;
        int size;
        if ((size = recv(a->fd, &request, sizeof(request), RCV_FLAGS)) <= 0) {
            pthread_mutex_lock(&output_lock);
            cerr << "[Log] broken connection, need reconnection" << endl;
            pthread_mutex_unlock(&output_lock);
            break;
        }
        
        string cmd(request.command);
        
        pthread_mutex_lock(&output_lock);
        cerr << "[Log] receive command [" << cmd << "]" << endl;
        pthread_mutex_unlock(&output_lock);

        if (cmd.compare("register") == 0) {
            string account(request.account);
            string password(request.password);

            pthread_mutex_lock(&registered_account_lock);
            bool account_exist = registered_account.count(account);
            pthread_mutex_unlock(&registered_account_lock);

            if (account_exist) {

                pthread_mutex_lock(&output_lock);
                cerr << "[Log] account [" << account <<"] already exists" << endl;
                pthread_mutex_unlock(&output_lock);

                Response response;
                strcpy(response.status, "unsuccessful");
                send(a->fd, &response, sizeof(response), 0);
            }
            else {

                pthread_mutex_lock(&registered_account_lock);
                registered_account.insert(pair<string, string>(account, password));
                pthread_mutex_unlock(&registered_account_lock);

                pthread_mutex_lock(&output_lock);
                cerr << "[Log] account [" << account <<"] is registered" << endl;
                pthread_mutex_unlock(&output_lock);

                Response response;
                strcpy(response.status, "successful");
                send(a->fd, &response, sizeof(response), 0);
            }
        }
        else if (cmd.compare("login") == 0) {
            string account(request.account);
            string password(request.password);

            pthread_mutex_lock(&registered_account_lock);
            bool account_exist = registered_account.count(account) && registered_account[account].compare(password) == 0;
            pthread_mutex_unlock(&registered_account_lock);

            if (account_exist) {
                login_account = account;

                pthread_mutex_lock(&output_lock);
                cerr << "[Log] account [" << login_account << "] login to the server" << endl;
                pthread_mutex_unlock(&output_lock);

                pthread_mutex_lock(&online_account_lock);
                online_account.insert(login_account);
                pthread_mutex_unlock(&online_account_lock);

                Response response;
                strcpy(response.status, "successful");
                send(a->fd, &response, sizeof(response), 0);
            }
            else {

                pthread_mutex_lock(&output_lock);
                cerr << "[Log] Login fails, account or password incorrect" << endl;
                pthread_mutex_unlock(&output_lock);

                Response response;
                strcpy(response.status, "unsuccessful");
                send(a->fd, &response, sizeof(response), 0);
            }
        }
        else if (cmd.compare("logout") == 0) {

            pthread_mutex_lock(&output_lock);
            cerr << "[Log] account [" << login_account << "] logout from the server" << endl;
            pthread_mutex_unlock(&output_lock);

            pthread_mutex_lock(&online_account_lock);
            online_account.erase(login_account);
            pthread_mutex_unlock(&online_account_lock);

            login_account = "";
        }
        else if (cmd.compare("exit") == 0) {

            pthread_mutex_lock(&output_lock);
            cerr << "[Log] client left" << endl;
            pthread_mutex_unlock(&output_lock);
            
            break;
        }
        else if (cmd.compare("registered user") == 0) {
            Response response;
            strcpy(response.status, "successful");

            string accounts;
            pthread_mutex_lock(&registered_account_lock);
            for (auto const &map_ent : registered_account) {
                accounts += map_ent.first;
                accounts += "\n";
            }
            pthread_mutex_unlock(&registered_account_lock);

            response.subsequent_data_size = accounts.length() + 1;

            send(a->fd, &response, sizeof(response), 0);

            send(a->fd, accounts.c_str(), response.subsequent_data_size, 0);
        }
        else if (cmd.compare("online user") == 0) {
            Response response;
            strcpy(response.status, "successful");

            string accounts;
            pthread_mutex_lock(&online_account_lock);
            for (auto const &set_ent : online_account) {
                accounts += set_ent;
                accounts += "\n";
            }
            pthread_mutex_unlock(&online_account_lock);

            response.subsequent_data_size = accounts.length() + 1;

            send(a->fd, &response, sizeof(response), 0);

            send(a->fd, accounts.c_str(), response.subsequent_data_size, 0);
        }
        else {
            exit(1);
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: server [port]";
        exit(1);
    }

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(socketfd >= 0);

    sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(std::stoi(argv[1]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    assert(bind(socketfd, (sockaddr *) &serverAddress, sizeof(sockaddr)) >= 0);

    assert(listen(socketfd, N_REQUEST_QUEUE) >= 0);

    pthread_mutex_init(&registered_account_lock, NULL);
    pthread_mutex_init(&online_account_lock, NULL);
    pthread_mutex_init(&output_lock, NULL);
    
    vector<Thread_Argument> threads;
    
    while (true) {
        Thread_Argument *thread = new Thread_Argument;

        thread->addr_len = sizeof(socklen_t);
        int newfd = accept(socketfd, (sockaddr *) &thread->addr, &thread->addr_len);

        assert(newfd != -1);

        thread->fd = newfd;

        assert(pthread_create(&thread->th, NULL, handle_connection_request, (void *) thread) == 0);

        threads.push_back(*thread);
    }

    for (const auto& thread : threads) {
        pthread_join(thread.th, NULL);
        close(thread.fd);
    }

    pthread_mutex_destroy(&registered_account_lock);

}