#ifndef WAV_H
#define WAV_H

#include "models.h"
#include <stdio.h>
#include <stdint.h>

//Checks if the given file is a real wav file
taskStatus validateWAVFile(FILE *f);

//Finds ID Fields in a wav field and puts the file at the position of data start, saves the size of datablock in uint32_t size
    //For Playback we need to find "fmt " and "data"
taskStatus findIDFieldData(FILE *f,uint32_t *size ,const char* IDName);

//Searches with findIDField "fmt " field and saves data into fmtHeader struct
taskStatus getFMTHeader(FILE *f, struct fmtHeader *header);

//2 Functions to use when you are at the point of pcm sample start, create header with fmtHeader and use findIDField with IDName "data" first
taskStatus getPCMFrame(FILE *f, struct fmtHeader *header, uint8_t* frameBuffer, int bufferSize);
size_t getPCMFramesBuffer(FILE *f, struct fmtHeader *header ,struct pcmBuffer *buffer);

#endif