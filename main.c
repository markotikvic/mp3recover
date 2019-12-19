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

/*
    ID3v1 identification:  3 characters ("TAG")
    Song Title:           30 characters
    Artist:               30 characters
    Album:                30 characters
    Year:                  4 characters
    Comment:              30 characters
    Genre:                 1 byte
*/

#define ID3v1_METADATA_SIZE 128
#define MAX_PATH_SIZE       4096

char const *usage = "usage: mp3rec -i=<input directory> -o=<output directory>\n";

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir);
int list_dir(char *path, char ***files, int *files_n);
int count_files(DIR *dir, char *file_ext);
void sort_files(char ***files, int files_n);

int  is_id3v1(FILE *fp);
void read_id3v1_tag(FILE *fp, char *dest, long end_offset, int size);
void read_id3v1_artist(FILE *fp, char *dest);
void read_id3v1_title(FILE *fp, char *dest);

void read_id3v2_artist(struct id3_file *id3file, char *dest);
void read_id3v2_title(struct id3_file *id3file, char *dest);

int copy_file(char *src, char *dest);
void make_out_path(char *path_out, char *out_dir, char *artist, char *title, int recovered);

int main(int argc, char **argv) {
    char in[MAX_PATH_SIZE]  = {0};
    char out[MAX_PATH_SIZE] = {0};
    if (parse_flags(argc, argv, in, out) != 0) {
        printf("%s", usage);
        return 1;
    }

    char **files;
    int files_n = 0;
    if (list_dir(in, &files, &files_n) != 0) {
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

        // TRY ID3V2 TAG
        struct id3_file *id3file = id3_file_open(path_in, ID3_FILE_MODE_READONLY);
        if (id3file != NULL) {
            read_id3v2_artist(id3file, artist);
            read_id3v2_title(id3file, title);
            id3_file_close(id3file);
        }

        // TRY ID3V1 TAG
        FILE *fp = fopen(path_in, "r");
        if (fp != NULL) {
            if (is_id3v1(fp)) {
                if (strlen(artist) == 0) {
                    read_id3v1_artist(fp, artist);
                }
                if (strlen(title) == 0) {
                    read_id3v1_title(fp, title);
                }
            }
            fclose(fp);
        }

        scanned++;
        if (strlen(title) == 0 && strlen(artist) == 0) {
            printf("can't recover %s's name: missing metadata\n", files[i]);
            continue;
        }
        recovered++;

        make_out_path(path_out, out, artist, title, recovered);

        printf("%d. %s - %s\n", recovered, artist, title);

        copy_file(path_in, path_out);
    }

    float recp = scanned > 0 ? (float) recovered / (float) scanned * 100.0 : 0.0;
    printf("\nscanned %d file(s), recovered %d name(s) (%.2f%%)\n", scanned, recovered, recp);

    return 0;
}

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir) {
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

int count_files(DIR *dir, char *file_ext) {
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

void sort_files(char ***files, int files_n) {
    for (int i = 0; i < files_n - 1; i++) {
        for (int j = i; j < files_n; j++) {
            if (strcmp((*files)[i], (*files)[j]) == 1) {
                char *t = (*files)[i];
                (*files)[i] = (*files)[j];
                (*files)[j] = t;
            }
        }
    }
}

int list_dir(char *path, char ***files, int *files_n) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return 1;
    }

    int n = count_files(dir, "mp3");
    *files_n = n;
    *files = (char **) malloc(sizeof(char *) * n);

    n = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, ".mp3") == NULL) {
            continue;
        }
        char **temp = &(*files)[n];
        *temp = (char *) malloc(strlen(de->d_name) + 1);
        strcpy(*temp, de->d_name);
        n++;
    }
    closedir(dir);


    sort_files(files, *files_n);

    return 0;
}

int copy_file(char *src, char *dest) {
    FILE *fin = fopen(src, "r");
    if (fin == NULL) {
        return 1;
    }

    FILE *fp = fopen(dest, "w+");
    if (fp == NULL) {
        fclose(fin);
        return 1;
    }

    int c = 1;

    rewind(fin);
    while ((c = fgetc(fin)) != EOF) {
        fputc(c, fp);
    }
    fclose(fp);
    fclose(fin);

    return 0;
}

int is_id3v1(FILE *fp) {
    char header[4] = { 0 };
    fseek(fp, -ID3v1_METADATA_SIZE, SEEK_END);
    fgets(header, 4, fp);
    return (strcmp(header, "TAG") == 0);
}

void read_id3v1_tag(FILE *fp, char *dest, long end_offset, int size) {
    fseek(fp, end_offset, SEEK_END);
    for (int i = 0; i < size; i++) {
        dest[i] = fgetc(fp);
    }
    dest[size] = 0;
}

void read_id3v1_artist(FILE *fp, char *dest) {
    read_id3v1_tag(fp, dest, -ID3v1_METADATA_SIZE+3+30, 30);
}

void read_id3v1_title(FILE *fp, char *dest) {
    read_id3v1_tag(fp, dest, -ID3v1_METADATA_SIZE+3, 30);
}

void read_id3v2_artist(struct id3_file *id3file, char *dest) {
    struct id3_tag *id3tag = id3_file_tag(id3file);
    struct id3_frame *artist_frame = id3_tag_findframe(id3tag, "TPE1", 0);
    if (artist_frame != NULL) {
        unsigned int n_string = id3_field_getnstrings(&artist_frame->fields[1]);
        for (int j = 0; j < n_string; j++) {
            id3_ucs4_t const *tempstr = id3_field_getstrings(&artist_frame->fields[1], j);
            id3_latin1_t *dup = id3_ucs4_latin1duplicate(tempstr);
            strcpy(dest, (char *) dup);
            free(dup);
        }
    }
}

void read_id3v2_title(struct id3_file *id3file, char *dest) {
    struct id3_tag *id3tag = id3_file_tag(id3file);
    struct id3_frame *title_frame = id3_tag_findframe(id3tag, "TIT2", 0);
    if (title_frame != NULL) {
        unsigned int n_string = id3_field_getnstrings(&title_frame->fields[1]);
        for (int j = 0; j < n_string; j++) {
            id3_ucs4_t const *tempstr = id3_field_getstrings(&title_frame->fields[1], j);
            id3_latin1_t *dup = id3_ucs4_latin1duplicate(tempstr);
            strcpy(dest, (char *) dup);
            free(dup);
        }
    }
}

void make_out_path(char *path_out, char *out_dir, char *artist, char *title, int recovered) {
    if (strlen(artist) != 0 && strlen(title) != 0) {
        sprintf(path_out, "%s/%s - %s.mp3", out_dir, artist, title);
    } else if (strlen(title) != 0) {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, title, recovered);
    } else {
        sprintf(path_out, "%s/%s (%d).mp3", out_dir, artist, recovered);
    }
}
