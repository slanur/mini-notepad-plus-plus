#pragma once
// toolbox.h — Üst şerit (Toolbox) çizimi
// Komutlar tek satırda yan yana gösterilir; sığmazsa ikinci satıra geçer.

#include <string>
#include <vector>

struct ToolboxItem {
    std::string label;    ///< Görünen ad
    std::string shortcut; ///< Kısayol (parantez içinde gösterilir)
};

class Toolbox {
public:
    Toolbox();

    /// Toolbox'ı ekrana çiz; çizilen satır sayısını döndür
    int render(int termCols) const;

    /// Kaç satır kapladığını hesapla (editor'ün metin alanı başlangıcını bulmak için)
    int height(int termCols) const;

private:
    std::vector<ToolboxItem> items_;

    // Tek satır çiz; kalan items index ve items listesini döndürür
    int renderLine(int row, int termCols,
                   const std::vector<ToolboxItem> &items, int startIdx) const;
};
