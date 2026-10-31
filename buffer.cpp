// buffer.cpp — Çift yönlü bağlı liste ile metin tamponu implementasyonu

#include "buffer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>

// ── Kurucu / Yıkıcı ─────────────────────────────────────────────────────
Buffer::Buffer() {
    // Başlangıçta tek boş satır
    appendLine("");
}

Buffer::~Buffer() {
    clear();
}

void Buffer::clear() {
    LineNode *cur = head_;
    while (cur) {
        LineNode *nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head_ = tail_ = nullptr;
    lineCount_ = 0;
}

// ── Yardımcı: Satır ekleme ───────────────────────────────────────────────
LineNode *Buffer::appendLine(const std::string &text) {
    LineNode *node = new LineNode(text);
    if (!head_) {
        head_ = tail_ = node;
    } else {
        node->prev = tail_;
        tail_->next = node;
        tail_ = node;
    }
    lineCount_++;
    return node;
}

void Buffer::insertLineAfter(LineNode *node, const std::string &text) {
    LineNode *newNode = new LineNode(text);
    newNode->prev = node;
    newNode->next = node->next;
    if (node->next) node->next->prev = newNode;
    else tail_ = newNode;
    node->next = newNode;
    lineCount_++;
}

void Buffer::removeLine(LineNode *node) {
    if (node->prev) node->prev->next = node->next;
    else head_ = node->next;
    if (node->next) node->next->prev = node->prev;
    else tail_ = node->prev;
    delete node;
    lineCount_--;
}

void Buffer::mergeWithNext(LineNode *node) {
    if (!node || !node->next) return;
    node->text += node->next->text;
    removeLine(node->next);
}

// ── Dosya İşlemleri ──────────────────────────────────────────────────────
bool Buffer::loadFromFile(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    clear();

    std::string line;
    bool first = true;
    while (std::getline(ifs, line)) {
        // Windows CRLF temizle
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (first) { first = false; }
        appendLine(line);
    }
    if (lineCount_ == 0) appendLine(""); // Boş dosya
    return true;
}

bool Buffer::saveToFile(const std::string &path) const {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    LineNode *cur = head_;
    while (cur) {
        ofs << cur->text;
        if (cur->next) ofs << '\n';
        cur = cur->next;
    }
    return true;
}

void Buffer::setText(const std::string &text) {
    clear();
    size_t start = 0;
    while (true) {
        size_t newline = text.find('\n', start);
        if (newline == std::string::npos) {
            appendLine(text.substr(start));
            return;
        }
        appendLine(text.substr(start, newline - start));
        start = newline + 1;
    }
}

// ── Navigasyon ───────────────────────────────────────────────────────────
LineNode *Buffer::lineAt(int index) const {
    if (index < 0 || index >= lineCount_) return nullptr;
    // Ortadan başla (küçük optimizasyon)
    if (index < lineCount_ / 2) {
        LineNode *cur = head_;
        for (int i = 0; i < index; i++) cur = cur->next;
        return cur;
    } else {
        LineNode *cur = tail_;
        for (int i = lineCount_ - 1; i > index; i--) cur = cur->prev;
        return cur;
    }
}

// ── Düzenleme ────────────────────────────────────────────────────────────
void Buffer::insertChar(int lineIdx, int colIdx, char c) {
    LineNode *node = lineAt(lineIdx);
    if (!node) return;
    // Sınır kontrolü
    if (colIdx < 0) colIdx = 0;
    if (colIdx > (int)node->text.size()) colIdx = (int)node->text.size();
    node->text.insert(colIdx, 1, c);
}

void Buffer::insertString(int lineIdx, int colIdx, const std::string &s) {
    // Çok satırlı yapıştırma desteği
    if (s.find('\n') == std::string::npos) {
        LineNode *node = lineAt(lineIdx);
        if (!node) return;
        if (colIdx < 0) colIdx = 0;
        if (colIdx > (int)node->text.size()) colIdx = (int)node->text.size();
        node->text.insert(colIdx, s);
        return;
    }
    // Satır satır ayır ve ekle
    std::istringstream ss(s);
    std::string part;
    int curLine = lineIdx;
    int curCol  = colIdx;
    bool first = true;
    while (std::getline(ss, part)) {
        if (!first) {
            splitLine(curLine, curCol);
            curLine++;
            curCol = 0;
        }
        insertString(curLine, curCol, part);
        curCol += (int)part.size();
        first = false;
    }
}

void Buffer::deleteCharBefore(int lineIdx, int colIdx, int &newLineIdx, int &newColIdx) {
    LineNode *node = lineAt(lineIdx);
    if (!node) { newLineIdx = lineIdx; newColIdx = colIdx; return; }

    if (colIdx > 0) {
        // UTF-8: birden fazla bayt olabilir, geriye doğru git
        int pos = colIdx - 1;
        // UTF-8 multibyte: bayt 0x80-0xBF devam baytıdır
        while (pos > 0 && (unsigned char)node->text[pos] >= 0x80 &&
               (unsigned char)node->text[pos] <= 0xBF) {
            pos--;
        }
        node->text.erase(pos, colIdx - pos);
        newLineIdx = lineIdx;
        newColIdx  = pos;
    } else if (lineIdx > 0) {
        // Satır başında: önceki satıra birleştir
        LineNode *prev = node->prev;
        newLineIdx = lineIdx - 1;
        newColIdx  = (int)prev->text.size();
        mergeWithNext(prev);
    } else {
        newLineIdx = 0;
        newColIdx  = 0;
    }
}

void Buffer::deleteCharAt(int lineIdx, int colIdx) {
    LineNode *node = lineAt(lineIdx);
    if (!node) return;
    if (colIdx < (int)node->text.size()) {
        // UTF-8 çok-bayt karakter
        int start = colIdx;
        int end   = colIdx + 1;
        while (end < (int)node->text.size() &&
               (unsigned char)node->text[end] >= 0x80 &&
               (unsigned char)node->text[end] <= 0xBF) {
            end++;
        }
        node->text.erase(start, end - start);
    } else if (node->next) {
        mergeWithNext(node);
    }
}

void Buffer::splitLine(int lineIdx, int colIdx) {
    LineNode *node = lineAt(lineIdx);
    if (!node) return;
    if (colIdx < 0) colIdx = 0;
    if (colIdx > (int)node->text.size()) colIdx = (int)node->text.size();

    std::string newLineText = node->text.substr(colIdx);
    node->text = node->text.substr(0, colIdx);
    insertLineAfter(node, newLineText);
}

void Buffer::deleteWordBefore(int lineIdx, int colIdx, int &newLineIdx, int &newColIdx) {
    LineNode *node = lineAt(lineIdx);
    if (!node) { newLineIdx = lineIdx; newColIdx = colIdx; return; }

    if (colIdx == 0) {
        // Satır başı: öncekiyle birleştir
        deleteCharBefore(lineIdx, 0, newLineIdx, newColIdx);
        return;
    }

    const std::string &t = node->text;
    int pos = colIdx - 1;
    // Boşlukları atla
    while (pos > 0 && t[pos] == ' ') pos--;
    // Kelime karakterlerini atla
    while (pos > 0 && t[pos - 1] != ' ') pos--;

    node->text.erase(pos, colIdx - pos);
    newLineIdx = lineIdx;
    newColIdx  = pos;
}

void Buffer::deleteRange(int startLine, int startCol, int endLine, int endCol,
                         int &newLineIdx, int &newColIdx) {
    if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
        std::swap(startLine, endLine);
        std::swap(startCol,  endCol);
    }

    LineNode *startNode = lineAt(startLine);
    if (!startNode) { newLineIdx = startLine; newColIdx = startCol; return; }

    if (startLine == endLine) {
        startNode->text.erase(startCol, endCol - startCol);
    } else {
        // Başlangıç satırı: startCol'dan sona kadar sil
        startNode->text = startNode->text.substr(0, startCol);
        // Son satır: baştan endCol'a kadar sil
        LineNode *endNode = lineAt(endLine);
        if (endNode) {
            startNode->text += endNode->text.substr(endCol);
        }
        // Aradaki satırları sil (endLine dahil)
        LineNode *cur = startNode->next;
        for (int i = startLine + 1; i <= endLine && cur; i++) {
            LineNode *nxt = cur->next;
            removeLine(cur);
            cur = nxt;
        }
    }
    newLineIdx = startLine;
    newColIdx  = startCol;
}

std::string Buffer::getRange(int startLine, int startCol, int endLine, int endCol) const {
    if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
        std::swap(startLine, endLine);
        std::swap(startCol,  endCol);
    }
    std::string result;
    for (int i = startLine; i <= endLine; i++) {
        LineNode *node = lineAt(i);
        if (!node) break;
        int from = (i == startLine) ? startCol : 0;
        int to   = (i == endLine)   ? endCol   : (int)node->text.size();
        from = std::max(0, std::min(from, (int)node->text.size()));
        to   = std::max(0, std::min(to,   (int)node->text.size()));
        result += node->text.substr(from, to - from);
        if (i < endLine) result += '\n';
    }
    return result;
}

std::string Buffer::getAllText() const {
    std::string result;
    LineNode *cur = head_;
    while (cur) {
        result += cur->text;
        if (cur->next) result += '\n';
        cur = cur->next;
    }
    return result;
}

// ── KMP Arama ────────────────────────────────────────────────────────────
std::vector<int> Buffer::buildKMPTable(const std::string &pattern) {
    int m = (int)pattern.size();
    std::vector<int> table(m, 0);
    int k = 0;
    for (int i = 1; i < m; i++) {
        while (k > 0 && pattern[k] != pattern[i]) k = table[k - 1];
        if (pattern[k] == pattern[i]) k++;
        table[i] = k;
    }
    return table;
}

std::vector<Buffer::Match> Buffer::findAll(const std::string &pattern, bool caseSensitive) const {
    std::vector<Match> results;
    if (pattern.empty()) return results;

    std::string pat = pattern;
    if (!caseSensitive) {
        std::transform(pat.begin(), pat.end(), pat.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }
    auto table = buildKMPTable(pat);
    int m = (int)pat.size();

    int lineIdx = 0;
    LineNode *cur = head_;
    while (cur) {
        std::string line = cur->text;
        if (!caseSensitive) {
            std::transform(line.begin(), line.end(), line.begin(),
                           [](unsigned char c){ return std::tolower(c); });
        }
        int k = 0;
        for (int i = 0; i < (int)line.size(); i++) {
            while (k > 0 && pat[k] != line[i]) k = table[k - 1];
            if (pat[k] == line[i]) k++;
            if (k == m) {
                results.push_back({lineIdx, i - m + 1, m});
                k = table[k - 1];
            }
        }
        cur = cur->next;
        lineIdx++;
    }
    return results;
}

int Buffer::replaceAll(const std::string &pattern, const std::string &replacement,
                       bool caseSensitive) {
    auto matches = findAll(pattern, caseSensitive);
    if (matches.empty()) return 0;

    // Sondan başa doğru değiştir (indeks kayması olmasın)
    for (int i = (int)matches.size() - 1; i >= 0; i--) {
        auto &m = matches[i];
        LineNode *node = lineAt(m.line);
        if (node) {
            node->text.replace(m.col, m.len, replacement);
        }
    }
    return (int)matches.size();
}
