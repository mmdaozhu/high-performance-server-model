#include "TCPServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

namespace tpc {

void FatalError(const std::string& syscall) {
    perror(syscall.c_str());
    exit(1);
}

int GetLine(int sock, char* buf, int size) {
    int i(0);
    char c('\0');
    ssize_t n;

    while (i < size - 1 && c != '\n') {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if (n > 0 && c == '\n') {
                    recv(sock, &c, 1, 0);
                }

            } else {
                buf[i] = c;
                i++;
            }
        } else {
            // not a line.
            return 0;
        }
    }

    buf[i] = '\0';
    return i;
}

TCPServer::TCPServer(const std::string& server_address, int port) : server_address_(server_address), port_(port) {
    std::cout << "server_address: " << server_address_ << " port: " << port_ << std::endl;
}

void TCPServer::SetUpListeningSocket() {
    // step 1: socket
    listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd_ == -1) {
        FatalError("socket()");
    }

    // open address reuse function
    // deal with TIME_WAIT status
    int enable = 1;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        FatalError("setsockopt(SO_REUSEADDR)");
    }

    // step 2: bind
    sockaddr_in srv_addr;
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, server_address_.c_str(), &srv_addr.sin_addr) < 0) {
        FatalError("inet_pton()");
    }

    if (bind(listenfd_, (sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        FatalError("bind()");
    }

    // step 3: listen
    if (listen(listenfd_, 128) < 0) {
        FatalError("listen()");
    }
}

void TCPServer::Start() {
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        // step 4: accept
        int client_socket = accept(listenfd_, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            FatalError("accept()");
        }
        printf("accept success.\n");

        std::thread thread(&TCPServer::HandleClient, this, client_socket);
        thread.detach();
    }
}

void TCPServer::SetHandle(Handler handler) { handler_ = handler; }

void TCPServer::HandleClient(int client_socket) {
    printf("HandleClient\n");
    printf("client_socket: %d\n", client_socket);

    char line_buffer[1024] = {0};
    char method_buffer[1024] = {0};
    int method_line = 0;

    while (true) {
        GetLine(client_socket, line_buffer, sizeof(line_buffer));
        std::cout << line_buffer << std::endl;
        method_line++;

        unsigned long len = strlen(line_buffer);
        if (method_line == 1) {
            if (len == 0) {
                return;
            }
            bcopy(line_buffer, method_buffer, len);
        } else {
            if (len == 0) {
                break;
            }
        }
    }
    handler_(method_buffer, client_socket);
    close(client_socket);
}

}  // namespace tpc