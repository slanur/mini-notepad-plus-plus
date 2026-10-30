// main.cpp — Mini Notepad++ Giriş Noktası
// Kocaeli Üniversitesi — Programlama Laboratuvarı II — Proje 1

#include "editor.h"
#include "terminal.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    try {
        Editor editor;

        // Komut satırından dosya yolu verilmişse aç
        if (argc > 1) {
            // Editor sınıfı içinde openFile() public API'ye ihtiyaç var.
            // Şimdilik run() öncesi manuel yükleme yapılabilir.
            // (Genişletme: Editor::loadFile(path) eklenebilir)
        }

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
