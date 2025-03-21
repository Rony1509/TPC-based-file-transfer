#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <stdarg.h>
#include <fstream>
#include <sstream>

#define SERVER_PORT 5208
#define BUF_SIZE 1024
#define MAX_CLNT 256

void handle_clnt(int clnt_sock);
void send_msg(const std::string &msg);
void receive_file(int clnt_sock, const std::string &filename);
void send_file(int clnt_sock, const std::string &filename);
int output(const char *arg,...);
int error_output(const char *arg,...);
void error_handling(const std::string &message);

int clnt_cnt = 0;
std::mutex mtx;
std::unordered_map<std::string, int> clnt_socks;

int main(int argc, const char **argv, const char **envp) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1) {
        error_handling("socket() failed!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("bind() failed!");
    }
    printf("The server is running on port %d\n", SERVER_PORT);

    if (listen(serv_sock, MAX_CLNT) == -1) {
        error_handling("listen() error!");
    }

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) {
            error_handling("accept() failed!");
        }

        mtx.lock();
        clnt_cnt++;
        mtx.unlock();

        std::thread th(handle_clnt, clnt_sock);
        th.detach();

        output("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void handle_clnt(int clnt_sock) {
    char msg[BUF_SIZE];
    int flag = 0;

    char tell_name[13] = "#new client:";
    while (recv(clnt_sock, msg, sizeof(msg), 0) != 0) {
        if (std::strlen(msg) > std::strlen(tell_name)) {
            char pre_name[13];
            std::strncpy(pre_name, msg, 12);
            pre_name[12] = '\0';
            if (std::strcmp(pre_name, tell_name) == 0) {
                char name[20];
                std::strcpy(name, msg + 12);
                if (clnt_socks.find(name) == clnt_socks.end()) {
                    output("The name of socket %d: %s\n", clnt_sock, name);
                    clnt_socks[name] = clnt_sock;
                } else {
                    std::string error_msg = std::string(name) + " already exists. Please quit and enter with another name!";
                    send(clnt_sock, error_msg.c_str(), error_msg.length() + 1, 0);
                    mtx.lock();
                    clnt_cnt--;
                    mtx.unlock();
                    flag = 1;
                }
            }
        }

        if (flag == 0)
            send_msg(std::string(msg));
    }
    if (flag == 0) {
        std::string leave_msg;
        std::string name;
        mtx.lock();
        for (auto it = clnt_socks.begin(); it != clnt_socks.end(); ++it) {
            if (it->second == clnt_sock) {
                name = it->first;
                clnt_socks.erase(it->first);
            }
        }
        clnt_cnt--;
        mtx.unlock();
        leave_msg = "Client " + name + " leaves the chat room";
        send_msg(leave_msg);
        output("Client %s leaves the chat room\n", name.c_str());
        close(clnt_sock);
    } else {
        close(clnt_sock);
    }
}

void send_msg(const std::string &msg) {
    mtx.lock();
    std::string pre = "@";
    int first_space = msg.find_first_of(" ");
    if (msg.compare(first_space + 1, 1, pre) == 0) {
        int space = msg.find_first_of(" ", first_space + 1);
        std::string receive_name = msg.substr(first_space + 2, space - first_space - 2);
        std::string send_name = msg.substr(1, first_space - 2);
        if (clnt_socks.find(receive_name) == clnt_socks.end()) {
            std::string error_msg = "[Error] There is no client named " + receive_name;
            send(clnt_socks[send_name], error_msg.c_str(), error_msg.length() + 1, 0);
        } else {
            send(clnt_socks[receive_name], msg.c_str(), msg.length() + 1, 0);
            send(clnt_socks[send_name], msg.c_str(), msg.length() + 1, 0);
        }
    } else {
        for (auto it = clnt_socks.begin(); it != clnt_socks.end(); it++) {
            send(it->second, msg.c_str(), msg.length() + 1, 0);
        }
    }
    mtx.unlock();
}


void receive_file(int clnt_sock, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "[Error] Failed to create file\n";
        return;
    }

    char buffer[BUF_SIZE];
    int bytes_received;
    while ((bytes_received = recv(clnt_sock, buffer, BUF_SIZE, 0)) > 0) {
        file.write(buffer, bytes_received);
        if (bytes_received < BUF_SIZE) {
            break;  
        }
    }

    file.close();
    std::string msg = "File received successfully: " + filename;
    send(clnt_sock, msg.c_str(), msg.length() + 1, 0);
}


void send_file(int clnt_sock, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "[Error] Failed to open file\n";
        return;
    }

    char buffer[BUF_SIZE];
    while (file.read(buffer, BUF_SIZE) || file.gcount() > 0) {
        send(clnt_sock, buffer, file.gcount(), 0);
    }

    file.close();
    std::string msg = "File sent successfully: " + filename;
    send(clnt_sock, msg.c_str(), msg.length() + 1, 0);
}

int output(const char *arg, ...) {
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stdout, arg, ap);
    va_end(ap);
    return res;
}

int error_output(const char *arg, ...) {
    int res;
    va_list ap;
    va_start(ap, arg);
    res = vfprintf(stderr, arg, ap);
    va_end(ap);
    return res;
}
//rony
void error_handling(const std::string &message) {
    std::cerr << message << std::endl;
    exit(1);
}
