#ifndef MP3_H
#define MP3_H

#include <stdint.h>
#include <stdio.h>
//This File contains functions and structures related to get towards playable PCM and save data for playing.

// Completly optional, but for better clarity i like to use an enum to return the status of the task
typedef enum taskStatus
{
    SUCCESS = 0,
    FAILED = 1
} taskStatus;


/*MP3 Frame Header
Byte 1: 1111 1111 (part of syncword)
Byte 2:
    bit 0: Protection bit
    bit 1 - 2: Layer
    bit 3 - 4: MPEG Version
    bit 5 - 7: 111 (part of syncword)
Byte 3:
    bit 0: private bit
    bit 1: padding
    bit 2 - 3: sample Rate index
    bit 4 - 7: bitrate Index
Byte 4:
    bit 0 - 1: Emphasis
    bit 2: original
    bit 3: copyright
    bit 4 - 5: mode extension
    bit 6 - 7: channel mode 
*/
//MP3 Frame header struct to save data on how we play the file
    //Also needed for decoding
struct mp3FrameHeader
{
    uint8_t modeChannels; //0 = Stereo, 1 = Joint Stereo, 2 = Dual Channel, 3 = Single Channel
    uint8_t numChannels; //1 for mono, 2 for stereo. Can be derived from modeChannels but is easier to have it as a separate field
    uint8_t layer;  //1 = Layer III, 2 = Layer II, 3 = Layer I, we will only cover Layer III
    uint8_t mpegVersion; //0 = MPEG Version 2.5, 1 = reserved, 2 = MPEG Version 2, 3 = MPEG Version 1
    uint16_t bitrate; // in kbps, can be derived from bitrateIndex but is easier to have it as a separate field
    uint8_t bitrateIndex;   //Used to calculate bitrate. In the heaer bitrateIndex is saved. The actual bitrate can be derived from this index with the help of the mpegVersion and layer. For example for Layer III and MPEG Version 1, a bitrateIndex of 0b0001 would mean a bitrate of 32 kbps, 0b0010 would mean 40 kbps and so on. You can find the full table in the mp3 specification.
     //The same goes for samplerateIndex
    uint32_t samplerate;
    uint8_t samplerateIndex;

    uint8_t padding;    
    uint8_t protectionAbsent;   //If there is protection we need to skip 2 bytes of crc after reading header
    uint8_t modeExtension;
};

//Functions

//This functions checks a given File if it is an mp3 file.
//It first check for an ID3v2 header and evaluates it.
//We skip it anyways and try to check for frames.
taskStatus validateMP3File(FILE *f);



#endif