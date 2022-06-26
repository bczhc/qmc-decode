//
// Created by bczhc on 25/06/22.
//

#include "qmcflac.h"
#include "def.h"
#include <cstdio>
#include <cassert>
#include <optional>
#include <iostream>

using namespace std;

u8 seedMap[8][7] = {
        {0x4a, 0xd6, 0xca, 0x90, 0x67, 0xf7, 0x52},
        {0x5e, 0x95, 0x23, 0x9f, 0x13, 0x11, 0x7e},
        {0x47, 0x74, 0x3d, 0x90, 0xaa, 0x3f, 0x51},
        {0xc6, 0x09, 0xd5, 0x9f, 0xfa, 0x66, 0xf9},
        {0xf3, 0xd6, 0xa1, 0x90, 0xa0, 0xf7, 0xf0},
        {0x1d, 0x95, 0xde, 0x9f, 0x84, 0x11, 0xf4},
        {0x0e, 0x74, 0xbb, 0x90, 0xbc, 0x3f, 0x92},
        {0x00, 0x09, 0x5b, 0x9f, 0x62, 0x66, 0xa1}
};

int x = -1;
int y = 8;
int dx = 1;
int i = -1;

u8 nextMask_() {
    u8 ret;
    while (true) {
        ++i;
        if (x < 0) {
            dx = 1;
            y = ((8 - y) % 8);
            ret = 0xc3;
        } else if (x > 6) {
            dx = -1;
            y = 7 - y;
            ret = 0xd8;
        } else {
            ret = seedMap[y][x];
        }
        x += dx;
        if (!(i == 0x8000 || (i > 0x8000 && !((i + 1) % 0x8000))))
            break;
    }
    return ret;
}

optional<u64> getFileSize(FILE *fp) {
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("seek error");
        return {};
    }
    auto pos = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("seek error");
        return {};
    }
    return pos;
}

int qmcflac::decode(const char *fileName, const char *destFileName) {
    x = -1;
    y = 8;
    dx = 1;
    i = -1;


    FILE *fp = fopen(fileName, "rb");
    FILE *fpO = fopen(destFileName, "wb");
    if (fp == nullptr || fpO == nullptr) {
        cerr << "Failed to open file" << endl;
        return 1;
    }
    u8 c[1024] = {0};
    auto fL = getFileSize(fp);
    if (!fL.has_value()) {
        fputs("Cannot get file size\n", stderr);
        return 1;
    }
    u64 a = fL.value() / 1024;
    int b = (int) (fL.value() % 1024);
    for (int j = 0; j < a; ++j) {
        auto size = fread(c, 1, 1024, fp);
        assert(size == 1024);
        for (unsigned char &k: c) {
            k ^= nextMask_();
        }
        size = fwrite(c, 1, 1024, fpO);
        assert(size == 1024);
    }
    if (b) {
        auto size = fread(c, 1, b, fp);
        assert(size == b);
        for (int j = 0; j < b; ++j) {
            c[j] ^= nextMask_();
        }
        size = fwrite(c, 1, b, fpO);
        assert(size == b);
    }
    if (fclose(fpO) != 0 || fclose(fp) != 0) {
        perror("Failed to close file");
        return 1;
    }
    return 0;
}