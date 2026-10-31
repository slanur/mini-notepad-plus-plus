# Mini Notepad++

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows-555)
![Bağımlılık](https://img.shields.io/badge/ba%C4%9F%C4%B1ml%C4%B1l%C4%B1k-yok-2ea44f)

Sızma testi (pentest) sırasında not almak için tasarlanmış, terminalde çalışan küçük bir metin editörü. Test yaptığın terminalin yanında açık kalır: araç çıktısını yapıştırırsın, IP ve CVE'leri otomatik renklendirir, bulgularını şablonla yazar ve tek tuşla nottaki tüm hedefleri listelersin.

C++ ile sıfırdan yazılmıştır. Grafik arayüz ya da `ncurses` gibi harici bir kütüphane kullanmaz. Notlar düz Markdown olarak kaydedilir, yani rapor yazarken doğrudan kullanılabilir.

Kocaeli Üniversitesi **Programlama Laboratuvarı II** dersi için yazılan konsol metin editöründen geliştirilmiştir.

```text
 Mini Notepad++  Ctrl+K Yardim  Ctrl+S Kaydet  Ctrl+T Sablon  Ctrl+L Ozet
    1 | ## Keşif: 10.10.11.5
    2 |
    3 | [2026-10-30 21:40:12] servis taraması
    4 | $ nmap -sV 10.10.11.5
    5 | 443/tcp open https
    6 |
    7 | ## Bulgu: Log4Shell
    8 | - **Ciddiyet:** [KRITIK]
    9 | - **CVE:** CVE-2021-44228
   10 | - **Etkilenen varlık:** https://portal.acme.test/login
    hedef.md Satir:1  Sutun:1  Eslesmeler:0
```

Tüm kısayolları görmek için `Ctrl+K` veya `F1` yeterlidir.

## Hızlı başlangıç

```bash
git clone https://github.com/slanur/mini-notepad-plus-plus.git
```

```bash
cd mini-notepad-plus-plus && make
```

```bash
./build/mini-notepad hedef.md
```

Sonra `Ctrl+T` ile "Test bilgileri" şablonunu ekle, araç çıktılarını yapıştır, notların üzerinde `Ctrl+L` ile özeti aç. Kurulum seçenekleri için [Derleme ve çalıştırma](#derleme-ve-çalıştırma) bölümüne bakabilirsin.

## Pentest özellikleri

### Otomatik yedek

Kaydedilmemiş değişiklikler birkaç saniyede bir gizli bir yedeğe yazılır; `hedef.md` notunun yedeği aynı klasörde `.hedef.md.autosave` olur. Terminal kapanırsa veya bağlantı koparsa, not tekrar açıldığında yedeği geri yüklemek isteyip istemediğin sorulur. Not kaydedilince yedek silinir.

### Sistem panosu

`Ctrl+C` seçili metni sistem panosuna kopyalar. Seçim yoksa bulunulan satırı kopyalar ve baştaki `$ ` önekini atar, böylece nottaki bir komut doğrudan diğer terminale yapıştırılabilir. `Ctrl+V` sistem panosundan yapıştırır.

Bu özellik macOS'ta hazır çalışır, Linux'ta `wl-clipboard` veya `xclip` gerekir. Pano aracı yoksa editör kendi iç panosunu kullanır.

### Otomatik renklendirme

Notu yazarken aşağıdaki değerler kendiliğinden renklenir.

| Değer | Örnek |
| --- | --- |
| Başlık | `## Keşif: web01` |
| Komut satırı | `$ nmap -sV 10.10.11.5` |
| IPv4, CIDR, IP:port | `10.10.11.5`, `192.168.1.0/24`, `10.0.0.1:8080` |
| Port | `443/tcp`, `53/udp` |
| Adres | `https://`, `http://`, `ftp://`, `ssh://`, `smb://`, `ldap://` |
| CVE | `CVE-2021-44228` |
| Hash | 32, 40 ve 64 karakterlik hex (MD5, SHA-1, SHA-256 uzunlukları) |
| Ciddiyet etiketi | `[KRITIK]`, `[YUKSEK]`, `[ORTA]`, `[DUSUK]`, `[BILGI]` |

Ciddiyet etiketleri İngilizce de yazılabilir: `[CRITICAL]`, `[HIGH]`, `[MEDIUM]`, `[LOW]`, `[INFO]`.

### Not özeti (`Ctrl+L`)

Nottaki bulguları ve benzersiz IP, adres, CVE, port ve hash değerlerini tek ekranda listeler. Bulgular ciddiyete göre sıralanır, tekrar eden değerler sayılır. Bir kaydın üzerinde Enter'a basınca o satıra gidilir.

Başlığında `Bulgu` geçen bölümler bulgu sayılır. Bölümdeki ilk ciddiyet etiketi o bulgunun ciddiyeti olur.

### Şablonlar (`Ctrl+T`)

| Şablon | İçerik |
| --- | --- |
| Test bilgileri | Müşteri, yetki belgesi, kapsam, test penceresi, acil iletişim |
| Keşif (host) | Host bilgileri, açık port tablosu, komutlar |
| Web uygulaması | Test kontrol listesi ve notlar |
| Bulgu | Ciddiyet, CVSS, CVE, etkilenen varlık, tekrar üretme adımları, kanıt, etki, çözüm önerisi |
| Komut günlüğü | Zaman damgalı komut kaydı |

### Araç çıktısı yapıştırma

Terminalin yapıştırma özelliği desteklenir: uzun bir `nmap` çıktısı tek seferde eklenir ve tek `Ctrl+Z` ile geri alınır. Sekmeler boşluğa, Windows satır sonları `\n` karakterine çevrilir.

### Zaman damgası (`Ctrl+D`)

İmlecin olduğu yere `[2026-10-30 21:40:12]` biçiminde yerel saat ekler. Testin zaman çizelgesini tutmak için kullanılabilir.

### Kaçış kodu koruması

Hedeften alınan çıktılarda veya dosyalarda bulunan kontrol karakterleri terminale ham olarak gönderilmez, ekranda `.` olarak gösterilir. Böylece hazırlanmış bir dosya, açıldığında terminalini yönetemez.

## Derleme ve çalıştırma

C++17 destekleyen bir derleyici yeterlidir.

**macOS / Linux**

```bash
make install
```

```bash
mini-notepad hedef.md
```

Program `~/.local/bin` altına kurulur, `sudo` gerekmez. Kaldırmak için `make uninstall` yeterlidir.

`command not found: mini-notepad` hatası alırsan bu klasör `PATH` içinde değildir. zsh kullanıyorsan şu komutla ekleyip terminali yeniden açabilirsin:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
```

İstersen kurmadan da kullanabilirsin: `make` ile derleyip `./build/mini-notepad hedef.md` komutuyla çalıştır.

Verilen dosya yoksa bu adla yeni bir not başlar. Dosya adı vermeden de açılabilir.

**Windows (MinGW)**

Windows'a özgü kodun derlenmesi için `PLATFORM_WINDOWS` makrosu verilmelidir:

```powershell
g++ -std=c++17 -O2 -DPLATFORM_WINDOWS main.cpp autosave.cpp buffer.cpp editor.cpp filemanager.cpp pentest.cpp terminal.cpp toolbox.cpp undo.cpp -o main.exe
```

```powershell
.\main.exe hedef.md
```

## Klavye kısayolları

| Kısayol | İşlev |
| --- | --- |
| `Ctrl+K` veya `F1` | Yardım ekranı |
| `Ctrl+T` | Not şablonu ekle |
| `Ctrl+D` | Zaman damgası ekle |
| `Ctrl+L` | Not özeti |
| `Ctrl+O` | Dosya aç |
| `Ctrl+S` | Kaydet |
| `Ctrl+W` | Farklı kaydet |
| `Ctrl+F` | Ara |
| `Ctrl+G` | Eşleşmeler arasında gezin (`←` `↑` önceki, `→` `↓` sonraki) |
| `Ctrl+R` | Tümünü değiştir (önce eski, sonra yeni kelime sorulur) |
| `Ctrl+Z` | Geri al |
| `Ctrl+Y` | Yinele |
| `Shift+←` / `Shift+→` | Karakter bazında seç |
| `Ctrl+Shift+←` / `Ctrl+Shift+→` | Kelime bazında seç |
| `Ctrl+C` | Kopyala (seçim yoksa bulunulan satırı) |
| `Ctrl+X` / `Ctrl+V` | Kes / yapıştır |
| `Ctrl+Backspace` | Önceki kelimeyi sil |
| `Tab` | 4 boşluk ekle |
| `Home` / `End` | Satır başına / sonuna git |
| `Page Up` / `Page Down` | Yarım ekran yukarı / aşağı git |
| `ESC` | Seçimi ve vurguları temizle; yoksa çık (kaydedilmemiş değişiklik varsa sorar) |

## Proje yapısı

| Dosya | Sorumluluk |
| --- | --- |
| `main.cpp` | Giriş noktası ve hata yakalama |
| `editor.h/.cpp` | Ana döngü, modlar, ekran çizimi ve komutlar |
| `pentest.h/.cpp` | Satır tarama, şablonlar, not özeti |
| `autosave.h/.cpp` | Otomatik yedek dosyaları |
| `buffer.h/.cpp` | Metin tamponu, arama ve değiştirme |
| `undo.h/.cpp` | Geri alma ve yineleme geçmişi |
| `filemanager.h/.cpp` | Konsol tabanlı dosya yöneticisi |
| `terminal.h/.cpp` | Platformdan bağımsız terminal katmanı |
| `toolbox.h/.cpp` | Üst araç çubuğu |

## Teknik tasarım

- **Metin tamponu:** Her satır, çift yönlü bağlı listede bir düğümdür (`LineNode`). Satır ekleme, silme ve birleştirme işlemleri düğüm bağlantıları üzerinden yapılır; satır içeriği `std::string` olarak tutulur.
- **Arama:** Knuth-Morris-Pratt (KMP) algoritması kullanılır, bu nedenle arama süresi satır uzunluğu ve kalıp uzunluğuyla doğrusal artar.
- **Belirteç tarama:** IP, CVE, hash gibi değerler düzenli ifade kütüphanesi olmadan elle yazılmış eşleştiricilerle bulunur. Aynı tarayıcı hem renklendirmeyi hem not özetini besler.
- **Geri alma ve yineleme:** Anlık görüntü (snapshot) yaklaşımı kullanılır. Her değişiklikten önce metnin tamamı ve imleç konumu saklanır; en fazla 100 durum tutulur.
- **Otomatik yedek:** Ana döngü tuş beklerken zaman aşımıyla uyanır, böylece yazmayı bıraktığında da yedek alınır. Yedek önce geçici dosyaya yazılır ve yeniden adlandırılır, yani yazma sırasında kesilse bile eski yedek bozulmaz.
- **Platform soyutlaması:** Windows Console API ve POSIX `termios` farkları `terminal.cpp` içinde gizlenir. Editörün geri kalanı işletim sisteminden bağımsızdır.

## Güvenlik notları

- **Notlar şifrelenmez.** Müşteri verisi, parola ve hash içeren notlar düz metin olarak diske yazılır. Bu notları şifreli bir diskte tut, bulut senkronizasyonuna ve Git'e ekleme.
- **Yedekler de not içeriği taşır.** `.autosave` dosyaları yalnızca sahibi tarafından okunabilir, ama şifrelenmez ve notla aynı klasörde durur.
- **Sistem panosuna kopyalanan veri** pano geçmişi tutan uygulamalarda kalabilir. Parola ve benzeri değerleri kopyalarken buna dikkat et.
- Bu editör bir saldırı aracı değil, not defteridir. Sızma testlerini yalnızca yazılı yetkin olan sistemlerde yap.

## Bilinen sınırlamalar

- Bu sürüm yalnızca macOS'ta test edildi. Windows ve Linux için derleme ve çalışma denenmedi.
- Windows'ta sistem panosu ve toplu yapıştırma iyileştirmesi yoktur; editör kendi iç panosunu kullanır.
- Aynı not iki pencerede açılırsa ikisi de aynı yedeğe yazar.
- Arama, büyük/küçük harf ayrımını yalnızca ASCII harflerde yok sayar. Türkçe karakterler bire bir eşleşmelidir.
- Emoji gibi 4 baytlık karakterler bulunan satırlarda imleç konumu kayabilir.
- `Ctrl+←` ve `Ctrl+→` kelime atlamaz, tek karakter ilerler.
- Bir IPv4 adresine benzeyen sürüm numaraları (`1.2.3.4`) IP olarak işaretlenir.
