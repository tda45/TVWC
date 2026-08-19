#ifndef TVWC_CRYPTO_UTILS_HPP
#define TVWC_CRYPTO_UTILS_HPP

#include <string>

namespace tvwc::crypto {

class CryptoUtils {
public:
    // Sezar Şifreleme (+3 Kaydırma)
    static std::string caesar_encrypt(const std::string& input, int shift = 3);
    static std::string caesar_decrypt(const std::string& input, int shift = 3);

    // OpenSSL SHA-256 Karma Fonksiyonu
    static std::string sha256(const std::string& input);

    // 45 Haneli Asal/Kriptografik Seri Numarası Üretici
    static std::string generate_45digit_serial();
};

} // namespace tvwc::crypto

#endif // TVWC_CRYPTO_UTILS_HPP