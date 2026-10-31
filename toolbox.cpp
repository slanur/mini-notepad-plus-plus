// toolbox.cpp — Toolbox şerit çizimi

#include "toolbox.h"
#include "terminal.h"

Toolbox::Toolbox() {
    // Dar pencerede sigmayanlar sondan dusurulur; en onemliler basta
    hints_ = {
        {"Yardim", "Ctrl+K"},
        {"Kaydet", "Ctrl+S"},
        {"Sablon", "Ctrl+T"},
        {"Ozet",   "Ctrl+L"},
        {"Zaman",  "Ctrl+D"},
        {"Cikis",  "ESC"},
    };

    help_ = {
        {"Yardim ekrani",                 "Ctrl+K veya F1"},
        {"Not sablonu ekle",              "Ctrl+T"},
        {"Zaman damgasi ekle",            "Ctrl+D"},
        {"Not ozeti",                     "Ctrl+L"},
        {"Dosya ac",                      "Ctrl+O"},
        {"Kaydet",                        "Ctrl+S"},
        {"Farkli kaydet",                 "Ctrl+W"},
        {"Ara",                           "Ctrl+F"},
        {"Eslesmeler arasinda gezin",     "Ctrl+G"},
        {"Tumunu degistir",               "Ctrl+R"},   // Windows'ta Ctrl+H Backspace ile catisiyor
        {"Geri al",                       "Ctrl+Z"},
        {"Yinele",                        "Ctrl+Y"},
        {"Karakter sec",                  "Shift+Ok"},
        {"Kelime sec",                    "Ctrl+Shift+Ok"},
        {"Kopyala (secim yoksa satir)",   "Ctrl+C"},
        {"Kes",                           "Ctrl+X"},
        {"Yapistir",                      "Ctrl+V"},
        {"Onceki kelimeyi sil",           "Ctrl+Backspace"},
        {"Secimi temizle / cikis",        "ESC"},
    };
}

int Toolbox::render(int termCols) const {
    std::string line = " Mini Notepad++";
    for (const ToolboxItem &item : hints_) {
        std::string token = "  " + item.shortcut + " " + item.label;
        if ((int)(line.size() + token.size()) > termCols) break;
        line += token;
    }
    line.resize(termCols, ' ');

    Terminal::moveCursor(1, 1);
    Terminal::setColor(COLOR_WHITE, COLOR_BLUE);
    Terminal::print(line);
    Terminal::resetColor();
    return height();
}

std::vector<std::string> Toolbox::helpRows() const {
    std::vector<std::string> rows;
    for (const ToolboxItem &item : help_) {
        std::string row = item.shortcut;
        row.resize(18, ' ');
        rows.push_back(row + item.label);
    }
    return rows;
}
