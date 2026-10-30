#pragma once
// undo.h — Stack tabanlı geri alma (Undo) mekanizması
// Her düzenleme öncesi durum snapshot olarak tutulur.

#include <string>
#include <stack>

// Basit snapshot tabanlı undo: her değişiklik öncesi tamponun tüm metni
// ve imleç pozisyonu kaydedilir. Büyük dosyalarda bellek maliyeti artar,
// ancak proje için yeterlidir.

struct UndoState {
    std::string text;   ///< Değişiklik öncesi tüm metin (\n ile satır ayrılmış)
    int cursorLine;
    int cursorCol;
};

class UndoStack {
public:
    static const int MAX_UNDO = 100; ///< Maksimum geri alma adımı

    /// Mevcut durumu kaydet
    void push(const std::string &text, int cursorLine, int cursorCol);

    /// Geri alma durumu var mı?
    bool canUndo() const { return !stack_.empty(); }

    /// Son kaydedilen durumu çıkar
    UndoState pop();

    /// Tüm geçmişi temizle
    void clear();

    int size() const { return (int)stack_.size(); }

private:
    std::stack<UndoState> stack_;
};
