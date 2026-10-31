// undo.cpp — Undo stack implementasyonu

#include "undo.h"

void UndoStack::push(const std::string &text, int cursorLine, int cursorCol) {
    if ((int)states_.size() >= MAX_UNDO) states_.pop_front();
    states_.push_back({text, cursorLine, cursorCol});
}

UndoState UndoStack::pop() {
    UndoState state = states_.back();
    states_.pop_back();
    return state;
}

void UndoStack::clear() {
    states_.clear();
}
