#ifndef TVWC_FILE_MANAGER_HPP
#define TVWC_FILE_MANAGER_HPP

#include <string>
#include <filesystem>

namespace tvwc::core {

class FileManager {
public:
    static std::filesystem::path get_video_dir();
    static void ensure_video_dir();
    static std::string get_unique_filename(const std::string& filename);
};

} // namespace tvwc::core

#endif