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
#include <id3tag.h>

#define MAX_PATH_SIZE       4096

#define to_latin(s) id3_ucs4_latin1duplicate(s)

char const *usage = "usage: mp3rec -i=<input directory> -o=<output directory>\n";

static int parse_flags(int argc, char **argv, char *in_dir, char *out_dir) {
    if (argc < 3) {
        return 1;
    }

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

static int count_files(DIR *dir, char *file_ext) {
    int n = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, file_ext) == NULL) {
            continue;
        }
        n++;
    }
    rewinddir(dir);
    return n;
}

static void sort_files(char **files, int files_n) {
    for (int i = 0; i < files_n - 1; i++) {
        for (int j = i; j < files_n; j++) {
            if (strcmp(files[i], files[j]) == 1) {
                char *t = files[i];
                files[i] = files[j];
                files[j] = t;
            }
        }
    }
}

static char **list_dir(char *path, int *files_n) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return NULL;
    }

    *files_n = count_files(dir, "mp3");
    char **files = (char **) malloc(sizeof(char *) * (*files_n));

    int n = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, ".mp3") == NULL) {
            continue;
        }
        files[n] = (char *) malloc(strlen(de->d_name) + 1);
        strcpy(files[n], de->d_name);
        n++;
    }
    closedir(dir);

    sort_files(files, *files_n);

    return files;
}

static int copy_file(char *src, char *dest) {
    FILE *fin = fopen(src, "r");
    if (fin == NULL) {
        return 1;
    }

    FILE *fp = fopen(dest, "w+");
    if (fp == NULL) {
        fclose(fin);
        return 1;
    }

    int c;
    while ((c = fgetc(fin)) != EOF) {
        fputc(c, fp);
    }

    fclose(fp);
    fclose(fin);

    return 0;
}

static void copy_and_dump(char *dest, char *src) {
    strcpy(dest, src);
    free(src);
}

static void id3v2_artist(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TPE1", 0);
    if (frame != NULL) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
    }
}

static void id3v2_title(struct id3_tag *tag, char *dest) {
    struct id3_frame *frame = id3_tag_findframe(tag, "TIT2", 0);
    if (frame != NULL) {
        id3_ucs4_t const *tmp = id3_field_getstrings(&frame->fields[1], 0);
        copy_and_dump(dest, (char *) to_latin(tmp));
    }
}

static void filepath(char *path_out, char *out_dir, char *artist, char *title, int recovered) {
    if (strlen(artist) != 0 && strlen(title) != 0) {
        sprintf(path_out, "%s/%s - %s.mp3", out_dir, artist, title);
    } else if (strlen(title) != 0) {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, title, recovered);
    } else {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, artist, recovered);
    }
}

int main(int argc, char **argv) {
    char in[MAX_PATH_SIZE]  = {0};
    char out[MAX_PATH_SIZE] = {0};
    if (parse_flags(argc, argv, in, out) != 0) {
        printf("%s", usage);
        return 1;
    }

    int files_n = 0;
    char **files = list_dir(in, &files_n);
    if (files == NULL) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int recovered = 0, scanned = 0;

    for (int i = 0; i < files_n; i++) {
        char path_in[MAX_PATH_SIZE] = {0};
        char path_out[MAX_PATH_SIZE] = {0};
        char artist[1000] = {0};
        char title[1000]  = {0};

        sprintf(path_in, "%s/%s", in, files[i]);

        struct id3_file *id3file = id3_file_open(path_in, ID3_FILE_MODE_READONLY);
        if (id3file != NULL) {
            struct id3_tag *tag = id3_file_tag(id3file);
            id3v2_artist(tag, artist);
            id3v2_title(tag, title);
            id3_file_close(id3file);
        }

        scanned++;
        if (strlen(title) == 0 && strlen(artist) == 0) {
            printf("can't recover %s's name: missing ID3 tags\n", files[i]);
            continue;
        }
        recovered++;

        filepath(path_out, out, artist, title, recovered);

        printf("%d. %s - %s\n", recovered, artist, title);

        copy_file(path_in, path_out);
    }

    float perc = 0.0;
    if (scanned > 0) {
        perc = (float) recovered / (float) scanned * 100.0;
    }
    printf("\nscanned %d file(s), recovered %d name(s) (%.2f%%)\n", scanned, recovered, perc);

    return 0;
}
