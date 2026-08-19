#ifndef TVWC_NETWORK_HTTP_SERVER_HPP
#define TVWC_NETWORK_HTTP_SERVER_HPP

#include <string>
#include <atomic>
#include <thread>

namespace tvwc::network {

class HTTPServer {
public:
    explicit HTTPServer(int port = 8080);
    ~HTTPServer();

    void start();
    void stop();
    void set_upload_busy(bool status);
    bool is_upload_busy() const;

private:
    int port_;
    int server_fd_ = -1;
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_uploading_{false};
    std::thread server_thread_;

    void listen_loop();
    void handle_client(int client_socket);
    std::string get_mime_type(const std::string& path);
    bool parse_range(const std::string& range_header, size_t file_size, size_t& start, size_t& end);
};

} // namespace tvwc::network

#endif