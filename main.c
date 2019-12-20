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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>

#include <id3tag.h>

#define MAX_PATH_SIZE 4096

#define to_latin(s) id3_ucs4_latin1duplicate(s)

typedef struct {
    char name[1024];
    char dir[MAX_PATH_SIZE];
} mp3_file;

char const *usage = "usage: mp3rec -i=<input directory> -o=<output directory>\n";

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir);
mp3_file **list_dir(char *path, int *files_n);
int count_files(char *path, char *file_ext);
long fsize(FILE *fp);
void read_file(char *src, int **buff, long *nbuff);
void write_file(char *dest, int *buff, long nbuff);
void copy_file(char *src, char *dest);
void copy_and_dump(char *dest, char *src);
void id3v2_artist(struct id3_tag *tag, char *dest);
void id3v2_title(struct id3_tag *tag, char *dest);
char *make_subdir_path(char *base, char *sub);
void filepath(char *path_out, char *out_dir, char *artist, char *title, int recovered);
int is_dir(char *path);
void sort_mp3_files(mp3_file **files, int files_n);
char *cut_base_dir(char *str, char *pre);
int make_directory_all(char *path);

int main(int argc, char **argv) {
    char in[MAX_PATH_SIZE]  = {0};
    char out[MAX_PATH_SIZE] = {0};

    // default values
    strcpy(in, ".");
    strcpy(out, "./recovered");

    if (parse_flags(argc, argv, in, out) != 0) {
        printf("%s", usage);
        return 1;
    }

    int files_n = 0;
    mp3_file **files = list_dir(in, &files_n);
    if (files == 0) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int recovered = 0, scanned = 0;
    for (int i = 0; i < files_n; i++) {
        char path_in[MAX_PATH_SIZE] = {0};
        char path_out[MAX_PATH_SIZE] = {0};
        char artist[1000] = {0};
        char title[1000]  = {0};

        sprintf(path_in, "%s/%s", files[i]->dir, files[i]->name);

        struct id3_file *id3file = id3_file_open(path_in, ID3_FILE_MODE_READONLY);
        if (id3file != 0) {
            struct id3_tag *tag = id3_file_tag(id3file);
            id3v2_artist(tag, artist);
            id3v2_title(tag, title);
            id3_file_close(id3file);
        }

        scanned++;
        if (strlen(title) == 0 && strlen(artist) == 0) {
            //printf("can't recover %s's name: missing ID3 tags\n", files[i]->name);
            continue;
        }
        recovered++;

        printf("%d. %s - %s\n", recovered, artist, title);

        char *sub = make_subdir_path(out, cut_base_dir(files[i]->dir, in));
        filepath(path_out, sub, artist, title, recovered);
        if (make_directory_all(sub) == 0) {
            copy_file(path_in, path_out);
        } else {
            printf("can't create directory %s\n", sub);
        };
        free(sub);
    }

    float perc = 0.0;
    if (scanned > 0) {
        perc = (float) recovered / (float) scanned * 100.0;
    }
    printf("\nscanned %d file(s), recovered %d name(s) (%.2f%%)\n", scanned, recovered, perc);

    return 0;
}

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir) {
    for (int i = 0; i < argc; i++) {
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
    return f;
}

mp3_file **list_dir(char *path, int *files_n) {
    *files_n = count_files(path, ".mp3");

    DIR *dir = opendir(path);
    if (dir == 0) {
        return 0;
    }

    mp3_file **files = (mp3_file **) malloc(sizeof(mp3_file *) * (*files_n));

    int i = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != 0) {
        char *sub = make_subdir_path(path, de->d_name);
        if (is_dir(sub)) {
            int subn = 0;
            mp3_file **subfiles = list_dir(sub, &subn);
            for (int j = 0; j < subn; j++) {
                files[i++] = subfiles[j];
            }
        }
        free(sub);

        if (strstr(de->d_name, ".mp3") == 0) {
            continue;
        }

        files[i++] = new_mp3_file(path, de->d_name);
    }
    closedir(dir);

    sort_mp3_files(files, *files_n);

    return files;
}

long fsize(FILE *fp) {
    long n = 0;
    fseek(fp, 0L, SEEK_END);
    n = ftell(fp);
    rewind(fp);
    return n;
}

void read_file(char *src, int **buff, long *nbuff) {
    FILE *fp = fopen(src, "r");
    if (fp == 0) {
        return;
    }
    long n = fsize(fp);
    *buff = (int *) malloc(n * sizeof(int));
    fread(*buff, sizeof(int), n, fp);
    *nbuff = n;
    fclose(fp);
}

void write_file(char *dest, int *buff, long nbuff) {
    FILE *fp = fopen(dest, "w");
    if (fp == 0) {
        return;
    }
    fwrite(buff, sizeof(int), nbuff, fp);
    fclose(fp);
}

void copy_file(char *src, char *dest) {
    int *buff;
    long nbuff;
    read_file(src, &buff, &nbuff);
    write_file(dest, buff, nbuff);
    if (buff != 0) {
        free(buff);
    }
}

void copy_and_dump(char *dest, char *src) {
    strcpy(dest, src);
    free(src);
}

void id3v2_artist(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TPE1", 0);
    if (frame != 0) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
    }
}

void id3v2_title(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TIT2", 0);
    if (frame != 0) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
    }
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

char *cut_base_dir(char *str, char *pre) {
    char *found = strstr(str, pre);
    if (found != str) { // found but not at the start of the string
        printf("prefix %s not found in %s\n", pre, str);
        return str;
    }

    return &str[strlen(pre)+1];
}

int make_directory_all(char *path) {
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
