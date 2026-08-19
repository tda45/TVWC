#include "network/http_server.hpp"
#include "core/file_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace tvwc::network {

HTTPServer::HTTPServer(int port) : port_(port) {}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[NET] Soket oluşturulamadı!\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[NET] Port " << port_ << " bağlanamadı!\n";
        close(server_fd_);
        return;
    }

    if (listen(server_fd_, 10) < 0) {
        std::cerr << "[NET] Listen hatası!\n";
        close(server_fd_);
        return;
    }

    is_running_ = true;
    std::cout << "[NET] HTTP Video Streaming Sunucusu Port " << port_ << " üzerinde aktif!\n";
    
    server_thread_ = std::thread(&HTTPServer::listen_loop, this);
    server_thread_.detach();
}

void HTTPServer::stop() {
    if (is_running_) {
        is_running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        std::cout << "[NET] Sunucu durduruldu.\n";
    }
}

void HTTPServer::set_upload_busy(bool status) {
    is_uploading_ = status;
}

bool HTTPServer::is_upload_busy() const {
    return is_uploading_.load();
}

std::string HTTPServer::get_mime_type(const std::string& path) {
    if (path.rfind(".mp4") != std::string::npos) return "video/mp4";
    if (path.rfind(".webm") != std::string::npos) return "video/webm";
    if (path.rfind(".mkv") != std::string::npos) return "video/x-matroska";
    return "application/octet-stream";
}

bool HTTPServer::parse_range(const std::string& range_header, size_t file_size, size_t& start, size_t& end) {
    std::regex range_regex(R"(bytes=(\d+)-(\d*))");
    std::smatch match;
    if (std::regex_search(range_header, match, range_regex)) {
        start = std::stoull(match[1].str());
        if (!match[2].str().empty()) {
            end = std::stoull(match[2].str());
        } else {
            end = file_size - 1;
        }
        if (start >= file_size) return false;
        if (end >= file_size) end = file_size - 1;
        return true;
    }
    return false;
}

void HTTPServer::listen_loop() {
    while (is_running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            if (!is_running_) break;
            continue;
        }

        std::thread(&HTTPServer::handle_client, this, client_socket).detach();
    }
}

void HTTPServer::handle_client(int client_socket) {
    char buffer[4096];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string request(buffer);

    // Yükleme kontrolü
    if (is_uploading_) {
        std::string busy_msg = "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/plain; charset=utf-8\r\n\r\nYönetici Yeni Videolar Yüklüyor Lütfen Bekleyin...";
        send(client_socket, busy_msg.c_str(), busy_msg.size(), MSG_NOSIGNAL);
        close(client_socket);
        return;
    }

    // İstek yöntemini ve yolunu al
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;

    // Range başlığını doğrudan string içinden güvenle ayıkla
    std::string range_header;
    size_t range_pos = request.find("Range: ");
    if (range_pos == std::string::npos) {
        range_pos = request.find("range: ");
    }
    if (range_pos != std::string::npos) {
        size_t end_pos = request.find("\r\n", range_pos);
        if (end_pos != std::string::npos) {
            range_header = request.substr(range_pos + 7, end_pos - (range_pos + 7));
        }
    }

    // Varsayılan video istek kontrolü
    auto video_dir = core::FileManager::get_video_dir();
    std::string file_name = (path == "/" || path.empty()) ? "" : path.substr(1);
    auto file_path = video_dir / file_name;

    if (file_name.empty() || !std::ifstream(file_path).good()) {
        std::string res = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, res.c_str(), res.size(), MSG_NOSIGNAL);
        close(client_socket);
        return;
    }

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    size_t file_size = file.tellg();
    size_t start = 0;
    size_t end = file_size - 1;
    bool is_range = !range_header.empty() && parse_range(range_header, file_size, start, end);

    size_t content_length = (end - start) + 1;
    std::string mime = get_mime_type(file_path.string());

    std::stringstream res_headers;
    if (is_range) {
        res_headers << "HTTP/1.1 206 Partial Content\r\n";
        res_headers << "Content-Range: bytes " << start << "-" << end << "/" << file_size << "\r\n";
    } else {
        res_headers << "HTTP/1.1 200 OK\r\n";
    }

    res_headers << "Content-Type: " << mime << "\r\n";
    res_headers << "Accept-Ranges: bytes\r\n";
    res_headers << "Content-Length: " << content_length << "\r\n";
    res_headers << "Connection: close\r\n\r\n";

    std::string header_str = res_headers.str();
    send(client_socket, header_str.c_str(), header_str.size(), MSG_NOSIGNAL);

    // HEAD isteğinde body göndermeden çık
    if (method == "HEAD") {
        close(client_socket);
        return;
    }

    file.seekg(start, std::ios::beg);
    char chunk[64 * 1024];
    size_t bytes_remaining = content_length;

    while (bytes_remaining > 0 && is_running_) {
        size_t to_read = std::min(sizeof(chunk), bytes_remaining);
        file.read(chunk, to_read);
        ssize_t read_bytes = file.gcount();
        if (read_bytes <= 0) break;

        ssize_t sent = send(client_socket, chunk, read_bytes, MSG_NOSIGNAL);
        if (sent <= 0) break;

        bytes_remaining -= sent;
    }

    close(client_socket);
}

} // namespace tvwc::network