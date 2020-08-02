/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include "util.h"

void make_subdir_path(char *path_out, char *base, char *sub) {
    strcpy(path_out, base);
    strcat(path_out, "/");
    strcat(path_out, sub);
}

int is_dir(char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }

    int dot = strrchr(path, '.') == &(path[strlen(path)-1]);
    return S_ISDIR(statbuf.st_mode) && !dot;

}

int count_files(char *path, char *file_ext) {
    DIR *dir = opendir(path);
    if (dir == 0) {
        return 0;
    }

    int n = 0;
    struct dirent *de;
    char sub[MAX_PATH_SIZE] = {0};
    while ((de = readdir(dir)) != 0) {
        memset(sub, 0, MAX_PATH_SIZE);
        make_subdir_path(sub, path, de->d_name);
        if (is_dir(sub)) {
            n += count_files(sub, file_ext);
        }
        if (strstr(de->d_name, file_ext) == 0) {
            continue;
        }
        n++;
    }
    closedir(dir);
    return n;
}

long file_size(FILE *fp) {
    long n = 0;
    fseek(fp, 0L, SEEK_END);
    n = ftell(fp);
    rewind(fp);
    return n;
}

void read_file(char *src, int **buf, long *nbuf) {
    FILE *fp = fopen(src, "r");
    if (fp == 0) {
        return;
    }
    long n = file_size(fp);
    *buf = (int *) malloc(n * sizeof(int));
    fread(*buf, sizeof(int), n, fp);
    *nbuf = n;
    fclose(fp);
}

void write_file(char *dest, int *buf, long nbuf) {
    FILE *fp = fopen(dest, "w");
    if (fp == 0) {
        return;
    }
    fwrite(buf, sizeof(int), nbuf, fp);
    fclose(fp);
}

void copy_file(char *src, char *dest) {
    int *buf;
    long nbuf;
    read_file(src, &buf, &nbuf);
    write_file(dest, buf, nbuf);
    if (buf != 0) {
        free(buf);
    }
}

void copy_and_free(char *dest, char *src) {
    strcpy(dest, src);
    free(src);
}

void make_filepath(char *path_out, char *out_dir, char *artist, char *title, int n) {
    if (strlen(artist) != 0 && strlen(title) != 0) {
        sprintf(path_out, "%s/%s - %s.mp3", out_dir, artist, title);
    } else if (strlen(title) != 0) {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, title, n);
    } else {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, artist, n);
    }
}

char *trim_prefix(char *str, char *pre) {
    char *found = strstr(str, pre);
    if (found != str) { // found but not at the start of the string
        printf("prefix %s not found in %s\n", pre, str);
        return str;
    }

    return &str[strlen(pre)+1];
}

char *trim_suffix(char *str, char *suf) {
    char *fnd = strstr(str, suf);
    if (fnd == 0) {
        return str;
    }

    if (strlen(fnd) != strlen(suf)) {
        return str;
    }

    str[strlen(str) - strlen(suf)] = 0;

    return str;
}

int make_directory(char *path) {
    struct stat st = {0};
    char temp_path[MAX_PATH_SIZE] = {0};
    int idx = 0;
    char c;

    while (1) {
        while ((c = path[idx]) != '/' && c != 0) {
            temp_path[idx] = c;
            idx++;
        }
        temp_path[idx] = c;
        idx++;

        if (stat(temp_path, &st) == 0) {
            if (c == 0) { // directory exists and we've reached end of path
                break;
            }
            continue;
        }

        if (mkdir(temp_path, 0700) != 0) {
            return 1;
        }
    }

    return 0;
}

char *sanitize_name(char *name) {
    int len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] == '/' || name[i] == '\\') {
            name[i] = '_';
        }
    }

    return name;
}

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir) {
    for (int i = 0; i < argc; i++) {
        if (strstr(argv[i], "-help") == argv[i]) {
            return 1;
        }

        if (strstr(argv[i], "-i=") == argv[i]) {
            strcpy(in_dir, argv[i]+3);
        }

        if (strstr(argv[i], "-o=") == argv[i]) {
            strcpy(out_dir, argv[i]+3);
        }
    }

    if (strlen(in_dir) == 0 || strlen(out_dir) == 0) {
        return 1;
    }

    return 0;
}

void sort_mp3_files(mp3_file **files, int files_n) {
    for (int i = 0; i < files_n - 1; i++) {
        for (int j = i; j < files_n; j++) {
            if (strcmp(files[i]->name, files[j]->name) == 1) {
                mp3_file *t = files[i];
                files[i] = files[j];
                files[j] = t;
            }
        }
    }
}

mp3_file *new_mp3_file(char *path, char *name) {
    mp3_file *f = (mp3_file *) malloc(sizeof(mp3_file));
    strcpy(f->name, name);
    strcpy(f->dir, path);
    sprintf(f->full_path, "%s/%s", f->dir, f->name);
    return f;
}

mp3_file **mp3_files_list(char *path, int *files_n) {
    *files_n = count_files(path, ".mp3");

    DIR *dir = opendir(path);
    if (dir == 0) {
        return 0;
    }

    mp3_file **files = (mp3_file **) malloc(sizeof(mp3_file *) * (*files_n));

    int i = 0;
    struct dirent *de;
    char sub[MAX_PATH_SIZE] = {0};
    while ((de = readdir(dir)) != 0) {
        memset(sub, 0, MAX_PATH_SIZE);
        make_subdir_path(sub, path, de->d_name);
        if (is_dir(sub)) {
            int subn = 0;
            mp3_file **subfiles = mp3_files_list(sub, &subn);
            for (int j = 0; j < subn; j++) {
                files[i++] = subfiles[j];
            }
        }

        if (strstr(de->d_name, ".mp3") == 0) {
            continue;
        }

        files[i++] = new_mp3_file(path, de->d_name);
    }
    closedir(dir);

    sort_mp3_files(files, *files_n);

    return files;
}

void id3v2_read_artist_title(struct id3_tag *tag, char *artist, char *title) {
    artist[0] = 0;
    title[0] = 0;

    struct id3_frame *frame = id3_tag_findframe(tag, "TPE1", 0);
    if (frame == 0) {
        return;
    }

    id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
    copy_and_free(artist, (char *) to_utf8(tmp));
    sanitize_name(artist);

    frame = id3_tag_findframe(tag, "TIT2", 0);
    if (frame == 0) {
        return;
    }

    tmp = id3_field_getstrings(&frame->fields[1], 0);
    copy_and_free(title, (char *) to_utf8(tmp));
    sanitize_name(title);
}
