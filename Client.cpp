#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define BUF_SIZE 1024
#define SERVER_PORT 5208
#define IP "127.0.0.1"

void send_msg(int sock);
void recv_msg(int sock);
void send_file(int sock);
void recv_file(int sock);
int output(const char *arg, ...);
int error_output(const char *arg, ...);
void error_handling(const std::string &message);

std::string name = "DEFAULT";
std::string msg;

int main(int argc, const char **argv, const char **envp) {
    int sock;
    struct sockaddr_in serv_addr;

    if (argc != 2) {
        error_output("Usage : %s <Name> \n", argv[0]);
        exit(1);
    }

    name = "[" + std::string(argv[1]) + "]";


    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        error_handling("socket() failed!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("connect() failed!");
    }

    std::string my_name = "#new client:" + std::string(argv[1]);
    send(sock, my_name.c_str(), my_name.length() + 1, 0);

    std::thread snd(send_msg, sock);
    std::thread rcv(recv_msg, sock);

    snd.join();
    rcv.join();

    close(sock);

    return 0;
}

void send_msg(int sock) {
    while (1) {
        getline(std::cin, msg);
        if (msg == "Quit" || msg == "quit") {
            close(sock);
            exit(0);
        } else if (msg.rfind("send_file", 0) == 0) {
            
            send_file(sock);
        } else {
            std::string name_msg = name + " " + msg;
            send(sock, name_msg.c_str(), name_msg.length() + 1, 0);
        }
    }
}

void recv_msg(int sock) {
    char name_msg[BUF_SIZE + name.length() + 1];
    while (1) {
        int str_len = recv(sock, name_msg, BUF_SIZE + name.length() + 1, 0);
        if (str_len == -1) {
            exit(-1);
        }
        
        
        std::string message = std::string(name_msg);
        if (message.rfind("send_file", 0) == 0) {
            recv_file(sock);
        } else {
            std::cout << message << std::endl;
        }
    }
}

void send_file(int sock) {
    std::string filename;
    std::cout << "Enter the filename to send: ";
    std::cin >> filename;

    
    std::string file_request = "send_file " + filename;
    send(sock, file_request.c_str(), file_request.length() + 1, 0);

    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "File could not be opened.\n";
        return;
    }

    char buffer[BUF_SIZE];
    while (file.read(buffer, BUF_SIZE) || file.gcount() > 0) {
        send(sock, buffer, file.gcount(), 0);
    }

    std::cout << "File sent successfully.\n";
    file.close();
}

void recv_file(int sock) {
    std::string filename = "received_file";
    std::ofstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "[Error] Failed to create file\n";
        return;
    }

    char buffer[BUF_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        file.write(buffer, bytes_received);
        if (bytes_received < BUF_SIZE) {
            break;  // End of file
        }
    }

    file.close();
    std::cout << "File received successfully: " << filename << std::endl;
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

void error_handling(const std::string &message) {
    std::cerr << message << std::endl;
    exit(1);
}
