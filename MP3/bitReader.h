#ifndef BITREADER_H
#define BITREADER_H

#include <stdint.h>

struct bitReader
{
    uint8_t *data; //The buffer we want to read from
    int bufferSize; //The size of the buffer in bytes
    int bytePosition; //The current byte position in the buffer
    int bitPosition; //The current bit position in the current byte (0-7)
};




//This function takes a bitReader and how many bits it should read and returns the value as uint32_t
uint32_t readBits(struct bitReader *reader, int numBits)
{
    uint32_t value = 0;
    for (int i = 0; i < numBits; i++)
    {
        if (reader->bytePosition >= reader->bufferSize)
        {
            fprintf(stderr, "readBits: Attempt to read past end of buffer\n");
            return 0; //You could also choose to return an error code or handle this differently
        }

        uint8_t currentByte = reader->data[reader->bytePosition];
        uint8_t bit = (currentByte >> (7 - reader->bitPosition)) & 1; //Get the current bit
        value = (value << 1) | bit; //Shift the value and add the new bit

        reader->bitPosition++;
        if (reader->bitPosition == 8) //If we have read all bits in the current byte, move to the next byte
        {
            reader->bitPosition = 0;
            reader->bytePosition++;
        }
    }
}

#endif