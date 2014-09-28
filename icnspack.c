#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct format {
    const char tag[5];
    const char *filename;
    uint8_t *data;
    off_t size;
};

static struct format formats[] = {
    {"ic07", "icon_128x128.png", NULL, 0},
    {"ic08", "icon_256x256.png", NULL, 0},
    {"ic09", "icon_512x512.png", NULL, 0},
    {"ic10", "icon_256x256@2x.png", NULL, 0},
    {"ic11", "icon_16x16@2x.png", NULL, 0},
    {"ic12", "icon_32x32@2x.png", NULL, 0},
    {"ic13", "icon_128x128@2x.png", NULL, 0},
    {"ic14", "icon_512x512@2x.png", NULL, 0},
    {"icp4", "icon_16x16.png", NULL, 0},
    {"icp5", "icon_32x32.png", NULL, 0},
};

static void write_size(int fd, uint32_t size) {
    size = __builtin_bswap32(size);
    write(fd, &size, 4);
}

int main(int argc, const char **argv) {
    if (argc != 3) {
        puts("usage: icnspack input.iconset output.icns");
        return 1;
    }

    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", argv[1], formats[i].filename);
        int fd = open(path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Missing: %s\n", path);
            continue;
        }

        formats[i].size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        formats[i].data = malloc(formats[i].size);
        read(fd, formats[i].data, formats[i].size);
        close(fd);
    }

    int fd = open(argv[2], O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        fprintf(stderr, "Failed to open output file: %s\n", argv[2]);
        return 1;
    }

    // Write header
    write(fd, "icns", 4);

    uint32_t size = 16;
    uint32_t toc_size = 8;
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        if (formats[i].data) {
            size += 16 + formats[i].size;
            toc_size += 8;
        }
    }
    write_size(fd, size);

    // Write TOC
    write(fd, "TOC ", 4);
    write_size(fd, toc_size);
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        if (formats[i].data) {
            write(fd, formats[i].tag, 4);
            write_size(fd, formats[i].size);
        }
    }

    // Write images
    for (size_t i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i) {
        if (formats[i].data) {
            write(fd, formats[i].tag, 4);
            write_size(fd, formats[i].size + 8);
            write(fd, formats[i].data, formats[i].size);
        }
    }
}
