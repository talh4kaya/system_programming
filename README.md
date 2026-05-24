# tarsau — Arşiv Oluşturma Programı

Sistem Programlama 2025-2026 Dönemi Proje Ödevi

Mehmet Zahid GÖRGEÇ - Talha KAYA
G221210060 - G221210063

## Derleme

```bash
make
```

## Kullanım

### Arşiv Oluşturma (`-b`)
```bash
./tarsau -b dosya1.txt dosya2.txt dosya3.txt -o arsiv.sau
./tarsau -b dosya1.txt dosya2.txt          # varsayılan: a.sau
```

### Arşiv Açma (`-a`)
```bash
./tarsau -a arsiv.sau hedef_dizin
./tarsau -a arsiv.sau                      # mevcut dizine açar
```

## .sau Arşiv Formatı

```
[10 byte: index uzunluğu]|dosyaadi,izin,boyut||dosyaadi2,izin2,boyut2|<içerik1><içerik2>
```

## Kısıtlar
- Maksimum 32 dosya
- Toplam boyut maksimum 200 MB
- Sadece text (ASCII) dosyalar

## Test

```bash
chmod +x test.sh
./test.sh
```

## Temizleme

```bash
make clean
```
