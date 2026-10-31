#pragma once
// editor.h — Ana editör sınıfı
// Tüm bileşenleri (Buffer, UndoStack, FileManager, Toolbox) bir araya getirir.

#include "buffer.h"
#include "undo.h"
#include "filemanager.h"
#include "toolbox.h"
#include <ctime>
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
    RecoverPrompt, ///< Önceki oturumun yedeği bulundu, geri yükleme onayı
};

class Editor {
public:
    Editor();
    ~Editor();

    /// Ana döngü; döndüğünde program biter
    void run();

    /// Dosyayı tampona yükle. createIfMissing ise olmayan dosya için bu adla yeni not başlar.
    bool loadFile(const std::string &path, bool createIfMissing = false);

private:
    // ── Bileşenler ───────────────────────────────────────────────────────
    Buffer      buffer_;
    UndoStack   undoStack_;
    UndoStack   redoStack_;
    FileManager fileManager_;
    Toolbox     toolbox_;

    // ── Durum ────────────────────────────────────────────────────────────
    int cursorLine_ = 0;   ///< İmleç satırı (0-tabanlı)
    int cursorCol_  = 0;   ///< İmleç sütunu (0-tabanlı, bayt)
    int viewportRow_ = 0;  ///< Kaydırma: ekranın hangi satırdan başladığı
    int viewportCol_ = 0;  ///< Yatay kaydırma: ekranın hangi sütundan başladığı

    bool      isDirty_   = false; ///< Kaydedilmemiş değişiklik var mı?
    std::string filePath_;        ///< Geçerli dosya yolu (boşsa yeni dosya)
    std::string statusMsg_;       ///< Durum çubuğunda bir tuş boyunca görünen mesaj

    // Otomatik yedek
    static const int AUTOSAVE_SECONDS = 5; ///< Yedekler arası en az süre
    std::string autosavePath_;      ///< Yazılan son yedeğin yolu
    std::string lastAutosaveText_;  ///< Yedekteki metin; değişmediyse yeniden yazılmaz
    std::string recoveryText_;      ///< Geri yükleme onayı bekleyen yedek
    std::time_t lastAutosaveTime_ = 0;

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
    void renderRecoverPrompt();

    // ── Olay işleme ──────────────────────────────────────────────────────
    void handleKey(int key);
    void handleNormalKey(int key);
    void handleSearchKey(int key);
    void handleSearchNavKey(int key);
    void handleReplaceKey(int key);
    void handlePaste();
    void handleRecoverKey(int key);

    // ── Otomatik yedek ───────────────────────────────────────────────────
    void autosave();
    void clearAutosave();
    void checkRecovery();

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
    void redo();
    void restoreState(const UndoState &state);
    void saveUndoState();

    void openFile();
    void saveFile();
    void saveFileAs();

    // ── Pentest notları ──────────────────────────────────────────────────
    void insertTemplate();
    void insertTimestamp();
    void showSummary();
    void showHelp();

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
