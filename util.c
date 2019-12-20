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

char *make_subdir_path(char *base, char *sub) {
    char *path = (char *) malloc(MAX_PATH_SIZE);
    strcpy(path, base);
    strcat(path, "/");
    strcat(path, sub);
    return path;
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
    while ((de = readdir(dir)) != 0) {
        char *sub = make_subdir_path(path, de->d_name);
        if (is_dir(sub)) {
            n += count_files(sub, file_ext);
        }
        free(sub);
        if (strstr(de->d_name, file_ext) == 0) {
            continue;
        }
        n++;
    }
    closedir(dir);
    return n;
}

long fsize(FILE *fp) {
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
    long n = fsize(fp);
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

void copy_and_dump(char *dest, char *src) {
    strcpy(dest, src);
    free(src);
}

void filepath(char *path_out, char *out_dir, char *artist, char *title, int recovered) {
    if (strlen(artist) != 0 && strlen(title) != 0) {
        sprintf(path_out, "%s/%s - %s.mp3", out_dir, artist, title);
    } else if (strlen(title) != 0) {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, title, recovered);
    } else {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, artist, recovered);
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

void trim_suffix(char *str, char *suf) {
    char *fnd = strstr(str, suf);
    if (fnd == 0) {
        return;
    }

    if (strlen(fnd) != strlen(suf)) {
        // not at the end
        return;
    }

    str[strlen(str) - strlen(suf)] = 0;
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
            if (c == 0) {
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

void sanitize_name(char *name) {
    int len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] == '/' || name[i] == '\\') {
            name[i] = '_';
        }
    }
}
