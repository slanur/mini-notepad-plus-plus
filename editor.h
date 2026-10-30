#pragma once
// editor.h — Ana editör sınıfı
// Tüm bileşenleri (Buffer, UndoStack, FileManager, Toolbox) bir araya getirir.

#include "buffer.h"
#include "undo.h"
#include "filemanager.h"
#include "toolbox.h"
#include <string>
#include <vector>

// Editör modu
enum class EditorMode {
    Normal,    ///< Normal düzenleme
    Search,    ///< CTRL+F: arama
    SearchNav, ///< CTRL+G: eşleşmeler arası gezinme
    Replace,   ///< CTRL+H: değiştirme (adım 1: eski kelime, adım 2: yeni kelime)
    FileBrowser, ///< Dosya yöneticisi açık
    ExitPrompt,  ///< Çıkış onayı
};

class Editor {
public:
    Editor();
    ~Editor();

    /// Ana döngü; döndüğünde program biter
    void run();

private:
    // ── Bileşenler ───────────────────────────────────────────────────────
    Buffer      buffer_;
    UndoStack   undoStack_;
    FileManager fileManager_;
    Toolbox     toolbox_;

    // ── Durum ────────────────────────────────────────────────────────────
    int cursorLine_ = 0;   ///< İmleç satırı (0-tabanlı)
    int cursorCol_  = 0;   ///< İmleç sütunu (0-tabanlı, bayt)
    int viewportRow_ = 0;  ///< Kaydırma: ekranın hangi satırdan başladığı

    bool      isDirty_   = false; ///< Kaydedilmemiş değişiklik var mı?
    std::string filePath_;        ///< Geçerli dosya yolu (boşsa yeni dosya)

    EditorMode mode_ = EditorMode::Normal;

    // Seçim
    bool selectActive_  = false;
    int  selectLine_    = 0;
    int  selectCol_     = 0;

    // Arama/değiştirme durumu
    std::string searchQuery_;
    std::string replaceQuery_;
    std::string searchInput_; ///< Kullanıcının şu an girdiği metin
    std::vector<Buffer::Match> searchMatches_;
    int searchMatchIdx_ = 0;
    int replaceStep_    = 0; /// 0: eski kelime bekleniyor, 1: yeni kelime bekleniyor

    // Pano (clipboard)
    std::string clipboard_;

    // ── Rendering ────────────────────────────────────────────────────────
    int termRows_ = 24;
    int termCols_ = 80;
    int toolboxHeight_ = 0; ///< Toolbox kaç satır kaplıyor

    void refreshScreen();
    void renderStatusBar();
    void renderTextArea();
    void renderSearchBar();
    void renderExitPrompt();

    // ── Olay işleme ──────────────────────────────────────────────────────
    void handleKey(int key);
    void handleNormalKey(int key);
    void handleSearchKey(int key);
    void handleSearchNavKey(int key);
    void handleReplaceKey(int key);

    // ── Editör komutları ─────────────────────────────────────────────────
    void insertChar(char c);
    void insertString(const std::string &s);
    void deleteCharBefore();
    void deleteCharAt();
    void deleteWordBefore();
    void newLine();

    void moveCursorLeft(bool byWord = false);
    void moveCursorRight(bool byWord = false);
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLineStart();
    void moveCursorLineEnd();

    void startSelection();
    void extendSelection(int newLine, int newCol);
    void clearSelection();
    std::string getSelectedText() const;
    void deleteSelection();

    void copySelection();
    void cutSelection();
    void paste();

    void undo();
    void saveUndoState();

    void openFile();
    void saveFile();
    void saveFileAs();

    void startSearch();
    void startReplace();
    void startSearchNav();
    void highlightMatches();
    void clearSearchHighlights();
    int  jumpToMatch(int idx);

    void adjustViewport();
    void clampCursor();
    int  currentLineLen() const;

    // UTF-8 yardımcı: char cinsinden sütun indeksini ekran sütununa çevir
    int byteToDisplayCol(const std::string &line, int byteIdx) const;
    int displayColToByteIdx(const std::string &line, int displayCol) const;
};
