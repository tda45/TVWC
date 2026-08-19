#include "crypto/crypto_utils.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>
#include <random>

namespace tvwc::crypto {

std::string CryptoUtils::caesar_encrypt(const std::string& input, int shift) {
    std::string result = input;
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') c = 'a' + (c - 'a' + shift) % 26;
        else if (c >= 'A' && c <= 'Z') c = 'A' + (c - 'A' + shift) % 26;
        else if (c >= '0' && c <= '9') c = '0' + (c - '0' + shift) % 10;
    }
    return result;
}

std::string CryptoUtils::caesar_decrypt(const std::string& input, int shift) {
    return caesar_encrypt(input, 26 - (shift % 26));
}

std::string CryptoUtils::sha256(const std::string& input) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, input.c_str(), input.size());
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string CryptoUtils::generate_45digit_serial() {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    unsigned char rand_bytes[45];
    RAND_bytes(rand_bytes, sizeof(rand_bytes)); // OpenSSL Kriptografik Rastgelelik

    std::string serial;
    serial.reserve(45);
    for (int i = 0; i < 45; ++i) {
        serial += charset[rand_bytes[i] % charset.length()];
    }
    return serial;
}

} // namespace tvwc::crypto