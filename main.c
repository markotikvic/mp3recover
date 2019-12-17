#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#define len(x) (sizeof(x)/sizeof(x[0]))

char const *usage = "usage: mp3rec i=<input directory> o=<output directory>\n";

char const *all_genres[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
    "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative",
    "Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel",
    "Noise", "AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream",
    "Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
    "Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
    "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock",
};

typedef struct {
    char header[4];
    char Title[31];
    char Artist[31];
    char Album[31];
    char Year[5];
    char Comment[31];
    char Genre[51];
    char genreID;
    FILE *f;
} ID3v1;

int  parse_flags(int argc, char **argv, char **in_dir, char **out_dir);
void copy_and_terminate(char *dest, char *src, int size);
int  readable(char *str);
void cleanup_tags(ID3v1 *tags);

ID3v1 *MP3_OpenFile(char *path);
void  MP3_CloseFile(ID3v1 *tags);
void  MP3_CopyFile(ID3v1 *tags, char *path);

void MP3_Free(ID3v1 *tags);
void MP3_WriteTags(ID3v1 *tags, FILE *fp);
int  MP3_HeaderValid(ID3v1 *tags);
char *MP3_Year(ID3v1 *tags);
char *MP3_Album(ID3v1 *tags);
char *MP3_Artist(ID3v1 *tags);
char *MP3_Title(ID3v1 *tags);
char *MP3_Genre(ID3v1 *tags);
char *MP3_Comment(ID3v1 *tags);

void MP3_SetYear(ID3v1 *tags, char *year);
void MP3_SetAlbum(ID3v1 *tags, char *album);
void MP3_SetArtist(ID3v1 *tags, char *artist);
void MP3_SetTitle(ID3v1 *tags, char *title);
void MP3_SetGenre(ID3v1 *tags, char genreID);
void MP3_SetComment(ID3v1 *tags, char *comment);
void MP3_SetHeader(ID3v1 *tags);

int main(int argc, char **argv) {
    char *in = NULL, *out = NULL;
    if (parse_flags(argc, argv, &in, &out) != 0) {
        printf(usage);
        return 1;
    }

    DIR *dir = opendir(in);
    if (dir == NULL) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int recovered = 0, scanned = 0;
    char path[200] = { 0 };

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, ".mp3") == NULL) {
            continue;
        }

        sprintf(path, "%s/%s", in, de->d_name);

        ID3v1 *t = MP3_OpenFile(path);
        if (t == NULL) {
            printf("error opening file: %s\n", path);
            continue;
        }

        scanned++;
        if (!MP3_HeaderValid(t)) {
            printf("can't recover %s's name: missing header\n", de->d_name);
            MP3_CloseFile(t);
            continue;
        }

        char *artist = MP3_Artist(t);
        char *title  = MP3_Title(t);

        if (strlen(artist) == 0 && strlen(title) == 0) {
            printf("can't recover %s's name: missing metadata\n", de->d_name);
            MP3_CloseFile(t);
            continue;
        }

        recovered++;
        printf("%d. %s - %s\n", recovered, artist, title);

        sprintf(path, "%s/%s - %s.mp3", out, artist, title);
        MP3_CopyFile(t, path);

    }
    closedir(dir);

    float recp = (float) recovered / (float) scanned * 100.0;
    printf("\nscanned %d file(s): recovered %d names (%.2f)\n", scanned, recovered, recp);

    return 0;
}

int parse_flags(int argc, char **argv, char **in_dir, char **out_dir) {
    if (argc < 3) {
        return 1;
    }

    int i;
    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "i=") == argv[i]) {
            *in_dir = (char *) malloc(strlen(argv[i]+2) + 1);
            copy_and_terminate(*in_dir, argv[i]+2, strlen(argv[i]+2));
        }

        if (strstr(argv[i], "o=") == argv[i]) {
            *out_dir = (char *) malloc(strlen(argv[i]+2) + 1);
            copy_and_terminate(*out_dir, argv[i]+2, strlen(argv[i]+2));
        }
    }

    if (*in_dir == NULL || *out_dir == NULL) {
        return 1;
    }

    return 0;
}

void copy_and_terminate(char *dest, char *src, int size) {
    strncpy(dest, src, size);
    dest[size] = 0;
}

int readable(char *str) {
    int len = strlen(str);

    int i;
    for (i = 0; i < len; i++) {
        if (!isalnum(str[i])) {
            return 0;
        }
    }
    return 1;
}

void cleanup_tags(ID3v1 *tags) {
    if (!readable(tags->Title)) {
        memset(tags->Title, 0, 31);
    }
    if (!readable(tags->Artist)) {
        memset(tags->Artist, 0, 31);
    }
    if (!readable(tags->Album)) {
        memset(tags->Album, 0, 31);
    }
    if (!readable(tags->Year)) {
        memset(tags->Year, 0, 5);
    }
    if (!readable(tags->Comment)) {
        memset(tags->Comment, 0, 31);
    }
    if (!readable(tags->Genre), 0, 51) {
        memset(tags->Genre, 0, 51);
        tags->genreID = 0;
    }
}

void MP3_Free(ID3v1 *tags) {
    free(tags);
}

void MP3_WriteTags(ID3v1 *tags, FILE *fp) {
    fseek(fp, -ID3v1_METADATA_SIZE, SEEK_END);

    int i;
    for (i = 0; i < 3; i++) {
        fputc(tags->header[i], fp);
    }
    for (i = 0; i < 30; i++) {
        fputc(tags->Title[i], fp);
    }
    for (i = 0; i < 30; i++) {
        fputc(tags->Artist[i], fp);
    }
    for (i = 0; i < 30; i++) {
        fputc(tags->Album[i], fp);
    }
    for (i = 0; i < 4; i++) {
        fputc(tags->Year[i], fp);
    }
    for (i = 0; i < 30; i++) {
        fputc(tags->Comment[i], fp);
    }
    fputc(tags->genreID, fp);
}

int MP3_HeaderValid(ID3v1 *tags) {
    return strcmp(tags->header, "TAG") == 0;
}

char *MP3_Year(ID3v1 *tags) {
    return tags->Year;
}

char *MP3_Album(ID3v1 *tags) {
    return tags->Album;
}

char *MP3_Artist(ID3v1 *tags) {
    return tags->Artist;
}

char *MP3_Title(ID3v1 *tags) {
    return tags->Title;
}

char *MP3_Genre(ID3v1 *tags) {
    return tags->Genre;
}

char *MP3_Comment(ID3v1 *tags) {
    return tags->Comment;
}

void MP3_SetYear(ID3v1 *tags, char *year) {
    copy_and_terminate(tags->Year, year, 4);
}

void MP3_SetAlbum(ID3v1 *tags, char *album) {
    copy_and_terminate(tags->Album, album, 30);
}

void MP3_SetArtist(ID3v1 *tags, char *artist) {
    copy_and_terminate(tags->Artist, artist, 30);
}

void MP3_SetTitle(ID3v1 *tags, char *title) {
    copy_and_terminate(tags->Title, title, 30);
}

void MP3_SetGenre(ID3v1 *tags, char genreID) {
    tags->genreID = genreID;
    if (tags->genreID < len(all_genres)) {
        copy_and_terminate(tags->Genre, (char *) all_genres[tags->genreID], 50);
    }
}

void MP3_SetComment(ID3v1 *tags, char *comment) {
    copy_and_terminate(tags->Comment, comment, 30);
}

void MP3_SetHeader(ID3v1 *tags) {
    copy_and_terminate(tags->header, "TAG", 3);
}

void MP3_CloseFile(ID3v1 *tags) {
    MP3_WriteTags(tags, tags->f);
    fclose(tags->f);
    MP3_Free(tags);
}

ID3v1 *MP3_OpenFile(char *path) {
    ID3v1 *tags = (ID3v1 *) malloc(sizeof(ID3v1));

    tags->f = fopen(path, "r+");
    if (tags->f == NULL) {
        MP3_Free(tags);
        return NULL;
    }

    fseek(tags->f, -ID3v1_METADATA_SIZE, SEEK_END);

    fgets(tags->header,   4, tags->f);
    if (!MP3_HeaderValid(tags)) {
        return tags;
    }
    fgets(tags->Title,   31, tags->f);
    fgets(tags->Artist,  31, tags->f);
    fgets(tags->Album,   31, tags->f);
    fgets(tags->Year,     5, tags->f);
    fgets(tags->Comment, 31, tags->f);
    MP3_SetGenre(tags, fgetc(tags->f));

    cleanup_tags(tags);

    return tags;
}

void MP3_CopyFile(ID3v1 *tags, char *path) {
    FILE *fp = fopen(path, "w+");

    rewind(tags->f);
    int c;
    while ((c = fgetc(tags->f)) != EOF) {
        fputc(c, fp);
    }
    MP3_WriteTags(tags, fp);
    fclose(fp);
    fclose(tags->f);
}
