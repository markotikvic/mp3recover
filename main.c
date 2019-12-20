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
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <id3tag.h>

#include "util.h"

#define to_latin(s) id3_ucs4_latin1duplicate(s)

char const *usage = "usage: mp3rec -i=<input directory> -o=<output directory>\n";

typedef struct {
    char name[1024];
    char dir[MAX_PATH_SIZE];
} mp3_file;

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir);
mp3_file **list_dir(char *path, int *files_n);
mp3_file *new_mp3_file(char *path, char *name);
void sort_mp3_files(mp3_file **files, int files_n);
void id3v2_artist(struct id3_tag *tag, char *dest);
void id3v2_title(struct id3_tag *tag, char *dest);

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
    trim_suffix(in, "/");
    trim_suffix(out, "/");

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

        char *sub = make_subdir_path(out, trim_prefix(files[i]->dir, in));
        if (make_directory(sub) == 0) {
            filepath(path_out, sub, artist, title, recovered);
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

void id3v2_artist(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TPE1", 0);
    if (frame != 0) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
        sanitize_name(dest);
    }
}

void id3v2_title(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TIT2", 0);
    if (frame != 0) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
        sanitize_name(dest);
    }
}
