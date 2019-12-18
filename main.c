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

char const *usage = "usage: mp3rec i=<input directory> o=<output directory>\n";

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir);
int contains_header(FILE *fp);
void copy_file(FILE *fin, char *path);
void read_tag(FILE *fp, char *dest, long end_offset, int size);
void read_artist(FILE *fp, char *dest);
void read_title(FILE *fp, char *dest);

int main(int argc, char **argv) {
    char in[MAX_PATH_SIZE]  = {0};
    char out[MAX_PATH_SIZE] = {0};
    if (parse_flags(argc, argv, in, out) != 0) {
        printf(usage);
        return 1;
    }

    DIR *dir = opendir(in);
    if (dir == NULL) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int recovered = 0, scanned = 0;
    char path[MAX_PATH_SIZE] = {0};
    char artist[31] = {0};
    char title[31]  = {0};

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, ".mp3") == NULL) {
            continue;
        }

        sprintf(path, "%s/%s", in, de->d_name);

        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
            printf("error opening file: %s\n", path);
            continue;
        }

        scanned++;
        if (!contains_header(fp)) {
            printf("can't recover %s's name: missing header\n", de->d_name);
            fclose(fp);
            continue;
        }

        read_artist(fp, artist);
        read_title(fp, title);

        if (strlen(title) == 0 && strlen(artist) == 0) {
            printf("can't recover %s's name: missing metadata\n", de->d_name);
            fclose(fp);
            continue;
        }
        recovered++;

        if (strlen(artist) != 0 && strlen(title) != 0) {
            sprintf(path, "%s/%s - %s.mp3", out, artist, title);
        } else if (strlen(title) != 0) {
            sprintf(path, "%s/%s (%d).mp3", out, title, recovered);
        } else {
            sprintf(path, "%s/%s (%d).mp3", out, artist, recovered);
        }

        printf("%d. %s - %s\n", recovered, artist, title);

        copy_file(fp, path);
        fclose(fp);

    }
    closedir(dir);

    float recp = scanned > 0 ? (float) recovered / (float) scanned * 100.0 : 0.0;
    printf("\nscanned %d file(s), recovered %d name(s) (%.2f)\n", scanned, recovered, recp);

    return 0;
}

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir) {
    if (argc < 3) {
        return 1;
    }

    int i;
    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "i=") == argv[i]) {
            strcpy(in_dir, argv[i]+2);
        }

        if (strstr(argv[i], "o=") == argv[i]) {
            strcpy(out_dir, argv[i]+2);
        }
    }

    if (strlen(in_dir) == 0 || strlen(out_dir) == 0) {
        return 1;
    }

    return 0;
}

void copy_file(FILE *fin, char *path) {
    FILE *fp = fopen(path, "w+");
    int c = 1;

    rewind(fin);
    while ((c = fgetc(fin)) != EOF) {
        fputc(c, fp);
    }
    fclose(fp);
}

int contains_header(FILE *fp) {
    char header[4] = { 0 };
    fseek(fp, -ID3v1_METADATA_SIZE, SEEK_END);
    fgets(header, 4, fp);
    return (strcmp(header, "TAG") == 0);
}

void read_tag(FILE *fp, char *dest, long end_offset, int size) {
    fseek(fp, end_offset, SEEK_END);
    int i, c;
    for (i = 0; i < size; i++) {
        c = fgetc(fp);
        if (c == EOF || c == 0 || !isalnum(c)) {
            break;
        }
        dest[i] = (char) c;
    }
    dest[i] = 0;
}

void read_artist(FILE *fp, char *dest) {
    read_tag(fp, dest, -ID3v1_METADATA_SIZE+3+30, 30);
}

void read_title(FILE *fp, char *dest) {
    read_tag(fp, dest, -ID3v1_METADATA_SIZE+3, 30);
}
