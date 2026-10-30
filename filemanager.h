#pragma once
// filemanager.h — Konsol tabanlı dosya yöneticisi
// Dosya Aç / Kaydet / Farklı Kaydet ekranları burada yönetilir.

#include <string>
#include <vector>

// ── Dosya/klasör girişi ────────────────────────────────────────────────────
struct FsEntry {
    std::string name;
    bool isDir;
};

// ── Dosya Yöneticisi ──────────────────────────────────────────────────────
class FileManager {
public:
    FileManager();

    /// Dosya seçme diyaloğunu göster.
    /// mode: "open" → mevcut dosya seç; "save" → dosya adı gir
    /// Seçilen tam yolu döndürür; iptal edilirse boş string.
    std::string show(const std::string &mode, const std::string &startDir = "");

private:
    std::string currentDir_;

    // Geçerli dizindeki dosya ve klasörleri listele
    std::vector<FsEntry> listDir(const std::string &dir) const;

    // Yolu normalize et (. ve .. çöz)
    std::string normalizePath(const std::string &path) const;

    // Üst dizine çık (platform uyumlu)
    void goUp();

    // Ekranı çiz
    void render(const std::vector<FsEntry> &entries, int selected,
                const std::string &inputBuf, const std::string &mode) const;
};