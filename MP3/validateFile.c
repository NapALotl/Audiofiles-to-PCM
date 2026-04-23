#include <stdint.h> //uint used for buffers. uint8_t is 8 bits or 1 Byte which is perfect for mp3
#include <string.h> //So we can easily compare our buffers to keywords like "ID3"

#include "mp3.h"
#include "models.h" //Here are the tables saved for example for bitrate or samplerate

//Definitions
taskStatus _skipFrame(FILE *f, struct mp3FrameHeader *header);



/*
This functions checks a given File if it is an mp3 file.
It first check for an ID3v2 header and evaluates it.
We skip it anyways and try to check for frames.
Checks a few frames if they are consistent.
Skips to the first frame after it is successfull else stays at the current position.
*/
taskStatus validateMP3File(FILE *f)
{
    rewind(f);  //Makes sure we are at the start of the file
    uint8_t buffer[10]; //MP3 could start with an ID3v2 header of 10 Bytes, but it is not necessary. We will just check if there is an ID3v2 header and if not we will assume it is a valid MP3 file. This is not the best way to validate an MP3 file, but it is a simple way to check if there is an ID3v2 header and if not we will assume it is a valid MP3 file. A better way would be to check for the presence of MP3 frame headers, but that would require more complex parsing and is not necessary for our purposes.
    

    if (fread(buffer, 1, 10, f) != 10)
    {
        perror("validateMP3File: Error reading file");
        return FAILED;
    } 

    struct mp3ID3v2Header header;
    ID3v2HeaderStatus headerStatus = _testID3v2Header(buffer, &header);

    if (headerStatus == ID3v2HeaderBROKEN)
    //When the ID3 Header exists but is broken we will cancell since it is a good indicator that the file is not valid. 
        {
            fprintf(stdout, "validateMP3File: Invalid ID3v2 header found. We will cancell.\n");
            return FAILED;
    }
    if (headerStatus == ID3v2HeaderValid)
    //If it is a valid ID3v2 header we dont have to worry at all that it is an invalid file
        //However we must ignore 

    {
        fprintf(stdout, "validateMP3File: Valid ID3v2 header found. Already skipped ID3v2 header.\n");
        int skip = header.size;
        //If version is 2.4 we must also skip 10 bytes of footer
        if (header.majorVersion == 4)
            skip += 10;

        if (fseek(f, skip, SEEK_CUR) != 0)
        {
            fprintf(stdout, "validateMP3File: Error skipping ID3v2 header. This is a problem since we don't know where the frames start.\n");
            return FAILED;
        }
        
    }
        

    if (headerStatus == NoID3v2Header)
    //If there is no ID3v2 header the file could also just start with MP3 Frames
    {
        fprintf(stdout, "validateMP3File: No ID3v2 header found. Checking if file starts with MP3 Frames.\n");
        rewind(f); //We have to rewind the file since we read the first 10 bytes for the ID3v2 header check. 
        if (ferror(f))
        {
            fprintf(stdout, "validateMP3File: Error rewinding file after ID3v2 header check");
            return FAILED;
        } 
    }

    long frameStartPos = ftell(f); //Here we will save the position of the first MP3 Frame

    //We will read a few what we think are frames and if all pass we can be pretty sure that it is a valid mp3 file.
    struct mp3FrameHeader frameHeader;
    for (int i = 0; i < 4; i++)
    {
        if (_getMP3Header(f, &frameHeader) == FAILED)
        {
            
            fprintf(stdout, "validateMP3File: Read Invalid MP3 Frame Header. This is not a valid MP3 file.\n");
            return FAILED;
        }
    }
    if (skipFrame(f, &frameHeader) == FAILED) //After reading the header of the first frame we will skip it since we just want to check if the frames are valid and not actually read the samples yet.
    {
        fprintf(stdout, "validateMP3File: Error skipping frame. Aborting...\n");
        return FAILED;
    }
//Now after checking multiple frames we can be pretty sure that it is a valid MP3 file. We will rewind to the start of the first frame and return SUCCESS.
    fprintf(stdout, "validateMP3File: Successfully validated MP3 file. Rewinding to start of first frame.\n");
    
    if (fseek(f, frameStartPos, SEEK_SET) != 0)
    {
        fprintf(stdout, "validateMP3File: Error rewinding file to start of first frame after validation");
        return FAILED;
    }

    return SUCCESS;
    
}
//This Functions thinks it is at the beggining of a frame

taskStatus _skipFrame(
    FILE *f,
    struct mp3FrameHeader *header
)
{   

    size_t frameSize = _calculateFrameSize(header);
    if (frameSize == 0)
    {
        fprintf(stdout, "skipFrame: Invalid frame size calculated. Can't skip frame.\n");
        return FAILED;
    }

    if (fseek(f, frameSize, SEEK_CUR) != 0)
    {
        fprintf(stdout, "skipFrame: Error skipping frame.\n");
        return FAILED;
    }

    return SUCCESS;
}



//This calculates just like the name implies the size of a frame with given information from header so we can skip it
size_t _calculateFrameSize(struct mp3FrameHeader *header)
{
    if (header->samplerate == 0 || header->bitrate == 0)
    {
        printf("skipFrame: Invalid bitrate or samplerate\n");
        return 0;
    }

    size_t frameSize;
//MPEG1 Layer III frameSize = (144 * bitrate) / sampleRate + padding
    if (header->mpegVersion == 3 && header->layer == 1)
    {
        frameSize = (144000 * header->bitrate) / header->samplerate + header->padding;
    }

    else if ((header->mpegVersion == 2 || header->mpegVersion == 0) && header->layer == 1)
    {
        frameSize = (72000 * header->bitrate) / header->samplerate + header->padding;
    }
    else
    {
        printf("skipFrame: Only MPEG Version 2.5, 2 and 1 are supported with layer III");
        return 0;
    }
    return frameSize;
}