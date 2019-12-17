#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

/*
    ID3v1 identification -> 3 characters (the word "TAG")

    Song Title -> 30 characters
    Artist -> 30 characters
    Album -> 30 characters
    Year -> 4 characters
    Comment -> 30 characters
    Genre -> 1 byte 
*/

#define len(a) (sizeof(a)/sizeof(a[0]))

char const *all_genres[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
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

void MP3_Free(ID3v1 *tags) {
    free(tags);
}

void MP3_WriteTags(ID3v1 *tags, FILE *fp) {
    fseek(fp, -128, SEEK_END);

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

void copy_and_terminate(char *dest, char *src, int size) {
    strncpy(dest, src, size);
    dest[size-1] = 0;
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
    copy_and_terminate(tags->Year, year, 5);
}

void MP3_SetAlbum(ID3v1 *tags, char *album) {
    copy_and_terminate(tags->Album, album, 31);
}

void MP3_SetArtist(ID3v1 *tags, char *artist) {
    copy_and_terminate(tags->Artist, artist, 31);
}

void MP3_SetTitle(ID3v1 *tags, char *title) {
    copy_and_terminate(tags->Title, title, 31);
}

void MP3_SetGenre(ID3v1 *tags, char genreID) {
    tags->genreID = genreID;
    if (tags->genreID < len(all_genres)) {
        copy_and_terminate(tags->Genre, (char *) all_genres[tags->genreID], 51);
    }
}

void MP3_SetComment(ID3v1 *tags, char *comment) {
    copy_and_terminate(tags->Comment, comment, 31);
}

void MP3_SetHeader(ID3v1 *tags) {
    copy_and_terminate(tags->header, "TAG", 4);
}

void MP3_SetDefaultHeaders(ID3v1 *tags) {
    MP3_SetTitle(tags, "Title");
    MP3_SetAlbum(tags, "Album");
    MP3_SetArtist(tags, "Artist");
    MP3_SetYear(tags, "0000");
    MP3_SetComment(tags, "");
    MP3_SetGenre(tags, 0);
}

void MP3_CloseFile(ID3v1 *tags) {
    // write tags
    MP3_WriteTags(tags, tags->f);
    fclose(tags->f);
    MP3_Free(tags);
}

ID3v1 *MP3_OpenFile(char const *path) {
    ID3v1 *tags = (ID3v1 *) malloc(sizeof(ID3v1));

    tags->f = fopen(path, "r+");
    if (tags->f == NULL) {
        MP3_Free(tags);
        return NULL;
    }

    fseek(tags->f, -128, SEEK_END);

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

void MP3_CopyFile(ID3v1 *tags, char *out_dir) {
    char path[200] = {0};
    sprintf(path, "%s/%s - %s.mp3", out_dir, MP3_Artist(tags), MP3_Title(tags));
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

int parse_flags(int argc, char **argv, char **in_dir, char **out_dir) {
    if (argc < 3) {
        return 1;
    }

    int i;
    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "i=") == argv[i]) {
            *in_dir = (char *) malloc(strlen(argv[i]+2) + 1);
            copy_and_terminate(*in_dir, argv[i]+2, strlen(argv[i]+2) + 1);
        }

        if (strstr(argv[i], "o=") == argv[i]) {
            *out_dir = (char *) malloc(strlen(argv[i]+2) + 1);
            copy_and_terminate(*out_dir, argv[i]+2, strlen(argv[i]+2) + 1);
        }
    }

    return 0;
}

char const *usage = "usage: mp3rec i=<input directory> o=<output directory>\n";

int main(int argc, char **argv) {
    char *in = NULL, *out = NULL;

    int res = parse_flags(argc, argv, &in, &out);
    if (res != 0 || in == NULL || out == NULL) {
        printf(usage);
        return 1;
    }

    printf("scanning %s and outputing to %s\n", in, out);

    DIR *dir = opendir(in);
    if (dir == NULL) {
        printf("can't open directory %s\n", in);
        return 1;
    }

    int total = 0;
    char in_path[200] = { 0 };

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strstr(de->d_name, ".mp3") == NULL) {
            continue;
        }

        sprintf(in_path, "%s/%s", in, de->d_name);

        ID3v1 *t = MP3_OpenFile(in_path);
        if (t == NULL) {
            printf("error opening file: %s\n", in_path);
            continue;
        }

        if (!MP3_HeaderValid(t)) {
            MP3_SetDefaultHeaders(t);
            MP3_CloseFile(t);
            continue;
        }

        total++;
        printf("%d. %s - %s\n", total, MP3_Artist(t), MP3_Title(t));
        MP3_CopyFile(t, out);

    }
    closedir(dir);

    return 0;
}
