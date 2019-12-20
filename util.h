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

#ifndef MP3_UTIL_H
#define MP3_UTIL_H

#include <stdio.h>
#include <id3tag.h>

#define MAX_PATH_SIZE 4096

#define to_latin(s) id3_ucs4_latin1duplicate(s)
#define to_utf8(s)  id3_ucs4_utf8duplicate(s)
#define to_utf16(s) id3_ucs4_utf16duplicate(s)

typedef struct {
    char name[1024];
    char dir[MAX_PATH_SIZE];
} mp3_file;

int parse_flags(int argc, char **argv, char *in_dir, char *out_dir);

int count_files(char *path, char *file_ext);
long fsize(FILE *fp);
void read_file(char *src, int **buf, long *nbuf);
void write_file(char *dest, int *buf, long nbuf);
void copy_file(char *src, char *dest);
void copy_and_dump(char *dest, char *src);
char *make_subdir_path(char *base, char *sub);
void filepath(char *path_out, char *out_dir, char *artist, char *title, int recovered);
int is_dir(char *path);
char *trim_prefix(char *str, char *pre);
void trim_suffix(char *str, char *suf);
int make_directory(char *path);
void sanitize_name(char *name);

mp3_file **mp3list(char *path, int *files_n);
mp3_file *new_mp3_file(char *path, char *name);
void sort_mp3_files(mp3_file **files, int files_n);
void id3v2_artist(struct id3_tag *tag, char *dest);
void id3v2_title(struct id3_tag *tag, char *dest);

#endif /* MP3_UTIL_H */
