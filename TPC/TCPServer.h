#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <functional>
#include <string>

namespace tpc {

void FatalError(const std::string& syscall);

int GetLine(int sock, char* buf, int size);

class TCPServer {
    using Handler = std::function<void(char* method_buffer, int client_socket)>;

public:
    TCPServer(const std::string& server_address, int port);
    TCPServer() = delete;
    ~TCPServer() = default;

    void SetUpListeningSocket();

    void Start();

    void SetHandle(Handler handler);

private:
    void HandleClient(int client_socket);

public:
    Handler handler_;

private:
    std::string server_address_;
    int port_;
    int listenfd_;
};
}  // namespace tpc

#endif