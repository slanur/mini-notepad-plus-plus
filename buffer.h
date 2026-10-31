#pragma once
// buffer.h — Çift yönlü bağlı liste (doubly linked list) tabanlı metin tamponu
// Her SATIR bir Node'dur. Satır içindeki karakterler std::string ile tutulur.
// Proje şartı: "metin karakterlerinin her biri seçeceğiniz bir veri yapısı kullanılarak tutulacaktır."
// Tasarım: satır bazında doubly-linked list; karakter bazında std::string (gap-buffer benzeri)

#include <string>
#include <vector>

// ── Satır düğümü ──────────────────────────────────────────────────────────
struct LineNode {
    std::string text;   ///< Satırdaki UTF-8 metin
    LineNode *prev = nullptr;
    LineNode *next = nullptr;

    explicit LineNode(const std::string &t = "") : text(t) {}
};

// ── Metin tamponu ─────────────────────────────────────────────────────────
class Buffer {
public:
    Buffer();
    ~Buffer();

    // Tüm içeriği sil, başa dön
    void clear();

    // İçeriği dosyadan yükle (her \n yeni Node)
    bool loadFromFile(const std::string &path);

    // Tüm içeriği dosyaya yaz
    bool saveToFile(const std::string &path) const;

    // İçeriği verilen metinle değiştir (getAllText'in tersi; geri alma ve kurtarma için)
    void setText(const std::string &text);

    // ── Navigasyon ────────────────────────────────────────────────────────
    int  lineCount() const { return lineCount_; }
    LineNode *firstLine() const { return head_; }
    LineNode *lastLine()  const { return tail_; }

    /// lineIndex (0-tabanlı) satırına pointer döner; yavaş ama küçük dosyalarda sorun yok
    LineNode *lineAt(int index) const;

    // ── Düzenleme ─────────────────────────────────────────────────────────

    /// (lineIdx, colIdx) konumuna karakter ekle
    void insertChar(int lineIdx, int colIdx, char c);

    /// (lineIdx, colIdx) konumuna UTF-8 string ekle (yapıştırma vb.)
    void insertString(int lineIdx, int colIdx, const std::string &s);

    /// (lineIdx, colIdx) konumundaki karakteri sil (Backspace semantiği)
    /// Eğer colIdx == 0 ise bir önceki satıra birleştir
    void deleteCharBefore(int lineIdx, int colIdx, int &newLineIdx, int &newColIdx);

    /// (lineIdx, colIdx) konumundaki karakteri sil (Delete tuşu semantiği)
    void deleteCharAt(int lineIdx, int colIdx);

    /// (lineIdx, colIdx) konumunda Enter: satırı böl
    void splitLine(int lineIdx, int colIdx);

    /// lineIdx satırındaki colIdx'den kelime sonuna kadar sil (Ctrl+Backspace)
    void deleteWordBefore(int lineIdx, int colIdx, int &newLineIdx, int &newColIdx);

    /// Belirtilen aralığı sil (seçim silme / kes)
    void deleteRange(int startLine, int startCol, int endLine, int endCol,
                     int &newLineIdx, int &newColIdx);

    /// Seçili alanın metnini döndür
    std::string getRange(int startLine, int startCol, int endLine, int endCol) const;

    /// Tüm metni tek string olarak döndür (arama/kaydetme için)
    std::string getAllText() const;

    // ── Arama ────────────────────────────────────────────────────────────
    struct Match {
        int line, col, len;
    };

    /// KMP tabanlı tam metin arama; tüm eşleşmeleri döndür
    std::vector<Match> findAll(const std::string &pattern, bool caseSensitive = false) const;

    /// replaceAll: tüm eşleşmeleri newText ile değiştir
    int replaceAll(const std::string &pattern, const std::string &replacement,
                   bool caseSensitive = false);

private:
    LineNode *head_     = nullptr;
    LineNode *tail_     = nullptr;
    int       lineCount_ = 0;

    LineNode *appendLine(const std::string &text = "");
    void      insertLineAfter(LineNode *node, const std::string &text = "");
    void      removeLine(LineNode *node);
    void      mergeWithNext(LineNode *node); // node ve node->next'i birleştir

    // KMP yardımcı
    static std::vector<int> buildKMPTable(const std::string &pattern);
};
