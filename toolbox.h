#pragma once
// toolbox.h — Üst şerit (Toolbox) çizimi
// Not alanına yer kalsın diye tek satırdır; tüm kısayollar yardım ekranındadır.

#include <string>
#include <vector>

struct ToolboxItem {
    std::string label;    ///< Görünen ad
    std::string shortcut; ///< Kısayol
};

class Toolbox {
public:
    Toolbox();

    /// Toolbox'ı ekrana çiz; çizilen satır sayısını döndür
    int render(int termCols) const;

    /// Kaç satır kapladığını döndür (editor'ün metin alanı başlangıcını bulmak için)
    int height() const { return 1; }

    /// Yardım ekranının satırları
    std::vector<std::string> helpRows() const;

private:
    std::vector<ToolboxItem> hints_; ///< Üst şeritte gösterilen en sık kısayollar
    std::vector<ToolboxItem> help_;  ///< Yardım ekranındaki tüm kısayollar
};
