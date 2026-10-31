// editor.cpp

#include "editor.h"
#include "autosave.h"
#include "pentest.h"
#include "terminal.h"
#include <algorithm>
#include <sstream>
#include <cstring>

// UTF-8 devam bayti mi? (cok baytli karakterin ilk bayti olmayanlar)
static bool isContByte(char c) { return ((unsigned char)c & 0xC0) == 0x80; }

// Disaridan gelen metin: satir sonlari \n olur, sekmeler ekranda hizayi bozmasin diye bosluk olur
static std::string normalizePastedText(const std::string &raw) {
    std::string text;
    text.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) {
        char c = raw[i];
        if (c == '\r') {
            if (i + 1 < raw.size() && raw[i + 1] == '\n') continue;
            text += '\n';
        } else if (c == '\t') {
            text += "    ";
        } else {
            text += c;
        }
    }
    return text;
}

Editor::Editor() {
    Terminal::setUTF8();
    Terminal::enableRawMode();
    Terminal::enableBracketedPaste();
    Terminal::clearScreen();
    Terminal::setCursorBar();
}

Editor::~Editor() {
    Terminal::disableBracketedPaste();
    Terminal::disableRawMode();
    Terminal::clearScreen();
    Terminal::moveCursor(1, 1);
    Terminal::resetColor();
    Terminal::showCursor();
    Terminal::setCursorBlock();
}

void Editor::run() {
    checkRecovery();

    while (true) {
        Terminal::getSize(termRows_, termCols_);
        toolboxHeight_ = toolbox_.height();
        refreshScreen();

        // Tus beklerken de yedek alinabilsin diye okuma dogrudan bloklanmaz
        autosave();
        while (!Terminal::waitForInput(1000)) autosave();

        int key = Terminal::readKey();
        statusMsg_.clear();

        if (key == KEY_PASTE) { handlePaste(); continue; }

        if (mode_ == EditorMode::RecoverPrompt) { handleRecoverKey(key); continue; }
        if (mode_ == EditorMode::Search)     { handleSearchKey(key);    continue; }
        if (mode_ == EditorMode::SearchNav)  { handleSearchNavKey(key); continue; }
        if (mode_ == EditorMode::Replace)    { handleReplaceKey(key);   continue; }
        if (mode_ == EditorMode::ExitPrompt) {
            if (key == 'y' || key == 'Y') {
                saveFile();
                if (!isDirty_) break;       // Kaydedilemediyse notu kaybetme
                mode_ = EditorMode::Normal;
            }
            if (key == 'n' || key == 'N') { break; }
            if (key == KEY_ESC)           { mode_ = EditorMode::Normal; }
            continue;
        }

        // ESC once secimi ve vurgulari temizler; ancak temizlenecek bir sey yoksa cikar
        bool escClearsState = key == KEY_ESC && (selectActive_ || !searchMatches_.empty());

        handleNormalKey(key);

        if (key == KEY_ESC && !escClearsState && mode_ == EditorMode::Normal) break;
    }

    // Buraya yalnizca not kaydedildiyse veya kullanici vazgectiyse gelinir
    clearAutosave();
}

bool Editor::loadFile(const std::string &path, bool createIfMissing) {
    if (!buffer_.loadFromFile(path) && !createIfMissing) {
        statusMsg_ = "Dosya acilamadi";
        return false;
    }
    filePath_ = path; cursorLine_ = 0; cursorCol_ = 0;
    viewportRow_ = 0; viewportCol_ = 0; isDirty_ = false;
    undoStack_.clear(); redoStack_.clear(); searchMatches_.clear();
    return true;
}

// Kaydedilmemis degisiklikleri yedekle; arada metin degismediyse diske dokunma
void Editor::autosave() {
    if (!isDirty_ || mode_ == EditorMode::RecoverPrompt) return;

    std::time_t now = std::time(nullptr);
    if (now - lastAutosaveTime_ < AUTOSAVE_SECONDS) return;
    lastAutosaveTime_ = now;

    std::string text = buffer_.getAllText();
    if (text == lastAutosaveText_) return;

    // Not adi degistiyse eski yedek ortada kalmasin
    std::string path = Autosave::pathFor(filePath_);
    if (path != autosavePath_) Autosave::remove(autosavePath_);
    autosavePath_ = path;

    if (Autosave::write(path, text)) lastAutosaveText_ = text;
}

void Editor::clearAutosave() {
    Autosave::remove(autosavePath_);
    autosavePath_.clear();
    lastAutosaveText_.clear();
}

// Onceki oturumdan kalan yedek varsa geri yuklemeyi sor
void Editor::checkRecovery() {
    std::string path = Autosave::pathFor(filePath_);
    std::string text;
    if (!Autosave::read(path, text)) return;

    autosavePath_ = path;
    if (text == buffer_.getAllText()) { clearAutosave(); return; }

    recoveryText_ = text;
    mode_ = EditorMode::RecoverPrompt;
}

void Editor::handleRecoverKey(int key) {
    if (key == 'y' || key == 'Y') {
        buffer_.setText(recoveryText_);
        lastAutosaveText_ = recoveryText_;
        cursorLine_ = 0; cursorCol_ = 0;
        viewportRow_ = 0; viewportCol_ = 0;
        undoStack_.clear(); redoStack_.clear();
        isDirty_ = true;
        statusMsg_ = "Yedek geri yuklendi, kaydetmek icin Ctrl+S";
    } else if (key == 'n' || key == 'N') {
        clearAutosave();
    } else {
        return;
    }
    recoveryText_.clear();
    mode_ = EditorMode::Normal;
}

// Terminalden yapistirma: tek geri alma adimi; arac ciktilari hizlica eklenir
void Editor::handlePaste() {
    std::string text = normalizePastedText(Terminal::readPaste());

    if (mode_ == EditorMode::Normal) {
        if (text.empty()) return;
        if (selectActive_) deleteSelection();
        saveUndoState();
        insertString(text);
        isDirty_ = true;
    } else if (mode_ == EditorMode::Search || mode_ == EditorMode::Replace) {
        searchInput_ += text.substr(0, text.find('\n'));
    }
}

int Editor::byteToDisplayCol(const std::string &line, int byteIdx) const {
    int col = 0, i = 0;
    while (i < byteIdx && i < (int)line.size()) {
        unsigned char c = (unsigned char)line[i];
        if      (c < 0x80)           { i++;    col++; }
        else if ((c & 0xE0) == 0xC0) { i += 2; col++; }
        else if ((c & 0xF0) == 0xE0) { i += 3; col++; }
        else                          { i++;    col++; }
    }
    return col;
}

int Editor::displayColToByteIdx(const std::string &line, int displayCol) const {
    int col = 0, i = 0;
    while (i < (int)line.size() && col < displayCol) {
        unsigned char c = (unsigned char)line[i];
        if      (c < 0x80)           { i++;    col++; }
        else if ((c & 0xE0) == 0xC0) { i += 2; col++; }
        else if ((c & 0xF0) == 0xE0) { i += 3; col++; }
        else                          { i++;    col++; }
    }
    return i;
}

int Editor::currentLineLen() const {
    LineNode *node = buffer_.lineAt(cursorLine_);
    return node ? (int)node->text.size() : 0;
}

void Editor::clampCursor() {
    if (cursorLine_ < 0) cursorLine_ = 0;
    if (cursorLine_ >= buffer_.lineCount()) cursorLine_ = buffer_.lineCount() - 1;
    int maxCol = currentLineLen();
    if (cursorCol_ < 0)      cursorCol_ = 0;
    if (cursorCol_ > maxCol) cursorCol_ = maxCol;
}

void Editor::adjustViewport() {
    int textAreaRows = termRows_ - toolboxHeight_ - 1;
    if (cursorLine_ < viewportRow_)
        viewportRow_ = cursorLine_;
    else if (cursorLine_ >= viewportRow_ + textAreaRows)
        viewportRow_ = cursorLine_ - textAreaRows + 1;
    if (viewportRow_ < 0) viewportRow_ = 0;

    // Yatay kaydirma: uzun komut ve URL satirlarinda imlec ekranda kalsin
    LineNode *node = buffer_.lineAt(cursorLine_);
    int displayCol = node ? byteToDisplayCol(node->text, cursorCol_) : 0;
    int textCols   = termCols_ - 8;
    if (displayCol < viewportCol_)
        viewportCol_ = displayCol;
    else if (displayCol >= viewportCol_ + textCols)
        viewportCol_ = displayCol - textCols + 1;
    if (viewportCol_ < 0) viewportCol_ = 0;
}

void Editor::refreshScreen() {
    Terminal::hideCursor();
    adjustViewport();
    toolbox_.render(termCols_);
    renderTextArea();
    renderStatusBar();

    if (mode_ == EditorMode::Search || mode_ == EditorMode::Replace ||
        mode_ == EditorMode::SearchNav)
        renderSearchBar();

    if (mode_ == EditorMode::ExitPrompt)
        renderExitPrompt();

    if (mode_ == EditorMode::RecoverPrompt)
        renderRecoverPrompt();

    int textStartRow = toolboxHeight_ + 1;
    int screenRow    = textStartRow + (cursorLine_ - viewportRow_);
    int lineNumWidth = 8;
    std::string curLineText = buffer_.lineAt(cursorLine_) ? buffer_.lineAt(cursorLine_)->text : "";
    int displayCol  = byteToDisplayCol(curLineText, cursorCol_);
    int screenCol   = lineNumWidth + displayCol - viewportCol_ + 1;

    Terminal::moveCursor(screenRow, screenCol);
    Terminal::showCursor();
}

void Editor::renderTextArea() {
    int textStartRow = toolboxHeight_ + 1;
    int textAreaRows = termRows_ - toolboxHeight_ - 1;

    for (int row = 0; row < textAreaRows; row++) {
        int lineIdx   = viewportRow_ + row;
        int screenRow = textStartRow + row;
        Terminal::moveCursor(screenRow, 1);

        if (lineIdx >= buffer_.lineCount()) {
            Terminal::setColor(COLOR_BRIGHT_BLACK, COLOR_DEFAULT);
            std::string empty = "      | ";
            while ((int)empty.size() < termCols_) empty += ' ';
            Terminal::print(empty.substr(0, termCols_));
            Terminal::resetColor();
            continue;
        }

        // Satir numarasi
        Terminal::resetColor();
        Terminal::setColor(COLOR_BRIGHT_BLACK, COLOR_DEFAULT);
        char numBuf[32];
        snprintf(numBuf, sizeof(numBuf), "%5d | ", lineIdx + 1);
        Terminal::print(numBuf);
        Terminal::resetColor();

        LineNode *node = buffer_.lineAt(lineIdx);
        std::string text = node ? node->text : "";
        int textLen = (int)text.size();

        // Her bayt icin renk: 0=normal, 1=arama(mavi), 2=secim(sari), 3+=pentest belirteci
        const int TOKEN_BASE = 3;
        std::vector<int> colorMap(textLen + 1, 0);

        // IP, URL, CVE vb. en altta; arama ve secim bunlarin ustune boyanir
        for (const Pentest::Token &t : Pentest::scanLine(text)) {
            for (int k = t.start; k < t.start + t.len && k < textLen; k++)
                colorMap[k] = TOKEN_BASE + (int)t.type;
        }

        // Arama eslesmelerini isaretle (sadece eslesen kisim)
        for (auto &m : searchMatches_) {
            if (m.line != lineIdx) continue;
            int from = m.col;
            int to   = m.col + m.len;
            if (from < 0) from = 0;
            if (to > textLen) to = textLen;
            for (int k = from; k < to; k++)
                colorMap[k] = 1;
        }

        // Secimi isaretle (aramanin uzerinde)
        if (selectActive_) {
            int sL = selectLine_, sC = selectCol_;
            int eL = cursorLine_,  eC = cursorCol_;
            if (sL > eL || (sL == eL && sC > eC)) { std::swap(sL,eL); std::swap(sC,eC); }
            if (lineIdx >= sL && lineIdx <= eL) {
                int from = (lineIdx == sL) ? sC : 0;
                int to   = (lineIdx == eL) ? eC : textLen;
                if (from < 0) from = 0;
                if (to > textLen) to = textLen;
                for (int k = from; k < to; k++)
                    colorMap[k] = 2;
            }
        }

        // Karakterleri renkli yaz
        // Her renk degisiminde once resetColor() cagrilir, sonra yeni renk set edilir.
        // Bu sayede onceki arka plan rengi temizlenir (asil highlight bug duzeltmesi).
        int screenCol = 8;
        int prevColorType = -1; // -1: henuz set edilmedi

        int i = displayColToByteIdx(text, viewportCol_);
        while (i < textLen && screenCol < termCols_) {
            int ct = colorMap[i];

            // Renk degisti mi?
            if (ct != prevColorType) {
                // Once tamamen sifirla, sonra yeni rengi uygula
                Terminal::resetColor();
                if (ct == 1) {
                    Terminal::setColor(COLOR_WHITE, COLOR_BLUE);   // Arama: mavi bg
                } else if (ct == 2) {
                    Terminal::setColor(COLOR_BLACK, COLOR_YELLOW); // Secim: sari bg
                } else if (ct >= TOKEN_BASE) {
                    Terminal::setColor(Pentest::tokenColor((Pentest::TokenType)(ct - TOKEN_BASE)),
                                       COLOR_DEFAULT);
                } else {
                    Terminal::setColor(COLOR_WHITE, COLOR_DEFAULT); // Normal
                }
                prevColorType = ct;
            }

            // UTF-8 karakteri tamamen yaz (cok baytli karakterler icin)
            unsigned char c0 = (unsigned char)text[i];

            // Kontrol karakterleri ham yazilmaz: hedeften gelen ciktilardaki
            // kaçis kodlari terminali yonetemesin
            if (c0 < 32 || c0 == 127) {
                Terminal::printChar('.');
                i++;
                screenCol++;
                continue;
            }

            int charBytes = 1;
            if      ((c0 & 0xE0) == 0xC0) charBytes = 2;
            else if ((c0 & 0xF0) == 0xE0) charBytes = 3;
            else if ((c0 & 0xF8) == 0xF0) charBytes = 4;

            // Tum baytlari yaz
            for (int b = 0; b < charBytes && i + b < textLen; b++) {
                Terminal::printChar(text[i + b]);
            }
            i += charBytes;
            screenCol++; // Gorsel olarak 1 karakter
        }

        // Satir sonuna kadar bosluklarla doldur (rengi sifirla)
        Terminal::resetColor();
        while (screenCol < termCols_) { Terminal::printChar(' '); screenCol++; }
    }
    Terminal::resetColor();
}

void Editor::renderStatusBar() {
    Terminal::moveCursor(termRows_, 1);
    Terminal::setColor(COLOR_BLACK, COLOR_WHITE);

    std::string dirty = isDirty_ ? "[*] " : "    ";
    std::string fname = filePath_.empty() ? "Yeni Dosya" : filePath_;
    size_t sep = fname.rfind('/');
    if (sep == std::string::npos) sep = fname.rfind('\\');
    if (sep != std::string::npos) fname = fname.substr(sep + 1);

    char posBuf[64];
    snprintf(posBuf, sizeof(posBuf), " Satir:%d  Sutun:%d  Eslesmeler:%d",
             cursorLine_+1, cursorCol_+1, (int)searchMatches_.size());

    std::string bar = dirty + Terminal::sanitize(fname) + posBuf;
    if (!statusMsg_.empty()) bar += "  | " + Terminal::sanitize(statusMsg_);
    while ((int)bar.size() < termCols_) bar += ' ';
    Terminal::print(bar.substr(0, termCols_));
    Terminal::resetColor();
}

void Editor::renderSearchBar() {
    int row = termRows_ - 1;
    Terminal::moveCursor(row, 1);
    Terminal::setColor(COLOR_BLACK, COLOR_CYAN);

    std::string prompt;
    if (mode_ == EditorMode::Search) {
        prompt = " [Ara] " + searchInput_ + "_ (Enter:Ara  ESC:Iptal)";
    } else if (mode_ == EditorMode::SearchNav) {
        char info[64] = "";
        if (!searchMatches_.empty())
            snprintf(info, sizeof(info), " [%d/%d]", searchMatchIdx_+1, (int)searchMatches_.size());
        else
            snprintf(info, sizeof(info), " [Bulunamadi]");
        prompt = std::string(" [Gezin") + info + "] '" + searchQuery_ +
                 "' (Sol/Sag:Onceki/Sonraki  ESC:Cikis)";
    } else if (mode_ == EditorMode::Replace) {
        if (replaceStep_ == 0)
            prompt = " [Degistir] Eski kelime: " + searchInput_ + "_  (Enter:Ileri  ESC:Iptal)";
        else
            prompt = " [Degistir] Yeni kelime: " + searchInput_ + "_  (Enter:Degistir  ESC:Iptal)";
    }

    while ((int)prompt.size() < termCols_) prompt += ' ';
    Terminal::print(prompt.substr(0, termCols_));
    Terminal::resetColor();
}

void Editor::renderExitPrompt() {
    Terminal::moveCursor(termRows_-1, 1);
    Terminal::setColor(COLOR_WHITE, COLOR_RED);
    std::string msg = " Kaydedilmemis degisiklik var. Kaydetmek istiyor musunuz? (y/n/ESC) ";
    while ((int)msg.size() < termCols_) msg += ' ';
    Terminal::print(msg.substr(0, termCols_));
    Terminal::resetColor();
}

void Editor::renderRecoverPrompt() {
    Terminal::moveCursor(termRows_-1, 1);
    Terminal::setColor(COLOR_BLACK, COLOR_YELLOW);
    std::string msg = " Kaydedilmemis bir yedek bulundu. Geri yuklensin mi? (y/n) ";
    while ((int)msg.size() < termCols_) msg += ' ';
    Terminal::print(msg.substr(0, termCols_));
    Terminal::resetColor();
}

// Normal mod
void Editor::handleNormalKey(int key) {
    switch (key) {
        case KEY_CTRL_O: openFile();    return;
        case KEY_CTRL_S: saveFile();    return;
        case KEY_CTRL_W: saveFileAs();  return;
        case KEY_CTRL_F: startSearch();    return;
        case KEY_CTRL_G: startSearchNav(); return;
        case KEY_CTRL_H_KEY: startReplace(); return;
        case KEY_CTRL_Z: undo(); return;
        case KEY_CTRL_Y: redo(); return;
        case KEY_CTRL_K:
        case KEY_F1:     showHelp(); return;
        case KEY_CTRL_C: copySelection(); return;
        case KEY_CTRL_X: cutSelection();  return;
        case KEY_CTRL_V: paste();         return;
        case KEY_CTRL_T: insertTemplate();  return;
        case KEY_CTRL_D: insertTimestamp(); return;
        case KEY_CTRL_L: showSummary();     return;

        case KEY_TAB:
            if (selectActive_) deleteSelection();
            saveUndoState(); insertString("    "); isDirty_ = true; return;

        case KEY_UP:
            if (selectActive_) clearSelection();
            moveCursorUp(); return;
        case KEY_DOWN:
            if (selectActive_) clearSelection();
            moveCursorDown(); return;
        case KEY_LEFT:
            if (selectActive_) clearSelection();
            moveCursorLeft(false); return;
        case KEY_RIGHT:
            if (selectActive_) clearSelection();
            moveCursorRight(false); return;

        // CTRL+Yon: tek karakter ilerleme (byWord=false), kelime atlamaz
        case KEY_CTRL_LEFT:
            if (selectActive_) clearSelection();
            moveCursorLeft(false); return;
        case KEY_CTRL_RIGHT:
            if (selectActive_) clearSelection();
            moveCursorRight(false); return;

        // CTRL+SHIFT+Yon: kelime bazli secim (byWord=true)
        case KEY_CTRL_SHIFT_LEFT:
            if (!selectActive_) startSelection();
            moveCursorLeft(true);
            extendSelection(cursorLine_, cursorCol_); return;
        case KEY_CTRL_SHIFT_RIGHT:
            if (!selectActive_) startSelection();
            moveCursorRight(true);
            extendSelection(cursorLine_, cursorCol_); return;

        // SHIFT+Yon: karakter bazli secim (byWord=false)
        case KEY_SHIFT_LEFT:
            if (!selectActive_) startSelection();
            moveCursorLeft(false);
            extendSelection(cursorLine_, cursorCol_); return;
        case KEY_SHIFT_RIGHT:
            if (!selectActive_) startSelection();
            moveCursorRight(false);
            extendSelection(cursorLine_, cursorCol_); return;

        case KEY_HOME:
            if (selectActive_) clearSelection();
            moveCursorLineStart(); return;
        case KEY_END:
            if (selectActive_) clearSelection();
            moveCursorLineEnd(); return;
        case KEY_PAGE_UP:
            if (selectActive_) clearSelection();
            for (int i = 0; i < termRows_/2; i++) moveCursorUp(); return;
        case KEY_PAGE_DOWN:
            if (selectActive_) clearSelection();
            for (int i = 0; i < termRows_/2; i++) moveCursorDown(); return;

        case KEY_BACKSPACE:
            if (selectActive_) { deleteSelection(); return; }
            saveUndoState(); deleteCharBefore(); isDirty_ = true; return;
        case KEY_CTRL_BACKSPACE:
            if (selectActive_) { deleteSelection(); return; }
            saveUndoState(); deleteWordBefore(); isDirty_ = true; return;
        case KEY_DELETE:
            if (selectActive_) { deleteSelection(); return; }
            saveUndoState(); deleteCharAt(); isDirty_ = true; return;
        case KEY_ENTER:
            if (selectActive_) deleteSelection();
            saveUndoState(); newLine(); isDirty_ = true; return;
        case KEY_ESC:
            if (selectActive_)         { clearSelection(); return; }
            if (!searchMatches_.empty()) { searchMatches_.clear(); return; }
            if (isDirty_) { mode_ = EditorMode::ExitPrompt; }
            return;
        default:
            if (key >= 32 && key < 256) {
                if (selectActive_) deleteSelection();
                // Cok baytli karakterin devam baytlari ayni geri alma adimina girer
                if (!isContByte((char)key)) saveUndoState();
                insertChar((char)key); isDirty_ = true;
            }
            return;
    }
}

// Ctrl+F: Arama modu — kullanici kelime girer, Enter'a basinca arar ve SearchNav moduna gecer
void Editor::handleSearchKey(int key) {
    if (key == KEY_ESC) {
        mode_ = EditorMode::Normal;
        searchInput_.clear();
        return;
    }
    if (key == KEY_ENTER) {
        if (searchInput_.empty()) { mode_ = EditorMode::Normal; return; }
        searchQuery_    = searchInput_;
        searchMatches_  = buffer_.findAll(searchQuery_, false);
        searchMatchIdx_ = 0;
        if (!searchMatches_.empty()) {
            cursorLine_ = searchMatches_[0].line;
            cursorCol_  = searchMatches_[0].col;
        }
        searchInput_.clear();
        // Arama bittikten sonra direkt SearchNav moduna gec; kullanici ok tuslariyla gezinebilir
        mode_ = EditorMode::SearchNav;
        return;
    }
    if (key == KEY_BACKSPACE) {
        if (!searchInput_.empty()) searchInput_.pop_back();
        return;
    }
    if (key >= 32 && key < 256) {
        searchInput_ += (char)key;
    }
}

// Ctrl+G: Eslesmeler arasi gezinme
void Editor::handleSearchNavKey(int key) {
    if (key == KEY_ESC || key == KEY_ENTER) {
        mode_ = EditorMode::Normal;
        return;
    }
    if (searchMatches_.empty()) return;
    if (key == KEY_RIGHT || key == KEY_DOWN) {
        searchMatchIdx_ = (searchMatchIdx_ + 1) % (int)searchMatches_.size();
        jumpToMatch(searchMatchIdx_);
    } else if (key == KEY_LEFT || key == KEY_UP) {
        searchMatchIdx_ = (searchMatchIdx_ - 1 + (int)searchMatches_.size()) % (int)searchMatches_.size();
        jumpToMatch(searchMatchIdx_);
    }
}

// Ctrl+R (eski Ctrl+H): 2 adimli degistirme
void Editor::handleReplaceKey(int key) {
    if (key == KEY_ESC) {
        mode_ = EditorMode::Normal;
        searchInput_.clear();
        searchQuery_.clear();
        replaceStep_ = 0;
        return;
    }
    if (key == KEY_BACKSPACE) {
        if (!searchInput_.empty()) searchInput_.pop_back();
        return;
    }
    if (key == KEY_ENTER) {
        if (replaceStep_ == 0) {
            if (searchInput_.empty()) return;
            searchQuery_ = searchInput_;
            searchInput_.clear();
            replaceStep_ = 1;
        } else {
            std::string newWord = searchInput_;
            saveUndoState();
            int count = buffer_.replaceAll(searchQuery_, newWord, false);
            if (count > 0) { isDirty_ = true; }
            searchMatches_.clear();
            searchQuery_.clear();
            mode_ = EditorMode::Normal;
            searchInput_.clear();
            replaceStep_ = 0;
        }
        return;
    }
    if (key >= 32 && key < 256) {
        searchInput_ += (char)key;
    }
}

// Duzenleme
void Editor::insertChar(char c) {
    buffer_.insertChar(cursorLine_, cursorCol_, c);
    cursorCol_++;
    clampCursor();
}

void Editor::insertString(const std::string &s) {
    if (s.find('\n') == std::string::npos) {
        buffer_.insertString(cursorLine_, cursorCol_, s);
        cursorCol_ += (int)s.size();
    } else {
        std::istringstream ss(s);
        std::string part;
        bool first = true;
        while (std::getline(ss, part)) {
            if (!first) newLine();
            buffer_.insertString(cursorLine_, cursorCol_, part);
            cursorCol_ += (int)part.size();
            first = false;
        }
    }
    clampCursor();
}

void Editor::deleteCharBefore() {
    int nl, nc;
    buffer_.deleteCharBefore(cursorLine_, cursorCol_, nl, nc);
    cursorLine_ = nl; cursorCol_ = nc;
}

void Editor::deleteCharAt() {
    buffer_.deleteCharAt(cursorLine_, cursorCol_);
    clampCursor();
}

void Editor::deleteWordBefore() {
    int nl, nc;
    buffer_.deleteWordBefore(cursorLine_, cursorCol_, nl, nc);
    cursorLine_ = nl; cursorCol_ = nc;
}

void Editor::newLine() {
    buffer_.splitLine(cursorLine_, cursorCol_);
    cursorLine_++;
    cursorCol_ = 0;
}

// Imleç hareketi
void Editor::moveCursorLeft(bool byWord) {
    if (byWord) {
        if (cursorCol_ == 0) {
            if (cursorLine_ > 0) { cursorLine_--; cursorCol_ = currentLineLen(); }
            return;
        }
        LineNode *node = buffer_.lineAt(cursorLine_);
        if (!node) return;
        const std::string &t = node->text;
        int pos = cursorCol_;
        while (pos > 0 && t[pos-1] == ' ') pos--;
        while (pos > 0 && t[pos-1] != ' ') pos--;
        cursorCol_ = pos;
    } else {
        if (cursorCol_ > 0) {
            const std::string &t = buffer_.lineAt(cursorLine_)->text;
            cursorCol_--;
            while (cursorCol_ > 0 && isContByte(t[cursorCol_])) cursorCol_--;
        } else if (cursorLine_ > 0) { cursorLine_--; cursorCol_ = currentLineLen(); }
    }
    clampCursor();
}

void Editor::moveCursorRight(bool byWord) {
    if (byWord) {
        LineNode *node = buffer_.lineAt(cursorLine_);
        if (!node) return;
        const std::string &t = node->text;
        int pos = cursorCol_;
        if (pos >= (int)t.size()) {
            if (cursorLine_ < buffer_.lineCount()-1) { cursorLine_++; cursorCol_ = 0; }
            return;
        }
        while (pos < (int)t.size() && t[pos] != ' ') pos++;
        while (pos < (int)t.size() && t[pos] == ' ') pos++;
        cursorCol_ = pos;
    } else {
        int len = currentLineLen();
        if (cursorCol_ < len) {
            const std::string &t = buffer_.lineAt(cursorLine_)->text;
            cursorCol_++;
            while (cursorCol_ < len && isContByte(t[cursorCol_])) cursorCol_++;
        } else if (cursorLine_ < buffer_.lineCount()-1) { cursorLine_++; cursorCol_ = 0; }
    }
    clampCursor();
}

void Editor::moveCursorUp()        { if (cursorLine_ > 0) { cursorLine_--; clampCursor(); } }
void Editor::moveCursorDown()      { if (cursorLine_ < buffer_.lineCount()-1) { cursorLine_++; clampCursor(); } }
void Editor::moveCursorLineStart() { cursorCol_ = 0; }
void Editor::moveCursorLineEnd()   { cursorCol_ = currentLineLen(); }

// Secim
void Editor::startSelection() {
    selectActive_ = true;
    selectLine_   = cursorLine_;
    selectCol_    = cursorCol_;
}

void Editor::extendSelection(int newLine, int newCol) {
    cursorLine_ = newLine; cursorCol_ = newCol; clampCursor();
}

void Editor::clearSelection() { selectActive_ = false; }

std::string Editor::getSelectedText() const {
    if (!selectActive_) return "";
    int sL = selectLine_, sC = selectCol_;
    int eL = cursorLine_,  eC = cursorCol_;
    if (sL > eL || (sL == eL && sC > eC)) { std::swap(sL,eL); std::swap(sC,eC); }
    return buffer_.getRange(sL, sC, eL, eC);
}

void Editor::deleteSelection() {
    if (!selectActive_) return;
    saveUndoState();
    int sL = selectLine_, sC = selectCol_;
    int eL = cursorLine_,  eC = cursorCol_;
    if (sL > eL || (sL == eL && sC > eC)) { std::swap(sL,eL); std::swap(sC,eC); }
    int nl, nc;
    buffer_.deleteRange(sL, sC, eL, eC, nl, nc);
    cursorLine_ = nl; cursorCol_ = nc;
    clearSelection();
    isDirty_ = true;
}

// Secim yoksa bulunulan satir kopyalanir; "$ " oneki diger terminale yapistirilacak
// komuta dahil olmasin diye atilir
void Editor::copySelection() {
    std::string text = getSelectedText();
    if (!selectActive_) {
        LineNode *node = buffer_.lineAt(cursorLine_);
        if (node) text = node->text;
        if (text.compare(0, 2, "$ ") == 0) text.erase(0, 2);
    }
    if (text.empty()) return;

    clipboard_ = text;
    statusMsg_ = Terminal::copyToSystemClipboard(text) ? "Panoya kopyalandi"
                                                       : "Yalnizca editor panosuna kopyalandi";
}

void Editor::cutSelection() {
    if (!selectActive_) return;
    copySelection();
    deleteSelection();
}

void Editor::paste() {
    std::string text;
    if (!Terminal::readSystemClipboard(text) || text.empty()) text = clipboard_;
    text = normalizePastedText(text);
    if (text.empty()) return;

    if (selectActive_) deleteSelection();
    saveUndoState();
    insertString(text);
    isDirty_ = true;
}

void Editor::saveUndoState() {
    undoStack_.push(buffer_.getAllText(), cursorLine_, cursorCol_);
    redoStack_.clear(); // Yeni degisiklik yineleme gecmisini gecersiz kilar
}

void Editor::restoreState(const UndoState &state) {
    buffer_.setText(state.text);
    cursorLine_ = state.cursorLine;
    cursorCol_  = state.cursorCol;
    clampCursor();
    isDirty_ = true;
}

void Editor::undo() {
    if (!undoStack_.canUndo()) return;
    redoStack_.push(buffer_.getAllText(), cursorLine_, cursorCol_);
    restoreState(undoStack_.pop());
}

void Editor::redo() {
    if (!redoStack_.canUndo()) return;
    undoStack_.push(buffer_.getAllText(), cursorLine_, cursorCol_);
    restoreState(redoStack_.pop());
}

void Editor::openFile() {
    if (isDirty_) { statusMsg_ = "Kaydedilmemis degisiklik var, once Ctrl+S"; return; }
    std::string path = fileManager_.show("open");
    Terminal::clearScreen();
    if (!path.empty() && loadFile(path)) checkRecovery();
}

void Editor::saveFile() {
    if (filePath_.empty()) { saveFileAs(); return; }
    if (buffer_.saveToFile(filePath_)) {
        isDirty_ = false;
        statusMsg_ = "Kaydedildi";
        clearAutosave();
    } else {
        statusMsg_ = "Kaydedilemedi!";
    }
}

void Editor::saveFileAs() {
    std::string path = fileManager_.show("save");
    Terminal::clearScreen();
    if (!path.empty()) { filePath_ = path; saveFile(); }
}

// Ctrl+T: secilen not sablonunu imlecin bulundugu yere ekle
void Editor::insertTemplate() {
    int idx = Pentest::pickTemplate();
    Terminal::clearScreen();
    if (idx < 0) return;
    if (selectActive_) deleteSelection();
    saveUndoState();
    if (cursorCol_ > 0) newLine();
    insertString(Pentest::renderTemplate(idx));
    isDirty_ = true;
}

// Ctrl+D: test gunlugu icin zaman damgasi ekle
void Editor::insertTimestamp() {
    if (selectActive_) deleteSelection();
    saveUndoState();
    insertString("[" + Pentest::timestamp() + "] ");
    isDirty_ = true;
}

// Ctrl+K / F1: tum kisayollar
void Editor::showHelp() {
    Pentest::showList(" Klavye Kisayollari", " [yukari/asagi] Gez  [ESC] Geri", toolbox_.helpRows());
    Terminal::clearScreen();
}

// Ctrl+L: nottaki bulgu, IP, URL vb. listesi; secilen kaydin satirina gider
void Editor::showSummary() {
    std::vector<std::string> lines;
    for (LineNode *node = buffer_.firstLine(); node; node = node->next)
        lines.push_back(node->text);

    int target = Pentest::showSummary(lines);
    Terminal::clearScreen();
    if (target >= 0) {
        clearSelection();
        cursorLine_ = target;
        cursorCol_  = 0;
        clampCursor();
    }
}

void Editor::startSearch() {
    mode_ = EditorMode::Search;
    searchInput_.clear();
}

// Ctrl+G: Eger onceki bir arama varsa direkt gezinme moduna gec.
// Yoksa once arama modunu ac (Ctrl+F gibi), kullanici Enter'a basinca SearchNav'a gecer.
void Editor::startSearchNav() {
    if (!searchQuery_.empty()) {
        // Onceki arama sorgusu mevcut: eslesmeler listesini guncelle ve gezinmeye gec
        searchMatches_  = buffer_.findAll(searchQuery_, false);
        searchMatchIdx_ = 0;
        if (!searchMatches_.empty()) jumpToMatch(0);
        mode_ = EditorMode::SearchNav;
    } else {
        // Hic arama yapilmamis: once arama modunu ac
        startSearch();
    }
}

void Editor::startReplace() {
    mode_        = EditorMode::Replace;
    searchInput_.clear();
    searchQuery_.clear();
    replaceStep_ = 0;
}

int Editor::jumpToMatch(int idx) {
    if (idx < 0 || idx >= (int)searchMatches_.size()) return -1;
    cursorLine_ = searchMatches_[idx].line;
    cursorCol_  = searchMatches_[idx].col;
    adjustViewport();
    return idx;
}