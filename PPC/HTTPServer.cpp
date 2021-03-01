#include "HTTPServer.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

namespace ppc {

std::string get_filename_ext(const std::string& path) { return path.substr(path.find(".") + 1); }

HTTPServer::HTTPServer(const std::string& server_address, int port) : TCPServer(server_address, port) {
    SetHandle(std::bind(&HTTPServer::HandleHttpMethod, this, std::placeholders::_1, std::placeholders::_2));
}

void HTTPServer::HandleHttpMethod(char* method_buffer, int client_socket) {
    std::string method = strtok(method_buffer, " ");
    std::string path = strtok(NULL, " ");
    std::transform(method.begin(), method.end(), method.begin(), ::tolower);

    if (method == "get") {
        handle_get_method(path, client_socket);
    } else if (method == "post") {
        std::cout << "post method" << std::endl;
    } else {
        std::cout << "other method" << std::endl;
    }
}

void HTTPServer::handle_get_method(const std::string& path, int client_socket) {
    std::filesystem::path filepath(path);
    if (!std::filesystem::exists(filepath)) {
        printf("404 Not Found: %s\n", path.c_str());
        handle_http_404(client_socket);
    } else {
        std::filesystem::directory_entry entry(filepath);
        if (entry.status().type() == std::filesystem::file_type::regular) {
            std::uintmax_t len = std::filesystem::file_size(path);
            send_headers(path, len, client_socket);
            send_file_contents(path, len, client_socket);
        } else {
            printf("This is a directory. 404 Not Found: %s\n", path.c_str());
            handle_http_404(client_socket);
        }
    }
}

void HTTPServer::send_headers(const std::string& path, std::uintmax_t len, int client_socket) {
    send(client_socket, http_ok_content.c_str(), http_ok_content.length(), 0);
    send(client_socket, server_string.c_str(), server_string.length(), 0);

    std::string ext = get_filename_ext(path);
    if (ext == "html") {
        send(client_socket, content_type_html.c_str(), content_type_html.length(), 0);
    } else if (ext == "txt") {
        send(client_socket, content_type_txt.c_str(), content_type_txt.length(), 0);
    } else if (ext == "jpg") {
        send(client_socket, content_type_txt.c_str(), content_type_txt.length(), 0);
    }

    std::string send_buffer = "content-length: " + std::to_string(len) + "\r\n";
    send(client_socket, send_buffer.c_str(), send_buffer.length(), 0);

    send_buffer = "\r\n";
    send(client_socket, send_buffer.c_str(), send_buffer.length(), 0);
}

void HTTPServer::send_file_contents(const std::string& path, std::uintmax_t len, int client_socket) {
    int fd = open(path.c_str(), O_RDONLY);
    sendfile(client_socket, fd, NULL, len);
    close(fd);
}

void HTTPServer::handle_http_404(int client_socket) {
    send(client_socket, http_404_content.c_str(), http_404_content.length(), 0);
}

}  // namespace ppc

void SigChildHandler(int signo) {
    while (true) {
        int wait_status;
        // wait for a child process to stop or terminate
        pid_t pid = waitpid(-1, &wait_status, WNOHANG);
        if (pid == 0 || pid == -1) {
            break;
        }
    }
    return;
}

void PrintStats(int signo) {
    using namespace ppc;
    struct rusage myusage;
    if (getrusage(RUSAGE_SELF, &myusage) < 0) {
        FatalError("getrusage()");
    }

    struct rusage childusage;
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0) {
        FatalError("getrusage()");
    }

    double user = (double)myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec / 1000000.0;
    user += (double)childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec / 1000000.0;
    double sys = (double)myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec / 1000000.0;
    sys += (double)childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec / 1000000.0;

    printf("\nuser time = %g, sys time = %g\n", user, sys);
    exit(0);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "usage: main <ip> <port>\n";
        exit(1);
    }
    signal(SIGINT, PrintStats);

    std::string server_address = argv[1];
    int port = std::stoi(argv[2]);

    using namespace ppc;
    HTTPServer server(server_address, port);
    server.SetUpListeningSocket();
    std::cout << "HTTP server listening on port " << port << std::endl;

    // setup the SIGCHLD handler with SA_RESTAR
    struct sigaction sa;
    sa.sa_handler = SigChildHandler;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    server.Start();
    return 0;
}