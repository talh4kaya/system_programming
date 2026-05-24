#ifndef TARSAU_H
#define TARSAU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_FILES        32
#define MAX_FILENAME     256
#define MAX_PATH         1024
#define MAX_TOTAL_SIZE   (200UL * 1024 * 1024)  /* 200 MB */
#define INDEX_LEN_BYTES  10
#define DEFAULT_OUTPUT   "a.sau"

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

typedef struct {
    char name[MAX_FILENAME];
    mode_t permissions;
    long size;
} FileEntry;

typedef struct {
    int   mode;          /* 0 = create (-b), 1 = extract (-a) */
    char  output[MAX_PATH];
    char  archive[MAX_PATH];
    char  directory[MAX_PATH];
    char  input_files[MAX_FILES][MAX_PATH];
    int   input_count;
} Args;

/* tarsau.c içindeki fonksiyonlar */
void  parse_args(int argc, char *argv[], Args *args);
int   is_text_file(const char *path);
void  create_archive(Args *args);
void  extract_archive(Args *args);
void  make_dir_recursive(const char *path);
void  die(const char *msg);

#endif /* TARSAU_H */
