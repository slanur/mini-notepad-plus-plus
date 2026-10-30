# Mini Notepad++

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS-555)
![Bağımlılık](https://img.shields.io/badge/ba%C4%9F%C4%B1ml%C4%B1l%C4%B1k-yok-2ea44f)

Terminalde çalışan, C++ ile sıfırdan yazılmış küçük bir metin editörü. Grafik arayüz ya da `ncurses` gibi harici bir kütüphane kullanmaz: ekranı ANSI kaçış kodlarıyla çizer, klavyeyi doğrudan işletim sistemi API'lerinden okur.

Kocaeli Üniversitesi **Programlama Laboratuvarı II** dersi kapsamında geliştirilmiştir.

```text
 Mini Notepad++ -- Konsol Metin Editoru
Dosya Ac (Ctrl+O)  Kaydet (Ctrl+S)  Farkli Kaydet (Ctrl+W)  Ara (Ctrl+F)
Git (Ctrl+G)  Degistir (Ctrl+R)  Geri Al (Ctrl+Z)  Sec(Kar) (Ctrl+->)
Sec(Kel) (Ctrl+Shift+->)  Kopyala (Ctrl+C)  Kes (Ctrl+X)  Yapistir (Ctrl+V)
Kelime Sil (Ctrl+BS)  Cikis (ESC)
--------------------------------------------------------------------------------
    1 | ali veli ali
    2 | ikinci satir
      |
      |
    seed.txt Satir:1  Sutun:1  Eslesmeler:0
```

## Özellikler

- **Dosya işlemleri:** Dahili dosya yöneticisiyle aç, kaydet, farklı kaydet
- **Arama:** Büyük/küçük harf duyarsız arama, eşleşmeleri vurgulama ve aralarında gezinme
- **Değiştirme:** Bir kelimenin tüm geçtiği yerleri tek seferde değiştirme
- **Geri alma:** 100 adıma kadar geri alma
- **Seçim ve pano:** Karakter veya kelime bazında seçim; kopyala, kes, yapıştır
- **Kaydetmeden çıkış uyarısı:** Değişiklikler kaybolmadan önce sorar
- **Arayüz:** Satır numaraları, durum çubuğu, pencere genişliğine uyan araç çubuğu
- **Türkçe karakter:** UTF-8 metin yazma ve kaydetme

## Derleme ve çalıştırma

C++17 destekleyen bir derleyici yeterlidir.

**macOS / Linux**

```bash
mkdir -p build
```

```bash
g++ -std=c++17 -O2 *.cpp -o build/mini-notepad
```

```bash
./build/mini-notepad
```

**Windows (MinGW)**

Windows'a özgü kodun derlenmesi için `PLATFORM_WINDOWS` makrosu verilmelidir:

```powershell
g++ -std=c++17 -O2 -DPLATFORM_WINDOWS main.cpp buffer.cpp editor.cpp filemanager.cpp terminal.cpp toolbox.cpp undo.cpp -o main.exe
```

```powershell
.\main.exe
```

## Klavye kısayolları

| Kısayol | İşlev |
| --- | --- |
| `Ctrl+O` | Dosya aç |
| `Ctrl+S` | Kaydet |
| `Ctrl+W` | Farklı kaydet |
| `Ctrl+F` | Ara |
| `Ctrl+G` | Eşleşmeler arasında gezin (`←` `↑` önceki, `→` `↓` sonraki) |
| `Ctrl+R` | Değiştir (önce eski, sonra yeni kelime sorulur) |
| `Ctrl+Z` | Geri al |
| `Shift+←` / `Shift+→` | Karakter bazında seç |
| `Ctrl+Shift+←` / `Ctrl+Shift+→` | Kelime bazında seç |
| `Ctrl+C` / `Ctrl+X` / `Ctrl+V` | Kopyala / kes / yapıştır |
| `Ctrl+Backspace` | Önceki kelimeyi sil |
| `Home` / `End` | Satır başına / sonuna git |
| `Page Up` / `Page Down` | Yarım ekran yukarı / aşağı git |
| `ESC` | Çıkış (kaydedilmemiş değişiklik varsa sorar) |

## Proje yapısı

| Dosya | Sorumluluk |
| --- | --- |
| `main.cpp` | Giriş noktası ve hata yakalama |
| `editor.h/.cpp` | Ana döngü, modlar, ekran çizimi ve komutlar |
| `buffer.h/.cpp` | Metin tamponu, arama ve değiştirme |
| `undo.h/.cpp` | Geri alma yığını |
| `filemanager.h/.cpp` | Konsol tabanlı dosya yöneticisi |
| `terminal.h/.cpp` | Platformdan bağımsız terminal katmanı |
| `toolbox.h/.cpp` | Üst araç çubuğu |

## Teknik tasarım

- **Metin tamponu:** Her satır, çift yönlü bağlı listede bir düğümdür (`LineNode`). Satır ekleme, silme ve birleştirme işlemleri düğüm bağlantıları üzerinden yapılır; satır içeriği `std::string` olarak tutulur.
- **Arama:** Knuth-Morris-Pratt (KMP) algoritması kullanılır, bu nedenle arama süresi satır uzunluğu ve kalıp uzunluğuyla doğrusal artar.
- **Geri alma:** `std::stack` üzerinde anlık görüntü (snapshot) yaklaşımı kullanılır. Her değişiklikten önce metnin tamamı ve imleç konumu saklanır; en fazla 100 durum tutulur.
- **Platform soyutlaması:** Windows Console API ve POSIX `termios` farkları `terminal.cpp` içinde gizlenir. Editörün geri kalanı işletim sisteminden bağımsızdır.

## Bilinen sınırlamalar

**Hatalar**

- **Yeni isimle kaydetme:** Kaydetme ekranında `..` seçiliyken isim yazıp Enter'a basılırsa dosya kaydedilmez, üst klasöre geçilir. Önce `↓` ile listede bir dosyanın üzerine gelinmelidir.
- **macOS / Linux'ta dosya yöneticisi:** Ekran satır tamponlu modda çalışır; tuşlar anında işlenmez, Enter beklenir.
- **Seçimi ESC ile kaldırma:** Metin seçiliyken ESC seçimi kaldırmakla kalmaz, çıkış işlemini de başlatır.
- **Türkçe karakter üzerinde gezinme:** `←` ve `→` bayt bayt ilerler. İmleç çok baytlı bir karakterin ortasındayken yazı yazılırsa dosyadaki UTF-8 metin bozulur.

**Eksikler**

- Komut satırından dosya açılamaz; dosyalar `Ctrl+O` ile açılır.
- `Tab` tuşu metne bir şey eklemez.
- Uzun satırlar kaydırılmaz, pencere genişliğinde kesilir.
- Yineleme (redo) yoktur.
- Pano editörün içindedir, sistem panosuyla bağlantılı değildir.
- Araç çubuğunda `Sec(Kar)` için `Ctrl+->` yazar, ama karakter seçimi `Shift+→` ile yapılır.
