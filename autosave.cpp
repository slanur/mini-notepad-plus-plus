// autosave.cpp — Otomatik yedek dosyaları implementasyonu

#include "autosave.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

#ifndef PLATFORM_WINDOWS
  #include <fcntl.h>
  #include <unistd.h>
#endif

namespace Autosave {

std::string pathFor(const std::string &filePath) {
    if (filePath.empty()) {
#ifdef PLATFORM_WINDOWS
        const char *home = std::getenv("USERPROFILE");
        const char *sep  = "\\";
#else
        const char *home = std::getenv("HOME");
        const char *sep  = "/";
#endif
        return std::string(home ? home : ".") + sep + ".mini-notepad.autosave";
    }

    size_t sep = filePath.find_last_of("/\\");
    std::string dir  = sep == std::string::npos ? "" : filePath.substr(0, sep + 1);
    std::string name = sep == std::string::npos ? filePath : filePath.substr(sep + 1);
    return dir + "." + name + ".autosave";
}

bool write(const std::string &path, const std::string &text) {
    std::string tmp = path + ".tmp";

#ifndef PLATFORM_WINDOWS
    // Dosya 0600 izniyle olusturulur; ofstream var olan izni korur
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) return false;
    close(fd);
#endif

    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) return false;
        ofs << text;
        if (!ofs.good()) return false;
    }

#ifdef PLATFORM_WINDOWS
    std::remove(path.c_str()); // Windows'ta rename var olan dosyanin ustune yazmaz
#endif
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

bool read(const std::string &path, std::string &text) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return false;
    text.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return true;
}

void remove(const std::string &path) {
    if (!path.empty()) std::remove(path.c_str());
}

} // namespace Autosave
