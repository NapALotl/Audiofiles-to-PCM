#ifndef MODELS_H
#define MODELS_H

typedef enum ID3v2HeaderStatus
{
    NoID3v2Header = 0,
    ID3v2HeaderValid = 1,
    ID3v2HeaderBROKEN = 2
} ID3v2HeaderStatus;






struct mp3ID3v2Header
{
    uint8_t majorVersion;
    uint8_t revision;
    uint8_t flags;
    uint32_t size; //Size of the ID3v2 tag, excluding the header. This is a syncsafe integer, so the actual size can be calculated with (size[0] << 21) | (size[1] << 14) | (size[2] << 7) | size[3]
};






#endif