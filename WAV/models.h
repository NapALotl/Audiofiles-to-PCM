#ifndef MODELS_H
#define MODELS_H

//For better clarity i like to use an enum to return the status of the task
    //This is completly optional. Not necessary for getting to PCM
typedef enum taskStatus 
{
    SUCCESS = 0,
    FAILED = 1
} taskStatus;



//FMT Header struct to save data on how we play the file
    //Default FMT Header is 16 Bytes, there are optional fields. We will just skip those
struct fmtHeader
{
    uint16_t audioFormat; //2 Bytes of the format. 1 = PCM, 3 = Float PCM
    uint16_t numChannels; //2 Bytes of channels. 
    uint32_t sampleRate; //4 Bytes of samplerate. How many samples per second will be played
    uint32_t byteRate; //4 Bytes of byterate.
    uint16_t blockAlign; //2 Bytes of blockalign. When playing we must not cut off blockalign. Is the product of bytes per sample (bits per sample / 8) * numChannels. Combination of samples to satisfy block align is reffered to as frame
    uint16_t bitsPerSample; //2 Bytes of bits per sample. Tells us how many bits we need to read for on sample.
};

//Default buffer to save samples/Frames to
struct pcmBuffer
{
    uint8_t *buffer; //Stores the actual frames
    int capacity; //How many bytes the buffer can hold
    int position; //Marks at what point the buffer is. Maybe pipewire buffer is already full and you can't transfer from this buffer to pipewire so you need to remember where you are.
    int eof; //Could be used to toggle and tell the audio api that there won't be more files since the file has ended
};


#endif