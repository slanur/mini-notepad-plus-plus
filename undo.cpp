// undo.cpp — Undo stack implementasyonu

#include "undo.h"

void UndoStack::push(const std::string &text, int cursorLine, int cursorCol) {
    // Sınırı aşarsa en alttan sil (stack kopyalanarak yeniden oluştur)
    if ((int)stack_.size() >= MAX_UNDO) {
        // std::stack alt elemana erişim sağlamaz; geçici bir yapıya aktar
        std::stack<UndoState> tmp;
        while (!stack_.empty()) {
            tmp.push(stack_.top());
            stack_.pop();
        }
        tmp.pop(); // En eski eleman
        while (!tmp.empty()) {
            stack_.push(tmp.top());
            tmp.pop();
        }
    }
    stack_.push({text, cursorLine, cursorCol});
}

UndoState UndoStack::pop() {
    UndoState state = stack_.top();
    stack_.pop();
    return state;
}

void UndoStack::clear() {
    while (!stack_.empty()) stack_.pop();
}
