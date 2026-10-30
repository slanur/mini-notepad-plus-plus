// filemanager.cpp — Konsol dosya yöneticisi implementasyonu

#include "filemanager.h"
#include "terminal.h"
#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef PLATFORM_WINDOWS
  #include <windows.h>
  #include <direct.h>
  #define GETCWD _getcwd
  #define PATH_SEP '\\'
#else
  #include <dirent.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #define GETCWD getcwd
  #define PATH_SEP '/'
#endif

// ── Kurucu ───────────────────────────────────────────────────────────────
FileManager::FileManager() {
    char buf[4096];
    if (GETCWD(buf, sizeof(buf))) currentDir_ = buf;
    else currentDir_ = ".";
}

// ── Dizin listeleme ──────────────────────────────────────────────────────
std::vector<FsEntry> FileManager::listDir(const std::string &dir) const {
    std::vector<FsEntry> entries;

    // Kok dizinde degil ise ".." goster
    bool isRoot = false;
#ifdef PLATFORM_WINDOWS
    // "C:\" veya "C:" gibi kok dizin kontrolu
    isRoot = (dir.size() <= 3 && dir.size() >= 2 && dir[1] == ':');
#else
    isRoot = (dir == "/");
#endif
    if (!isRoot) entries.push_back({"..", true});

#ifdef PLATFORM_WINDOWS
    WIN32_FIND_DATAA fdata;
    std::string pattern = dir + "\\*";
    HANDLE h = FindFirstFileA(pattern.c_str(), &fdata);
    if (h == INVALID_HANDLE_VALUE) return entries;
    do {
        std::string name = fdata.cFileName;
        if (name == "." || name == "..") continue;
        bool isDir = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entries.push_back({name, isDir});
    } while (FindNextFileA(h, &fdata));
    FindClose(h);
#else
    DIR *d = opendir(dir.c_str());
    if (!d) return entries;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string fullPath = dir + "/" + name;
        struct stat st;
        bool isDir = false;
        if (stat(fullPath.c_str(), &st) == 0) {
            isDir = S_ISDIR(st.st_mode);
        }
        entries.push_back({name, isDir});
    }
    closedir(d);
#endif

    // Siralama: once klasorler, sonra dosyalar, alfabetik
    int sortStart = isRoot ? 0 : 1; // ".." varsa ondan sonra sirala
    std::sort(entries.begin() + sortStart, entries.end(), [](const FsEntry &a, const FsEntry &b){
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        return a.name < b.name;
    });
    return entries;
}

// ── Ust dizine git (platform uyumlu) ─────────────────────────────────────
void FileManager::goUp() {
#ifdef PLATFORM_WINDOWS
    // "C:\foo\bar" -> "C:\foo"
    // "C:\foo"     -> "C:\"
    // "C:\"        -> kalir (kok)
    size_t pos = currentDir_.rfind('\\');
    if (pos == std::string::npos) return;
    if (pos == 0) { currentDir_ = "\\"; return; }
    // "C:\" durumu: pos==2 (C:\)
    if (pos <= 2 && currentDir_.size() > 2 && currentDir_[1] == ':') {
        currentDir_ = currentDir_.substr(0, 3); // "C:\" kalsın
        return;
    }
    currentDir_ = currentDir_.substr(0, pos);
#else
    if (currentDir_ == "/") return;
    size_t pos = currentDir_.rfind('/');
    if (pos == 0) { currentDir_ = "/"; return; }
    if (pos != std::string::npos) currentDir_ = currentDir_.substr(0, pos);
#endif
}

// ── Ekran cizimi ─────────────────────────────────────────────────────────
void FileManager::render(const std::vector<FsEntry> &entries, int selected,
                         const std::string &inputBuf, const std::string &mode) const {
    Terminal::clearScreen();
    int rows, cols;
    Terminal::getSize(rows, cols);

    // Baslik
    Terminal::moveCursor(1, 1);
    Terminal::setColor(COLOR_WHITE, COLOR_BLUE);
    std::string title = " Dosya Yoneticisi -- " + currentDir_ + " ";
    while ((int)title.size() < cols) title += ' ';
    Terminal::print(title.substr(0, cols));
    Terminal::resetColor();

    // Kisayol cubugu
    Terminal::moveCursor(2, 1);
    Terminal::setColor(COLOR_YELLOW, COLOR_DEFAULT);
    Terminal::print(" [yukari/asagi] Sec  [sag/Enter] Klasore Gir  [sol] Geri  [ESC] Iptal");
    Terminal::resetColor();

    // Girdi satiri
    Terminal::moveCursor(3, 1);
    Terminal::setColor(COLOR_CYAN, COLOR_DEFAULT);
    if (mode == "save") {
        Terminal::print(" Dosya adi: " + inputBuf + "_");
    } else {
        Terminal::print(" Sec veya yaz: " + inputBuf + "_");
    }
    Terminal::resetColor();

    // Ayirici
    Terminal::moveCursor(4, 1);
    Terminal::setColor(COLOR_BRIGHT_BLACK, COLOR_DEFAULT);
    Terminal::print(std::string(cols, '-'));
    Terminal::resetColor();

    // Dosya listesi
    int startRow   = 5;
    int maxVisible = rows - startRow - 1;
    int startIdx   = 0;
    if (selected >= maxVisible) startIdx = selected - maxVisible + 1;

    for (int i = 0; i < maxVisible && (startIdx + i) < (int)entries.size(); i++) {
        int idx = startIdx + i;
        const FsEntry &e = entries[idx];
        Terminal::moveCursor(startRow + i, 1);

        if (idx == selected) {
            Terminal::setColor(COLOR_BLACK, COLOR_WHITE);
        } else if (e.isDir) {
            Terminal::setColor(COLOR_CYAN, COLOR_DEFAULT);
        } else {
            Terminal::setColor(COLOR_WHITE, COLOR_DEFAULT);
        }

        std::string icon = e.isDir ? " [K] " : " [F] ";
        std::string line = icon + e.name;
        while ((int)line.size() < cols) line += ' ';
        Terminal::print(line.substr(0, cols));
        Terminal::resetColor();
    }

    // Alt bilgi
    Terminal::moveCursor(rows, 1);
    Terminal::setColor(COLOR_BRIGHT_BLACK, COLOR_DEFAULT);
    Terminal::print(std::string(cols, '-'));
    Terminal::resetColor();

    fflush(stdout);
}

// ── Ana diyalog ──────────────────────────────────────────────────────────
std::string FileManager::show(const std::string &mode, const std::string &startDir) {
    if (!startDir.empty()) currentDir_ = startDir;

    auto entries = listDir(currentDir_);
    int  selected = 0;
    std::string inputBuf;

    while (true) {
        render(entries, selected, inputBuf, mode);
        int key = Terminal::readKey();

        if (key == KEY_ESC) {
            return ""; // Iptal

        } else if (key == KEY_UP) {
            if (selected > 0) selected--;

        } else if (key == KEY_DOWN) {
            if (selected < (int)entries.size() - 1) selected++;

        } else if (key == KEY_RIGHT ||
                   (key == KEY_ENTER && selected < (int)entries.size() && entries[selected].isDir)) {
            // Klasore gir
            if (selected < (int)entries.size() && entries[selected].isDir) {
                std::string name = entries[selected].name;
                if (name == "..") {
                    goUp();
                } else {
                    currentDir_ += PATH_SEP;
                    currentDir_ += name;
                }
                entries  = listDir(currentDir_);
                selected = 0;
                inputBuf.clear();
            }

        } else if (key == KEY_LEFT) {
            goUp();
            entries  = listDir(currentDir_);
            selected = 0;
            inputBuf.clear();

        } else if (key == KEY_ENTER) {
            // Dosya sec: ya kullanici bir isim yazdiysa onu, yoksa secili dosyayi kullan
            std::string finalName;
            if (!inputBuf.empty()) {
                finalName = inputBuf;
            } else if (selected < (int)entries.size() && !entries[selected].isDir) {
                finalName = entries[selected].name;
            } else {
                continue;
            }
            // Tam yolu olustur
            std::string sep(1, PATH_SEP);
            // Kok dizinde PATH_SEP zaten sonda var olabilir (C:\), fazladan ekleme
            if (!currentDir_.empty() && currentDir_.back() == PATH_SEP)
                return currentDir_ + finalName;
            return currentDir_ + sep + finalName;

        } else if (key == KEY_BACKSPACE) {
            if (!inputBuf.empty()) inputBuf.pop_back();

        } else if (key >= 32 && key < 256) {
            inputBuf += (char)key;
            // Yazilan ile baslayan ilk ogeyi sec
            for (int i = 0; i < (int)entries.size(); i++) {
                if (entries[i].name.size() >= inputBuf.size() &&
                    entries[i].name.substr(0, inputBuf.size()) == inputBuf) {
                    selected = i;
                    break;
                }
            }
        }
    }
}