// main.cpp — Mini Notepad++ Giriş Noktası
// Kocaeli Üniversitesi — Programlama Laboratuvarı II — Proje 1

#include "editor.h"
#include "terminal.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
    try {
        Editor editor;

        // Komut satırından dosya yolu verilmişse aç; yoksa bu adla yeni dosya başlar
        if (argc > 1) editor.loadFile(argv[1], true);

        editor.run();
    } catch (const std::exception &e) {
        Terminal::disableRawMode();
        std::cerr << "\nHata: " << e.what() << "\n";
        return 1;
    } catch (...) {
        Terminal::disableRawMode();
        std::cerr << "\nBilinmeyen hata oluştu.\n";
        return 1;
    }

    return 0;
}
