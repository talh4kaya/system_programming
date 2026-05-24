#!/bin/bash
# tarsau test scripti

PASS=0
FAIL=0

ok()  { echo "[PASS] $1"; PASS=$((PASS+1)); }
err() { echo "[FAIL] $1"; FAIL=$((FAIL+1)); }

# Temizlik
cleanup() {
    rm -f t1.txt t2.txt t3.txt binary.bin a.sau test.sau
    rm -rf cikti test_dir
}

cleanup

echo "=== tarsau Test Scripti ==="
echo ""

# Derleme
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "DERLEME HATASI! Testler calismayacak."
    exit 1
fi
echo "Derleme basarili."
echo ""

# Test dosyaları oluştur
echo "Merhaba Dunya" > t1.txt
echo "Sistem Programlama 2026" > t2.txt
printf "Bu ucuncu dosyadir.\nSatir 2.\nSatir 3.\n" > t3.txt
printf '\x00\x01\x02binary' > binary.bin

# --- TEST 1: Basit arşiv oluşturma ---
echo "--- Test 1: Basit arsiv olusturma (varsayilan a.sau) ---"
./tarsau -b t1.txt t2.txt t3.txt
if [ -f a.sau ]; then ok "a.sau olusturuldu"
else err "a.sau olusturulamadi"; fi

# --- TEST 2: İsimli arşiv ---
echo ""
echo "--- Test 2: Isimli arsiv (-o test.sau) ---"
./tarsau -b t1.txt t2.txt -o test.sau
if [ -f test.sau ]; then ok "test.sau olusturuldu"
else err "test.sau olusturulamadi"; fi

# --- TEST 3: Arşiv açma ---
echo ""
echo "--- Test 3: Arsiv acma ---"
./tarsau -a test.sau cikti
if [ -f cikti/t1.txt ] && [ -f cikti/t2.txt ]; then
    ok "Dosyalar cikti/ dizinine cikartildi"
else
    err "Dosyalar cikartılamadi"
fi

# --- TEST 4: İçerik doğrulaması ---
echo ""
echo "--- Test 4: Icerik dogrulama ---"
ORIG=$(cat t1.txt)
EXTR=$(cat cikti/t1.txt 2>/dev/null)
if [ "$ORIG" = "$EXTR" ]; then ok "t1.txt icerigi eslesiyor"
else err "t1.txt icerigi ESLESMEDI"; fi

# --- TEST 5: Binary dosya reddi ---
echo ""
echo "--- Test 5: Binary dosya reddedilmeli ---"
./tarsau -b binary.bin -o binary.sau 2>&1 | grep -q -E "Binary|binary|Hata"
if [ $? -eq 0 ]; then ok "Binary dosya reddedildi"
else err "Binary dosya kabul edildi (olmamali)"; fi

# --- TEST 6: Bozuk arşiv ---
echo ""
echo "--- Test 6: Bozuk arsiv tespiti ---"
echo "BOZUKDOSYA" > corrupt.sau
./tarsau -a corrupt.sau 2>&1 | grep -q -E "uygunsuz|bozuk"
if [ $? -eq 0 ]; then ok "Bozuk arsiv tespit edildi"
else err "Bozuk arsiv tespit EDILEMEDi"; fi
rm -f corrupt.sau

# --- TEST 7: İzin korunumu ---
echo ""
echo "--- Test 7: Dosya izinleri korunuyor mu? ---"
PERM_ORIG=$(stat -c "%a" t1.txt 2>/dev/null || stat -f "%OLp" t1.txt 2>/dev/null)
PERM_EXTR=$(stat -c "%a" cikti/t1.txt 2>/dev/null || stat -f "%OLp" cikti/t1.txt 2>/dev/null)
if [ "$PERM_ORIG" = "$PERM_EXTR" ]; then ok "Izinler korunuyor ($PERM_ORIG)"
else err "Izinler korunmuyor (orijinal=$PERM_ORIG, cikartilan=$PERM_EXTR)"; fi

# --- TEST 8: Arşiv açma, mevcut dizin ---
echo ""
echo "--- Test 8: Arsiv acma mevcut dizine (dizin belirtilmez) ---"
mkdir -p test_dir && cd test_dir
../tarsau -a ../test.sau 2>/dev/null
if [ -f t1.txt ]; then ok "Mevcut dizine cikartildi"
else err "Mevcut dizine cikartılamadı"; fi
cd ..

# Sonuç
echo ""
echo "================================"
echo "SONUC: $PASS test gecti, $FAIL test basarisiz"
echo "================================"

cleanup
rm -rf test_dir
