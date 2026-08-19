#include "core/file_manager.hpp"
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

namespace tvwc::core {

fs::path FileManager::get_video_dir() {
    const char* home = std::getenv("HOME");
    std::string base = home ? home : ".";
    return fs::path(base) / "videos";
}

void FileManager::ensure_video_dir() {
    fs::path dir = get_video_dir();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
}

// "a.mp4" varsa "a_1.mp4", "a_2.mp4" şeklinde çakışmayı önler
std::string FileManager::get_unique_filename(const std::string& filename) {
    ensure_video_dir();
    fs::path target = get_video_dir() / filename;

    if (!fs::exists(target)) {
        return target.string();
    }

    std::string stem = target.stem().string();
    std::string ext = target.extension().string();
    int counter = 1;

    while (fs::exists(get_video_dir() / (stem + "_" + std::to_string(counter) + ext))) {
        counter++;
    }

    return (get_video_dir() / (stem + "_" + std::to_string(counter) + ext)).string();
}

} // namespace tvwc::core