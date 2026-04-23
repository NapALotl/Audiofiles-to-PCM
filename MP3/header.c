#include <stdint.h>
#include <string.h>

#include "models.h" //Here are the tables saved for example for bitrate or samplerate
#include "mp3.h" //So we have access to the public mp3 header
#include "bitReader.h" //Since mp3 header values dont stick to byte boundaries it is recommend to use a bit reader for parsing the header. This is a simple implementation of a bit reader which you can use for parsing the mp3 header. You can also implement your own bit reader if you want, but this one should be sufficient for our purposes.
#include "tables.h" //Here are the tables for bitrate and samplerate which we will apply to the header after reading the header. This is a simple implementation of the tables, you can also implement your own if you want, but this one should be sufficient for our purposes.

/*We are getting a buffer of 10 bytes which is the size of the ID3v2 header. We will check if the first 3 bytes are "ID3" and if so we will assume it is an ID3v2 header and skip it. If not we will assume it is a valid MP3 file and return SUCCESS. This is not the best way to validate an MP3 file, but it is a simple way to check if there is an ID3v2 header and if not we will assume it is a valid MP3 file. A better way would be to check for the presence of MP3 frame headers, but that would require more complex parsing and is not necessary for our purposes.
--- ID3v2 header ---
Byte 0 - 2 == ID3
Byte 3 == Major version
Byte 4 == Revision
Byte 5 == Flags
    bit 0 - 3 must be 0
    bit 4 Footer
    bit 5 Experimental
    bit 6 Extended Header
    bit 7 Unsync
Byte 6-9 syncsafe
*/
ID3v2HeaderStatus _testID3v2Header(uint8_t *buffer, struct mp3ID3v2Header *header)
{
    if (memcmp(buffer, "ID3", 3) != 0)
    {
        fprintf(stdout, "_testID3v2Header: No ID3v2 header found. Trying to test for Frames immediately.\n");
        return NoID3v2Header;
    }
    //Now we check if the version is valid. Accepted Version are 3 and 4.
    if (buffer[3] < 3 || buffer[3] > 4)
    {
        fprintf(stdout, "_testID3v2Header: Invalid ID3v2 header found. Version %d is not supported.\n", buffer[3]);
        return ID3v2HeaderBROKEN;
    }

    //Now we check for Flags, bit 0 - 3 must be 0
    if ((buffer[5] & 0x0F) != 0)
    {
        fprintf(stdout, "_testID3v2Header: Invalid ID3v2 header found. Flags bit 0 - 3 must be 0 but are %d.\n", buffer[5] & 0x0F);
        return ID3v2HeaderBROKEN;
    }

    //For the last check, we will check syncsafe. The hightst bit of each byte of syncsafe must be 0. So we will check if the hightst bit of each byte is 0 and if not we will assume it is a broken header. This is not the best way to validate an ID3v2 header, but it is a simple way to check if the syncsafe is valid. A better way would be to check for the presence of frames and if they are valid, but that would require more complex parsing and is not necessary for our purposes.
    for (int i = 6; i < 10; i++)
    {
        if ((buffer[i] & 0x80) != 0) //we target the last bit with 0x80 since it is the hightst bit of a byte and check if it is 0
        {
            fprintf(stdout, "_testID3v2Header: Invalid ID3v2 header found. Syncsafe byte %d has the hightst bit set.\n", i - 6);
            return ID3v2HeaderBROKEN;
        }
    }

    fprintf(stdout, "_testID3v2Header: Valid ID3v2 header found. Version %d.\n", buffer[3]);
//Applying values
    header->majorVersion = buffer[3];
    header->revision = buffer[4];
    header->flags = buffer[5];
    header->size = (buffer[6] << 21) | (buffer[7] << 14) | (buffer[8] << 7) | buffer[9];
    
    return ID3v2HeaderValid; 

}






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

//This functions thinks it is at the start of a MP3 frame header and saves the data into struct. Returns FAILED if the header is not valid and SUCCESS if it is valid.
    //You can do this check with bitReader but since values dont stick to Byte boundaries it is recommend to use a bit reader
taskStatus _getMP3Header(FILE *f, struct mp3FrameHeader *header)
{
    uint8_t buffer[4]; //MP3 Frame header is 4 bytes long, reading
    struct bitReader reader;
    reader.data = buffer;
    reader.bufferSize = 4;
    reader.bytePosition = 0;
    reader.bitPosition = 0;

    if (fread(buffer, 1, 4, f) != 4)
    {
        perror("_getMP3Header: Error reading file. Not rewinding...");
        return FAILED;
    }

//Every Frame MUST start with a syncword which is 1111 1111 111. If not it is immediatly invalid.
    //No need to save it
    if (readBits(&reader, 11) != 0x7FF) //0x7FF is 2047 in decimal and is the value of 11 bits set to 1
    {
        fprintf(stdout, "_getMP3Header: Invalid MP3 Frame header. Syncword not found.\n");
        return FAILED;
    }

//Mpeg version check and read
    //Now very important we read the mpeg version. value = 1 is not allowed. 3 = MPEG Version 1, 2 = MPEG Version 2, 0 = MPEG Version 2.5
    int mpegVersion = readBits(&reader, 2);
    if (mpegVersion == 1) //Value 1 is reserved and should not be used
    {
        fprintf(stdout, "_getMP3Header: Invalid MPEG Version. Version value 1 is reserved and should not be used.\n");
        return FAILED;
     }

//Layer Check and read
    int layer = readBits(&reader, 2);
    if (layer != 1) //Only Layer III is supported for now. Layer III Value == 1
    {
        fprintf(stdout, "_getMP3Header: Unsupported Layer. Only Layer III is supported for now.\n");
        return FAILED;
    }

//If protection Absend is set need to check it before reading side info and skip 2 Bytes if it is 1
    int protectionAbsent = readBits(&reader, 1);


    int bitrateIndex = readBits(&reader, 4);
    if (bitrateIndex == 0 || bitrateIndex == 0xF) //Value 0 and 0xF (15 in decimal) are not allowed for bitrate index
    {
        fprintf(stdout, "_getMP3Header: Invalid bitrate index. Value 0 and 15 are not allowed.\n");
        return FAILED;
    }


    int samplerateIndex = readBits(&reader, 2);
    if (samplerateIndex == 0x3) //Value 0x3 (3 in decimal) is not allowed for samplerate index
    {
        fprintf(stdout, "_getMP3Header: Invalid samplerate index. Value 3 is not allowed.\n");
        return FAILED;
    }

    int padding = readBits(&reader, 1);
    readBits(&reader, 1); //We dont need the private bit for now so we just read it and do nothing with it

    int channels = readBits(&reader, 2);
    int modeExtesnion = readBits(&reader, 2);
    //The rest of the header is not needed for now.

    //Now applying every read value to header structur
    header->protectionAbsent = protectionAbsent;
    header->layer = layer;
    header->mpegVersion = mpegVersion;
    header->bitrateIndex = bitrateIndex;
    header->samplerateIndex = samplerateIndex;
    header->padding = padding;
    header->modeChannels = channels;
    //We save the number of channels so it is easier later to determine if something is stereo oder mono
    header->numChannels = (channels == 3) ? 1 : 2; //If channel mode is 11 (3 in decimal) it is single channel (mono) else it is stereo
    header->modeExtension = modeExtesnion;
    header->bitrate = 0; //We will apply the real bitrate later with the help of the bitrate index and mpeg version
    header->samplerate = 0; //We will apply the real samplerate later with the help of the samplerate index and mpeg version
    
    if (_applyTablesToHeader(header) == FAILED)
    {
        fprintf(stdout, "_getMP3Header: Error applying tables to header. This should not happen since we already checked for valid values of bitrate and samplerate index, but something went wrong.\n");
        return FAILED;
    }
    return SUCCESS;
}




//Applys real values to header from the read index of Headers
    //Only applys to layer 3 for now.
taskStatus _applyTablesToHeader(struct mp3FrameHeader *header)
{
    //Only Layer III is supported for now since it is the most common layer and the only one we will cover in this project.
    if (header->layer != 1)
    {
        printf("appyTabesToHeader: Unsupported Layer. Only Layer III is supported for now.");
        return FAILED;
    }

    //sampleRate Tables
        //Different MPEG Version use different tables for sample rate. You can find them in models.h
    if (header->mpegVersion == 3 && header->layer == 1)
    {
        header->samplerate = samplerateTableMPEG1[header->samplerateIndex];

    }
    else if (header->mpegVersion == 2 && header->layer == 1)
    {
        header->samplerate = samplerateTableMPEG2[header->samplerateIndex];
    }
    else if (header-> mpegVersion == 0 && header->layer == 1)
    {
        header-> samplerate = samplerateTableMPEG25[header-> samplerateIndex];
    }
    else
    {
        printf("appyTabesToHeader: Unsupported MPEG Version or Layer");
        return FAILED;
    }

    //Bitrate Table
        //Different MPEG Version and Layer use different tables for bitrate. You can find them in models.h
    if ((header->mpegVersion == 2 || header->mpegVersion == 0) && header->layer == 1)
    {
        header->bitrate = bitrateTableMPEG2L3[header->bitrateIndex];
    }
    else if (header->mpegVersion == 3 && header->layer == 1)
    {
        header->bitrate = bitrateTableMPEG1L3[header->bitrateIndex];
    }
    else 
    {
        printf("appyTabesToHeader: Unsupported MPEG Version or Layer");
        return FAILED;
    }

    return SUCCESS;

}