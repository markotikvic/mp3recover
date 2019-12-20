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

#include "util.h"

int main(int argc, char **argv) {
    char in[MAX_PATH_SIZE]  = {0};
    char out[MAX_PATH_SIZE] = {0};

    // default values
    strcpy(in, ".");
    strcpy(out, "./recovered");

    if (parse_flags(argc, argv, in, out) != 0) {
        printf("usage: mp3rec -i=<input directory> -o=<output directory>\n");
        return 1;
    }
    trim_suffix(in, "/");
    trim_suffix(out, "/");

    int files_n = 0;
    mp3_file **files = mp3list(in, &files_n);
    if (files == 0) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int recovered = 0;
    int scanned = files_n;
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
