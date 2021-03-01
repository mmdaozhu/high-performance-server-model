#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

#include "TCPServer.h"

namespace ppc {

std::string get_filename_ext(const std::string& path);

class HTTPServer : public TCPServer {
public:
    HTTPServer(const std::string& server_address, int port);
    HTTPServer() = delete;
    ~HTTPServer() = default;

    void HandleHttpMethod(char* method_buffer, int client_socket);

private:
    void handle_get_method(const std::string& path, int client_socket);

    void send_headers(const std::string& path, std::uintmax_t len, int client_socket);
    void send_file_contents(const std::string& path, std::uintmax_t len, int client_socket);
    void handle_http_404(int client_socket);

private:
    const std::string http_ok_content = "HTTP/1.0 200 OK\r\n";
    const std::string server_string = "Server: HTTPServer/0.1\r\n";
    const std::string content_type_html = "Content-Type: text/html\r\n";
    const std::string content_type_txt = "Content-Type: text/plain\r\n";
    const std::string content_type_jpeg = "Content-Type: image/jpeg\r\n";

    const std::string http_404_content =
        "HTTP/1.0 404 Not Found\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<html>"
        "<head>"
        "<title>HTTPServer: Not Found</title>"
        "</head>"
        "<body>"
        "<h1>Not Found (404)</h1>"
        "<p>Your client is asking for an object that was not found on this server.</p>"
        "</body>"
        "</html>";
};
}  // namespace ppc

#endif