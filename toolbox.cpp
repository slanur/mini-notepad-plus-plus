// toolbox.cpp — Toolbox şerit çizimi

#include "toolbox.h"
#include "terminal.h"
#include <sstream>

Toolbox::Toolbox() {
    items_ = {
        {"Dosya Ac",          "Ctrl+O"},
        {"Kaydet",            "Ctrl+S"},
        {"Farkli Kaydet",     "Ctrl+W"},
        {"Ara",               "Ctrl+F"},
        {"Git",               "Ctrl+G"},
        {"Degistir",          "Ctrl+R"},   // Windows'ta Ctrl+H Backspace ile catisiyor
        {"Geri Al",           "Ctrl+Z"},
        {"Sec(Kar)",          "Ctrl+->"},
        {"Sec(Kel)",          "Ctrl+Shift+->"},
        {"Kopyala",           "Ctrl+C"},
        {"Kes",               "Ctrl+X"},
        {"Yapistir",          "Ctrl+V"},
        {"Kelime Sil",        "Ctrl+BS"},
        {"Cikis",             "ESC"},
    };
}

int Toolbox::renderLine(int row, int termCols,
                        const std::vector<ToolboxItem> &items, int startIdx) const {
    Terminal::moveCursor(row, 1);
    Terminal::setColor(COLOR_BLACK, COLOR_CYAN);

    std::string line(termCols, ' ');
    int pos = 0;

    int i = startIdx;
    for (; i < (int)items.size(); i++) {
        std::string token = items[i].label + " (" + items[i].shortcut + ")";
        if (i > startIdx) {
            token = "  " + token;
        }
        if (pos + (int)token.size() > termCols) break;
        for (int j = 0; j < (int)token.size() && pos < termCols; j++, pos++) {
            line[pos] = token[j];
        }
    }

    Terminal::print(line.substr(0, termCols));
    Terminal::resetColor();
    return i;
}

int Toolbox::render(int termCols) const {
    int row = 1;

    // Baslik satiri
    Terminal::moveCursor(row, 1);
    Terminal::setColor(COLOR_WHITE, COLOR_BLUE);
    std::string header = " Mini Notepad++ -- Konsol Metin Editoru ";
    while ((int)header.size() < termCols) header += ' ';
    Terminal::print(header.substr(0, termCols));
    Terminal::resetColor();
    row++;

    // Toolbox ogelerini satirlara bol
    int idx = 0;
    while (idx < (int)items_.size()) {
        int next = renderLine(row, termCols, items_, idx);
        row++;
        if (next == idx) break;
        idx = next;
    }

    // Ayirici
    Terminal::moveCursor(row, 1);
    Terminal::setColor(COLOR_BRIGHT_BLACK, COLOR_DEFAULT);
    Terminal::print(std::string(termCols, '-'));
    Terminal::resetColor();
    row++;

    return row - 1;
}

int Toolbox::height(int termCols) const {
    int rows = 2; // Baslik + ayirici
    int idx  = 0;
    while (idx < (int)items_.size()) {
        int pos = 0;
        int start = idx;
        for (; idx < (int)items_.size(); idx++) {
            std::string token = items_[idx].label + " (" + items_[idx].shortcut + ")";
            if (idx > start) token = "  " + token;
            if (pos + (int)token.size() > termCols) break;
            pos += (int)token.size();
        }
        rows++;
        if (idx == start) break;
    }
    return rows;
}