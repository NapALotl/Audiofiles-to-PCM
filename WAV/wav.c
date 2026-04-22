#include <stdio.h> //Needed for the "FILE" type and file handling like ftell, fseek or fread
#include <stdint.h> //uint used for buffers. uint8_t is 8 bits or 1 Byte which is perfect for wav
#include <string.h> //So we can easily compare our buffers to keywords like "WAVE" or "RIFF"

//Own models like fmtHeader
#include "models.h"



/*
We need a function that tells us if the given file is really a wav file.
Every WAV File starts with an wav header 

--- WAV Header ---
Bytes 0 - 3 must be "RIFF"
Bytes 4 - 7 are the Size of the whole File
Bytes 8 - 11 must be "WAVE"
*/
taskStatus validateWAVFile(FILE *f) 
{
    rewind(f);  //Makes sure we are at the start of the file
    uint8_t buffer[12]; //Every wav starts with a header of 12 Bytes

//Trying to read the whole header into our buffer so we can check if it is truly a wav file 
    if (fread(buffer, 1, 12, f) != 12) 
    {
        perror("validateWAVFile: Error reading file");
        return FAILED;
    }
    //buffer + 8 so we are at the pos of byte 9
    if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WAVE", 4) != 0)
    {
        fprintf(stdout, "validateWAVFile: File didn't start with valid wav header\n");
        return FAILED;
    }
    fprintf(stdout, "validateWAVFile: Successfully validated wav file\n");
    return SUCCESS;    
}



/*
In wav there are some chunks we need to find to play audio. 
To play audio we need to find "fmt " and "data"
Also wants a variable to save the size to
--- Chunk ---
4 Bytes of identifier
4 Bytes of size of data without identifier and size
DATA
*/
taskStatus findIDFieldData(FILE *f,uint32_t *size , const char* IDName)
{
//Char array ID Name must be 4 bytes long cause the wav fields are 4 bytes as well
    if (strlen(IDName) != 4)
    {
        fprintf(stdout, "findIdField: Given IDName must be 4 Bytes. WAV ID Field are always 4 Bytes long!\n");
        return FAILED;
    }

    rewind(f); //Sets file to the beginning
    if (fseek(f, 12, SEEK_CUR)) //Skip wav header
    {
        perror("findIDFieldData: Error while seeking in file.");
        return FAILED;

    } 

    uint8_t buffer[4]; //Buffer for reading field name
    uint8_t sizeBuffer[4]; //Buffer for reading size field


    do 
    {
    //Saving ID and Size into buffer
        if (fread(buffer, 1, 4, f) != 4 || fread(sizeBuffer, 1, 4, f) != 4)
        {
        //Checking for end of file.
            if (feof(f))
            {
                fprintf(stdout, "findIDField: Didn't find given id field '%s'. End of File.\n", IDName);
                return FAILED;
            }
        //Printing error because it couldn't read the requested 8 Bytess
            perror("findIDField: Error while reading file");
            return FAILED;
        }
        *size = sizeBuffer[0] | (sizeBuffer[1] << 8) | (sizeBuffer[2] << 16) | (sizeBuffer[3] << 24);

    //If ID Field doesn't match the field which was read we skip the data
        if (memcmp(buffer, IDName, 4) != 0)
        {
        //Since wav chunks start always on an even byte we need to add a padding byte if modulo 2 leaves doesn't end in 0. For example: size of data is 11 module(%) 2 = 1 -> padding byte needed
            
            uint32_t skip = *size;
            //Here we try modulo 2
            if (skip % 2 != 0) skip++;
            if (fseek(f, skip, SEEK_CUR))
            {
                perror("findIDFieldData: Error skipping in file.");
                return FAILED;
            }
        }

    } while (memcmp(buffer, IDName, 4) != 0);
    fprintf(stdout, "findIDField: Found requested ID Field '%s'. File is now at the beginning of data.\n");
    return SUCCESS;

}



/*
One of the most important things for playing pcm of wav is the data we get from the "fmt " field.
Here is data saved that we need so we know how to play the file for example samplerate, mono/stereo or bits per sample
How the fmt header is constructed and what each field does please look into 'models.h'
*/
taskStatus getFMTHeader(FILE *f, struct fmtHeader *header)
{
    char* IDName = "fmt ";
    uint8_t buffer[16]; //16 Bytes buffer because that is the length of default fmt header. If size of the data which we will get from findIDField is more than 16, we skip the rest since it is not needed for playback!
    uint32_t size;


//We need to find fmt field and save size
    if (findIDFieldData(f,&size ,IDName) == FAILED)
        return FAILED;

    if (size < 16)
    {
        fprintf(stdout, "fmtHeader: FMT field size of '%d' is too low. Header must be atleast 16 Bytes long.\n", size);
        return FAILED;
    }

//Saving default header into buffer
    if (fread(buffer, 1, 16, f) != 16)
    {
        perror("fmtHeader: Error reading file");
        return FAILED;
    }
    //If the size of fmt is higher than the default 16 we will skip it for now
    if (size > 16)
    {
        //Don't forget the padding byte :)
        int skip = size - 16;
        if (skip % 2 != 0) skip++;
        if (fseek(f, skip, SEEK_CUR))
        {
            perror("getFMTHeader: Error seeking in file.");
            return FAILED;
        }
    }

//Now it is time to put the read data in our struct.
    //Since wav is little Endian the values are saved in reverse so we have to shift the "outher" parts and add the values together with 'OR' -> |
    header->audioFormat = buffer[0] | buffer[1] << 8;
    header->numChannels = buffer[2] | buffer[3] << 8;
    header->sampleRate = buffer[4] | buffer[5] << 8 | buffer[6] << 16 | buffer[7] << 24;
    header->byteRate = buffer[8] | buffer[9] << 8 | buffer[10] << 16 | buffer[11] << 24;
    header->blockAlign = buffer[12] | buffer[13] << 8;
    header->bitsPerSample = buffer[14] | buffer[15] << 8;

    return SUCCESS;
    
}


//Now comes the part to read the samples. These 2 variants are functional examples how to get the samples
//You can use your own function, just make sure you find the data first and keep blockalign in mind so you don't cut of any audio

/*
This function is meant to be used after you used 'findIDField' to search for 'data'
It saves the data into a buffer which you then could use to give pipewire for playback as an example
Returns PCM frames and thinks of blockalign
*/
taskStatus getPCMFrame(FILE *f, struct fmtHeader *header, uint8_t* frameBuffer, int bufferSize)
{
    if (header->audioFormat != 1)
    {
        fprintf(stdout, "getPCMFrame: IEEE Float is not supported yet.\n");
        return FAILED;
    }
    uint16_t blockAlign = header->blockAlign;
    if (bufferSize < blockAlign)
    {
        fprintf(stdout, "getPCMFrame: Can't read samples since buffer is smaller than blockalign\n");
        return FAILED;
    }


    if (fread(frameBuffer, 1, blockAlign, f) != blockAlign)
    {
        if (feof(f))
        {
            fprintf(stdout, "getPCMFrame: Couldn't get sample. End of file was reached!\n");
            return FAILED;
        }
        else
        {
            perror("getPCMFrame: Couldn't get samples. Error reading File");
            return FAILED;
        }
    }
    return SUCCESS;
}





/*
This function is meant to be used after you used 'findIDField' to search for 'data'.
It will want to fill a buffer with samples
Feel free to create your own buffer that uses the same fields
*/
size_t getPCMFramesBuffer(FILE *f, struct fmtHeader *header ,struct pcmBuffer *buffer)
{

    if (header->audioFormat != 1)
    {
        fprintf(stdout, "getPCMFramesBuffer: IEEE Float is not supported yet.\n");
        return 0;
    }
    //Check if one whole frame even fits into buffer
    uint16_t blockAlign = header->blockAlign;
    if (buffer->capacity < blockAlign)
    {
        fprintf(stdout, "getPCMFramesBuffer: Can't get samples since buffer size is smaller than blockalign!\n");
        return 0;
    }
    //Trying to completly fill buffer and keeping block align in mind. Since the end result is an integer we dont have to worry about decimal points
    int frameCount = buffer->capacity / blockAlign;

    //Trying to fill the buffer with samples, 
    size_t framesRead = fread(buffer->buffer,
        blockAlign,
        frameCount,
        f
    );
    if (framesRead == 0)
    {
        if (feof(f))
        {
            buffer->eof = 1;
            return 0;
        }
        else
        {
            perror("getPCMFramesBuffer: Error reading samples");
            return 0;
        }
    }
    buffer->position = 0;
    buffer->eof = 0;

    return framesRead;

}

