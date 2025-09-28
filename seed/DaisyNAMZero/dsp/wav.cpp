//
//  wav.cpp
//  NeuralAmpModeler-macOS
//
//  Created by Steven Atkinson on 12/31/22.
//

#include <cstring> // memcmp
#include <cstdint>

#include "wav.h"

bool idIsNotJunk(unsigned char* id)
{
    return std::memcmp(id, "RIFF", 4) == 0 || std::memcmp(id, "WAVE", 4) == 0
           || std::memcmp(id, "fmt ", 4) == 0 || std::memcmp(id, "data", 4) == 0;
}

bool ReadChunkAndSkipJunk(const unsigned char*& inputData,
                          size_t&               dataSize,
                          unsigned char*                 chunkID)
{
    // Check if there’s enough data to read the chunk ID
    if(dataSize < 4)
        return false;

    // Copy the first 4 bytes to chunkID
    std::memcpy(chunkID, inputData, 4);

    // Continue reading and skipping junk until we find valid data
    while(!idIsNotJunk(chunkID) && dataSize >= 4)
    {
        // Check if there’s enough data to read the next chunk ID
        if(dataSize < 4)
            return false;

        // Update inputData pointer and remaining dataSize
        inputData += 4;
        dataSize -= 4;

        // Read the next chunk ID
        std::memcpy(chunkID, inputData, 4);
    }

    return dataSize >= 4;
}

dsp::wav::LoadReturnCode dsp::wav::Load(const unsigned char* data,
                                        size_t               dataSize,
                                        std::vector<float>&  audio,
                                        double&              sampleRate)
{
    unsigned char chunkId[4];
    if(!ReadChunkAndSkipJunk(data, dataSize, chunkId))
    {
        //std::cerr << "Error while reading for next chunk." << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
    }

    // Check RIFF chunk
    if(std::memcmp(data, "RIFF", 4) != 0)
    {
        return LoadReturnCode::ERROR_NOT_RIFF;
    }

    // Advance pointer by 4 + 4: RIFF + JUNK
    // Reduce dataSize by the same amount
    data += 4 + 4;
    dataSize -= 4 + 4;

    // Check WAVE chunk
    if(std::memcmp(data, "WAVE", 4) != 0)
    {
        return LoadReturnCode::ERROR_NOT_WAVE;
    }

    // Advance pointer by 4: WAVE
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    if(!ReadChunkAndSkipJunk(data, dataSize, chunkId))
    {
        //std::cerr << "Error while reading for next chunk." << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
    }

    // Check fmt chunk
    if(std::memcmp(data, "fmt ", 4) != 0)
    {
        return LoadReturnCode::ERROR_MISSING_FMT;
    }

    // Advance pointer by 4: fmt
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    int subchunk1Size;
    std::memcpy(&subchunk1Size, data, 4);
    if(subchunk1Size < 16)
    {
        // std::cerr << "WAV chunk 1 size is " << subchunk1Size
        //           << ", which is smaller than the requried 16 to fit the expected "
        //              "information."
        //           << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
    }

    // Advance pointer by 4: fmt
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    unsigned short audioFormat;
    std::memcpy(&audioFormat, data, 2);
    const short AUDIO_FORMAT_PCM  = 1;
    const short AUDIO_FORMAT_IEEE = 3;
    if(audioFormat != AUDIO_FORMAT_PCM && audioFormat != AUDIO_FORMAT_IEEE)
    {
        switch(audioFormat)
        {
            case 6:
                // std::cerr << "(Got: A-law)" << std::endl;
                return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_ALAW;
            case 7:
                // std::cerr << "(Got: mu-law)" << std::endl;
                return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_FORMAT_MULAW;
            case 65534:
                // std::cerr << "(Got: Extensible)" << std::endl;
                return dsp::wav::LoadReturnCode::
                    ERROR_UNSUPPORTED_FORMAT_EXTENSIBLE;
            default:
                // std::cerr << "(Got unknown format " << audioFormat << ")" << std::endl;
                return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
        }
    }

    // Advance pointer by 2 for audioFormat
    // Reduce dataSize by the same amount
    data += 2;
    dataSize -= 2;

    short numChannels;
    std::memcpy(&numChannels, data, 2);
    if(numChannels != 1)
    {
        //std::cerr << "Require mono (using for IR loading)" << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_NOT_MONO;
    }

    // Advance pointer by 2 for numChannels
    // Reduce dataSize by the same amount
    data += 2;
    dataSize -= 2;

    int iSampleRate;
    std::memcpy(&iSampleRate, data, 4);
    // Store in format we assume (SR is double)
    sampleRate = (double)iSampleRate;

    // Advance pointer by 4 for iSampleRate
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    int byteRate;
    std::memcpy(&byteRate, data, 4);

    // Advance pointer by 4 for byteRate
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    short blockAlign;
    std::memcpy(&blockAlign, data, 2);

    // Advance pointer by 2 for blockAlign
    // Reduce dataSize by the same amount
    data += 2;
    dataSize -= 2;

    short bitsPerSample;
    std::memcpy(&bitsPerSample, data, 2);

    // Advance pointer by 2 for bitsPerSample
    // Reduce dataSize by the same amount
    data += 2;
    dataSize -= 2;

    // The default is for there to be 16 bytes in the fmt chunk, but sometimes
    // it's different.
    if(subchunk1Size > 16)
    {
        return dsp::wav::LoadReturnCode::ERROR_OTHER;

        const int extraBytes = subchunk1Size - 16;
        const int skipChars  = extraBytes / 4 * 4; // truncate to dword size
        // memoryStream.ignore(skipChars);
        data += skipChars;
        dataSize -= skipChars;
        const int remainder = extraBytes % 4;
        // memoryStream.read(reinterpret_cast<char*>(&byteRate), remainder);
        std::memcpy(&byteRate, data, remainder);

        data += remainder;
        dataSize -= remainder;
    }

    // Read the data chunk
    unsigned char subchunk2Id[4];
    if(!ReadChunkAndSkipJunk(data, dataSize, subchunk2Id))
    {
        // std::cerr << "Error while reading for next chunk." << std::endl;
        return dsp::wav::LoadReturnCode::ERROR_INVALID_FILE;
    }
    // Check data chunk
    if(std::memcmp(data, "data", 4) != 0)
    {
        return LoadReturnCode::ERROR_INVALID_FILE;
    }

    // Advance pointer by 4 for data
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    // Size of the data chunk, in bits.
    int subchunk2Size;
    std::memcpy(&subchunk2Size, data, 4);

    // Advance pointer by 4 for subchunk2Size
    // Reduce dataSize by the same amount
    data += 4;
    dataSize -= 4;

    if(audioFormat == AUDIO_FORMAT_IEEE)
    {
        if(bitsPerSample == 32)
            dsp::wav::_LoadSamples32(data, subchunk2Size, audio);
        else
        {
            //std::cerr << "Error: Unsupported bits per sample for IEEE files: " << bitsPerSample << std::endl;
            return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_BITS_PER_SAMPLE;
        }
    }
    else if(audioFormat == AUDIO_FORMAT_PCM)
    {
        if(bitsPerSample == 16)
            dsp::wav::_LoadSamples16(data, subchunk2Size, audio);
        else if(bitsPerSample == 24)
            dsp::wav::_LoadSamples24(data, subchunk2Size, audio);
        else if(bitsPerSample == 32)
            dsp::wav::_LoadSamples32(data, subchunk2Size, audio);
        else
        {
            //std::cerr << "Error: Unsupported bits per sample for PCM files: " << bitsPerSample << std::endl;
            return dsp::wav::LoadReturnCode::ERROR_UNSUPPORTED_BITS_PER_SAMPLE;
        }
    }
    return dsp::wav::LoadReturnCode::SUCCESS;
}

void dsp::wav::_LoadSamples16(const unsigned char* inputData,
                              const int            chunkSize,
                              std::vector<float>&  samples)
{
    // Allocate an array to hold the samples
    std::vector<short> tmp(chunkSize / 2); // 16 bits (2 bytes) per sample

    // Read the samples from the file into the array
    // wavFile.read(reinterpret_cast<char*>(tmp.data()), chunkSize);
    std::memcpy(tmp.data(), inputData, chunkSize);

    // Copy into the return array
    const float scale = 1.0 / ((double)(1 << 15));
    samples.resize(tmp.size());
    for(auto i = 0; i < samples.size(); i++)
        samples[i] = scale * ((float)tmp[i]); // 2^16
}

void dsp::wav::_LoadSamples24(const unsigned char* inputData,
                              const int            chunkSize,
                              std::vector<float>&  samples)
{
    // Allocate an array to hold the samples
    std::vector<int> tmp( chunkSize / 3 ); // 24 bits (3 bytes) per sample
    // Read in and convert the samples
    for(int& x : tmp)
    {
        x = dsp::wav::_ReadSigned24BitInt(inputData);
        inputData += 3;
    }

    // Copy into the return array
    const float scale = 1.0 / ((double)(1 << 23));
    samples.resize(tmp.size());
    for(auto i = 0; i < samples.size(); i++)
        samples[i] = scale * ((float)tmp[i]);
}

void dsp::wav::_LoadSamples32(const unsigned char* inputData,
                              const int            chunkSize,
                              std::vector<float>&  samples)
{
    // NOTE: 32-bit is float.
    samples.resize(chunkSize / 4); // 32 bits (4 bytes) per sample
    // Read the samples from the file into the array
    std::memcpy(samples.data(), inputData, chunkSize);
}

int dsp::wav::_ReadSigned24BitInt(const unsigned char* inputData)
{
    // Read the three bytes of the 24-bit integer.
    std::uint8_t bytes[3];
    std::memcpy(bytes, inputData, 3);

    // Combine the three bytes into a single integer using bit shifting and
    // masking. This works by isolating each byte using a bit mask (0xff) and then
    // shifting the byte to the correct position in the final integer.
    int value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16);

    // The value is stored in two's complement format, so if the most significant
    // bit (the 24th bit) is set, then the value is negative. In this case, we
    // need to extend the sign bit to get the correct negative value.
    if(value & (1 << 23))
    {
        value |= ~((1 << 24) - 1);
    }

    return value;
}
