#pragma once
// autosave.h — Otomatik yedek dosyaları
// Kaydedilmemiş notlar terminal kapansa bile kaybolmasın diye gizli bir yedeğe yazılır.
// hedef.md notunun yedeği aynı klasörde .hedef.md.autosave olur.

#include <string>

namespace Autosave {

/// Notun yedek yolu; adı olmayan not için ev dizinindeki ortak yedek kullanılır
std::string pathFor(const std::string &filePath);

/// Yedeği yaz. Yarım dosya kalmaması için önce geçici dosyaya yazılır.
/// Notlar hassas olabileceğinden dosya yalnızca sahibi tarafından okunabilir.
bool write(const std::string &path, const std::string &text);

/// Yedeği oku; yoksa false döner
bool read(const std::string &path, std::string &text);

/// Yedeği sil
void remove(const std::string &path);

} // namespace Autosave
