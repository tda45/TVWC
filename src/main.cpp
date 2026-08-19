#include <iostream>
#include <fstream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>

#include "crypto/crypto_utils.hpp"
#include "core/file_manager.hpp"
#include "network/http_server.hpp"

using namespace tvwc;

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[SYS] Kapatma sinyali alındı...\n";
        g_running = false;
    }
}

// Seri numarasını ilk çalıştırmada oluşturup kaydeder, sonraki açılışlarda dosyadan okur
std::string get_or_create_serial() {
    std::filesystem::create_directories("config");
    std::string serial_path = "config/serial.key";
    
    std::ifstream in(serial_path);
    if (in.good()) {
        std::string serial;
        in >> serial;
        if (!serial.empty()) {
            return serial;
        }
    }

    std::string new_serial = crypto::CryptoUtils::generate_45digit_serial();
    std::ofstream out(serial_path);
    if (out.is_open()) {
        out << new_serial;
    }
    return new_serial;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN); // Connection reset / Broken pipe çökmesini engeller

    core::FileManager::ensure_video_dir();

    // Kalıcı Seri Numarası Yönetimi
    std::string serial = get_or_create_serial();

    // CLI Komutu: tvwc set password --reason client-managent
    if (argc >= 5 && std::string(argv[1]) == "set" &&
                     std::string(argv[2]) == "password" &&
                     std::string(argv[3]) == "--reason" &&
                     std::string(argv[4]) == "client-managent") {

        std::cout << "Yönetici şifrenizi girin: ";
        std::string raw_pass;
        std::cin >> raw_pass;

        std::string enc_pass = crypto::CryptoUtils::caesar_encrypt(raw_pass, 3);
        
        std::ofstream out("config/admin.pass");
        out << enc_pass;

        std::cout << "\nClient yönetimi şifresi başarıyla oluşturuldu şifre:" << enc_pass << std::endl;
        return 0;
    }

    std::cout << "=================================================\n";
    std::cout << " TVWC SERVER RUNNING (OpenSSL Enabled)\n";
    std::cout << " SERİ NO: " << serial << "\n";
    std::cout << " SHA-256: " << crypto::CryptoUtils::sha256(serial) << "\n";
    std::cout << "=================================================\n";

    network::HTTPServer server(8080);
    server.start();

    std::cout << "[INFO] Sunucu aktif. Durdurmak için Ctrl+C basın.\n";

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    server.stop();
    return 0;
}