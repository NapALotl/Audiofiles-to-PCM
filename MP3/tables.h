#ifndef TABLES_H
#define TABLES_H

static const int bitrateTableMPEG1L3[16] = {
    0, 32, 40, 48, 56, 64, 80, 96,
    112, 128, 160, 192, 224, 256, 320, 0
};

static const int bitrateTableMPEG2L3[16] = {
    0, 8, 16, 24, 32, 40, 48, 56,
    64, 80, 96, 112, 128, 144, 160, 0
};


static const int samplerateTableMPEG1[4]  = {44100, 48000, 32000, 0};
static const int samplerateTableMPEG2[4]  = {22050, 24000, 16000, 0};
static const int samplerateTableMPEG25[4] = {11025, 12000, 8000, 0};


#endif