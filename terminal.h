#pragma once
// terminal.h — Platform-bağımsız terminal giriş/çıkış soyutlama katmanı
// Mac (POSIX/termios) ve Windows (Windows Console API) farkını burada kapıyoruz.

#include <string>

// ── Özel tuş kodları (platform-bağımsız) ──────────────────────────────────
enum Key {
    KEY_NONE       = 0,
    KEY_CTRL_C     = 3,
    KEY_CTRL_F     = 6,
    KEY_CTRL_G     = 7,
    KEY_CTRL_H_KEY = 8,   // CTRL+H (arama-değiştirme); Backspace ile çakışmayı önlemek için H_KEY
    KEY_TAB        = 9,
    KEY_ENTER      = 10,
    KEY_CTRL_V     = 22,
    KEY_CTRL_X     = 24,
    KEY_CTRL_Y     = 25,
    KEY_CTRL_Z     = 26,
    KEY_ESC        = 27,
    KEY_BACKSPACE  = 127,

    // Genişletilmiş tuşlar (>127)
    KEY_UP         = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CTRL_LEFT,
    KEY_CTRL_RIGHT,
    KEY_CTRL_UP,
    KEY_CTRL_DOWN,
    KEY_SHIFT_LEFT,
    KEY_SHIFT_RIGHT,
    KEY_CTRL_SHIFT_LEFT,
    KEY_CTRL_SHIFT_RIGHT,
    KEY_DELETE,
    KEY_CTRL_BACKSPACE,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_F1,
    KEY_CTRL_S,   // Kaydet
    KEY_CTRL_O,   // Aç
    KEY_CTRL_W,   // Farklı kaydet
    KEY_CTRL_T,   // Şablon ekle
    KEY_CTRL_D,   // Zaman damgası ekle
    KEY_CTRL_L,   // Özet paneli
    KEY_CTRL_K,   // Yardım ekranı
    KEY_PASTE,    // Terminal yapıştırması başladı (metni readPaste() okur)
};

// ── Renk sabitleri ────────────────────────────────────────────────────────
enum Color {
    COLOR_DEFAULT = -1,
    COLOR_BLACK   =  0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BRIGHT_BLACK,
    COLOR_BRIGHT_WHITE,
};

// ── Terminal API ──────────────────────────────────────────────────────────
namespace Terminal {

/// Ham modu etkinleştir (karakter karakter okuma, echo kapalı)
void enableRawMode();

/// Ham modu kapat, terminali eski haline getir
void disableRawMode();

/// Tek tuş oku (bloke eder). Genişletilmiş tuşlar için Key enum değeri döner.
int readKey();

/// En fazla timeoutMs kadar girdi bekle; girdi geldiyse true döner
bool waitForInput(int timeoutMs);

/// Bracketed paste: terminal yapıştırılan metni işaretler, tek işlem olarak okunur
void enableBracketedPaste();
void disableBracketedPaste();

/// KEY_PASTE sonrası yapıştırılan metni bitiş işaretine kadar oku
std::string readPaste();

/// İmleci (row, col) koordinatına taşı. 1-tabanlı.
void moveCursor(int row, int col);

/// Terminali temizle
void clearScreen();

/// Bir satırı sonu dahil temizle
void clearLine();

/// Ön plan + arka plan rengini ayarla (ANSI)
void setColor(Color fg, Color bg = COLOR_DEFAULT);

/// Rengi sıfırla
void resetColor();

/// Terminal boyutunu al
void getSize(int &rows, int &cols);

/// İmleci gizle / göster
void hideCursor();
void showCursor();

/// Yanıp sönen dikey çizgi imleci (cursor block <-> bar)
void setCursorBar();
void setCursorBlock();

/// Sistem panosuna yaz / panodan oku. Pano aracı yoksa veya Windows'ta false döner.
/// macOS: pbcopy/pbpaste, Linux: wl-clipboard veya xclip
bool copyToSystemClipboard(const std::string &text);
bool readSystemClipboard(std::string &text);

/// Kontrol karakterlerini '.' ile değiştir (dosya içeriğindeki kaçış kodları terminale ulaşmasın)
std::string sanitize(const std::string &s);

/// Ham string yaz (renk/pozisyon zaten ayarlandıktan sonra)
void print(const std::string &s);
void printChar(char c);

/// Türkçe karakter desteği: terminali UTF-8 moduna al
void setUTF8();

} // namespace Terminal
