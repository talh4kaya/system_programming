#include "tarsau.h"

/* ------------------------------------------------------------------ */
/*  Yardımcı: hata mesajı yazdır ve çık                                */
/* ------------------------------------------------------------------ */
void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/* ------------------------------------------------------------------ */
/*  Yardımcı: dosyanın text (ASCII) dosyası olup olmadığını kontrol et */
/* ------------------------------------------------------------------ */
int is_text_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;

    int c;
    while ((c = fgetc(f)) != EOF)
    {
        /* NULL byte varsa binary dosyadır (Türkçe UTF-8 karakterlerin geçmesi için c > 127 kontrolü kaldırıldı) */
        if (c == 0)
        {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Yardımcı: iç içe dizin oluştur (mkdir -p gibi)                     */
/* ------------------------------------------------------------------ */
void make_dir_recursive(const char *path)
{
    char tmp[MAX_PATH];
    char *p;

    snprintf(tmp, sizeof(tmp), "%s", path);
    /* sondaki / varsa kaldır */
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

/* ------------------------------------------------------------------ */
/*  PART 2: Argüman ayrıştırma                                         */
/* ------------------------------------------------------------------ */
void parse_args(int argc, char *argv[], Args *args)
{
    memset(args, 0, sizeof(Args));
    args->mode = -1;
    snprintf(args->output, sizeof(args->output), "%s", DEFAULT_OUTPUT);

    if (argc < 2)
    {
        die("Kullanim:\n"
            "  tarsau -b dosya1 dosya2 ... [-o arsiv.sau]\n"
            "  tarsau -a arsiv.sau [dizin]");
    }

    int i = 1;
    while (i < argc)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            args->mode = 0;
            i++;
            /* Sonraki argümanlar -o veya - ile başlamıyorsa girdi dosyasıdır */
            while (i < argc && argv[i][0] != '-')
            {
                if (args->input_count >= MAX_FILES)
                    die("Hata: En fazla 32 dosya arsivlenebilir.");
                snprintf(args->input_files[args->input_count],
                         MAX_PATH, "%s", argv[i]);
                args->input_count++;
                i++;
            }
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            i++;
            if (i >= argc)
                die("Hata: -o parametresinden sonra dosya adi bekleniyor.");
            snprintf(args->output, sizeof(args->output), "%s", argv[i]);
            i++;
        }
        else if (strcmp(argv[i], "-a") == 0)
        {
            args->mode = 1;
            
            /* -a parametresinden sonra en fazla 2 parametre almalıdır */
            int remaining_args = argc - (i + 1);
            if (remaining_args < 1)
                die("Hata: -a parametresinden sonra arsiv dosyasi bekleniyor.");
            if (remaining_args > 2)
                die("Hata: -a parametresinden sonra en fazla 2 parametre alinabilir.");

            /* Arşiv dosyası kontrolü (Tırnak işaretleri kaldırıldı) */
            size_t len = strlen(argv[i + 1]);
            if (len < 4 || strcmp(argv[i + 1] + len - 4, ".sau") != 0)
            {
                die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
            }

            snprintf(args->archive, sizeof(args->archive), "%s", argv[i + 1]);
            i += 2;

            /* Opsiyonel: hedef dizin */
            if (i < argc && argv[i][0] != '-')
            {
                snprintf(args->directory, sizeof(args->directory), "%s", argv[i]);
                i++;
            }
            else
            {
                snprintf(args->directory, sizeof(args->directory), ".");
            }
        }
        else
        {
            fprintf(stderr, "Bilinmeyen parametre: %s\n", argv[i]);
            die("Kullanim:\n"
                "  tarsau -b dosya1 dosya2 ... [-o arsiv.sau]\n"
                "  tarsau -a arsiv.sau [dizin]");
        }
    }

    if (args->mode == -1)
        die("Hata: -b veya -a parametresi gereklidir.");
}

/* ------------------------------------------------------------------ */
/*  PART 3: Arşiv oluşturma (-b)                                       */
/* ------------------------------------------------------------------ */
void create_archive(Args *args)
{
    if (args->input_count == 0)
        die("Hata: Arsivlenecek dosya belirtilmedi.");

    struct stat st;
    FileEntry entries[MAX_FILES];
    long total_size = 0;

    /* Dosyaları doğrula ve meta veri topla */
    for (int i = 0; i < args->input_count; i++)
    {
        const char *path = args->input_files[i];

        if (stat(path, &st) != 0)
        {
            const char *bn = strrchr(path, '/');
            bn = bn ? bn + 1 : path;
            /* BAŞTAKİ VE SONDAKİ tırnaklar kaldırıldı */
            fprintf(stderr, "%s giri\xc5\x9f dosyas\xc4\xb1n\xc4\xb1n format\xc4\xb1 uyumsuzdur!\n", bn);
            exit(1);
        }
        if (!S_ISREG(st.st_mode) || !is_text_file(path))
        {
            /* Sadece dosya adını al */
            const char *bn = strrchr(path, '/');
            bn = bn ? bn + 1 : path;
            /* BAŞTAKİ VE SONDAKİ tırnaklar kaldırıldı */
            fprintf(stderr, "%s giri\xc5\x9f dosyas\xc4\xb1n\xc4\xb1n format\xc4\xb1 uyumsuzdur!\n", bn);
            exit(1);
        }

        total_size += st.st_size;
        if (total_size > (long)MAX_TOTAL_SIZE)
            die("Hata: Toplam dosya boyutu 200 MB sinirini asiyor.");

        /* Sadece dosya adını al (path'ten son parçayı al) */
        const char *basename = strrchr(path, '/');
        if (basename)
            basename++;
        else
            basename = path;

        /* Dosya adında virgül veya dikey çizgi kontrolü */
        if (strchr(basename, ',') || strchr(basename, '|'))
        {
            fprintf(stderr, "Hata: Dosya adi ',' veya '|' karakterlerini iceremez: %s\n", basename);
            exit(1);
        }

        /* Derleyici uyarısını önlemek için sınır kontrolü yapıp strcpy kullandık */
        if (strlen(basename) >= MAX_FILENAME)
        {
            die("Hata: Giris dosya adi 256 karakterden uzun olamaz.");
        }
        strcpy(entries[i].name, basename);
        entries[i].permissions = st.st_mode & 0777;
        entries[i].size = st.st_size;
    }

    /* Index bölümünü oluştur: |ad,izin,boyut| formatında */
    /* Önce uzunluğu hesapla */
    char index_buf[64 * 1024]; /* 64 KB index için yeterli */
    int index_pos = 0;

    for (int i = 0; i < args->input_count; i++)
    {
        int written = snprintf(index_buf + index_pos,
                               sizeof(index_buf) - index_pos,
                               "|%s,%o,%ld|",
                               entries[i].name,
                               entries[i].permissions,
                               entries[i].size);
        if (written < 0 || (size_t)written >= sizeof(index_buf) - index_pos)
            die("Hata: Index boyutu cok buyuk.");
        index_pos += written;
    }
    index_buf[index_pos] = '\0';

    /* Index uzunluğunu 10 byte ASCII olarak hazırla */
    char len_str[INDEX_LEN_BYTES + 1];
    snprintf(len_str, sizeof(len_str), "%010d", index_pos);

    /* Arşiv dosyasını yaz */
    FILE *out = fopen(args->output, "wb");
    if (!out)
    {
        fprintf(stderr, "Hata: Arsiv dosyasi olusturulamadi: %s\n", args->output);
        exit(1);
    }

    /* 1) 10 byte uzunluk */
    fwrite(len_str, 1, INDEX_LEN_BYTES, out);

    /* 2) Index bölümü */
    fwrite(index_buf, 1, index_pos, out);

    /* 3) Dosya içerikleri */
    for (int i = 0; i < args->input_count; i++)
    {
        FILE *in = fopen(args->input_files[i], "rb");
        if (!in)
        {
            fprintf(stderr, "Hata: Dosya okunamadi: %s\n", args->input_files[i]);
            fclose(out);
            exit(1);
        }

        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
            fwrite(buf, 1, n, out);

        fclose(in);
    }

    fclose(out);
    printf("Dosyalar birle\xc5\x9ftirildi.\n");
}

/* ------------------------------------------------------------------ */
/*  PART 4: Arşiv açma (-a)                                            */
/* ------------------------------------------------------------------ */
void extract_archive(Args *args)
{
    FILE *arc = fopen(args->archive, "rb");
    if (!arc)
        die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");

    /* 1) İlk 10 byte: index uzunluğu */
    char len_str[INDEX_LEN_BYTES + 1];
    if (fread(len_str, 1, INDEX_LEN_BYTES, arc) != (size_t)INDEX_LEN_BYTES)
    {
        fclose(arc);
        die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
    }
    len_str[INDEX_LEN_BYTES] = '\0';

    /* Sayısal doğrulama */
    for (int i = 0; i < INDEX_LEN_BYTES; i++)
    {
        if (len_str[i] < '0' || len_str[i] > '9')
        {
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }
    }
    int index_len = atoi(len_str);
    if (index_len <= 0 || index_len > 100000)
    {
        fclose(arc);
        die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
    }

    /* 2) Index bölümünü oku */
    char *index_buf = malloc(index_len + 1);
    if (!index_buf)
        die("Hata: Bellek tahsis edilemedi.");

    if (fread(index_buf, 1, index_len, arc) != (size_t)index_len)
    {
        free(index_buf);
        fclose(arc);
        die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
    }
    index_buf[index_len] = '\0';

    /* 3) Index bölümünü parse et: |ad,izin,boyut| */
    FileEntry entries[MAX_FILES];
    int entry_count = 0;
    long total_size = 0;

    char *p = index_buf;
    while (*p)
    {
        if (entry_count >= MAX_FILES)
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }

        if (*p != '|')
        {
            p++;
            continue;
        }
        p++; /* '|' sonrası */

        /* Kaydın sonunu bul */
        char *end = strchr(p, '|');
        if (!end)
            break;
        *end = '\0';

        /* ad,izin,boyut */
        char *name = strtok(p, ",");
        char *perm = strtok(NULL, ",");
        char *szstr = strtok(NULL, ",");

        if (!name || !perm || !szstr)
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }

        /* Path Traversal Koruması */
        if (strchr(name, '/') || strchr(name, '\\') || strstr(name, ".."))
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }

        /* Negatif boyut ve limit kontrolleri */
        long size = atol(szstr);
        if (size < 0)
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }
        total_size += size;
        if (total_size > (long)MAX_TOTAL_SIZE)
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }

        /* İzin sayısal doğrulama */
        char *endptr;
        long perm_val = strtol(perm, &endptr, 8);
        if (*endptr != '\0' || perm_val < 0 || perm_val > 0777)
        {
            free(index_buf);
            fclose(arc);
            die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
        }

        snprintf(entries[entry_count].name, MAX_FILENAME, "%s", name);
        entries[entry_count].permissions = (mode_t)perm_val;
        entries[entry_count].size = size;
        entry_count++;

        p = end + 1;
    }
    free(index_buf);

    if (entry_count == 0)
    {
        fclose(arc);
        die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
    }

    /* 4) Hedef dizini oluştur */
    if (strcmp(args->directory, ".") != 0)
        make_dir_recursive(args->directory);

    /* 5) Dosyaları çıkart */
    for (int i = 0; i < entry_count; i++)
    {
        char out_path[MAX_PATH];
        /* Derleyici uyarısını önlemek için sınır kontrolü yapıp sprintf kullandık */
        if (strlen(args->directory) + 1 + strlen(entries[i].name) >= sizeof(out_path))
        {
            fclose(arc);
            die("Hata: Hedef dosya yolu limitini asiyor.");
        }
        sprintf(out_path, "%s/%s", args->directory, entries[i].name);

        FILE *out = fopen(out_path, "wb");
        if (!out)
        {
            fprintf(stderr, "Hata: Dosya olusturulamadi: %s\n", out_path);
            fclose(arc);
            exit(1);
        }

        /* İçeriği boyuta göre kopyala */
        long remaining = entries[i].size;
        char buf[4096];
        while (remaining > 0)
        {
            size_t chunk = (remaining > (long)sizeof(buf))
                               ? sizeof(buf)
                               : (size_t)remaining;
            size_t n = fread(buf, 1, chunk, arc);
            if (n == 0)
            {
                fclose(out);
                fclose(arc);
                die("Ar\xc5\x9fiv dosyas\xc4\xb1 uygunsuz veya bozuk!");
            }
            fwrite(buf, 1, n, out);
            remaining -= (long)n;
        }
        fclose(out);

        /* 6) Orijinal izinleri uygula */
        chmod(out_path, entries[i].permissions);
    }

    fclose(arc);

    /* Başarı mesajı: "d1 dizininde t1, t2, t3 dosyaları açıldı." */
    char file_list[MAX_FILES * MAX_FILENAME] = {0};
    for (int i = 0; i < entry_count; i++)
    {
        if (i > 0)
            strncat(file_list, ", ", sizeof(file_list) - strlen(file_list) - 1);
        strncat(file_list, entries[i].name, sizeof(file_list) - strlen(file_list) - 1);
    }
    printf("%s dizininde %s dosyalar\xc4\xb1 a\xc3\xa7\xc4\xb1ld\xc4\xb1.\n",
           args->directory, file_list);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    Args args;
    parse_args(argc, argv, &args);

    if (args.mode == 0)
        create_archive(&args);
    else
        extract_archive(&args);

    return 0;
}
