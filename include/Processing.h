#pragma once

#include "kiss_fftr.h"
#include "AudioFile.h"
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>

#define EMPTY_CONTAINER "An empty container of the audio file spectrum, you did FFT or pFFT?\nYou may have called the wrong FFT Spectrum return method" 
#define BAD_ALLOCATE "Memory resources cannot be allocated"
#define BAD_NFFT "A number meaning size of the FFT window must be even and greater than 0"
#define BAD_TIMESCALE "The entered time scaling ratio should not be less than 1 or more than 1000"
#define BAD_CHANNEL "The requested channel does not match the available channels of the audio file" 

namespace spectrum {

/* A class representing the processing of an audio file:
 * 
 * data reading, FFT implementation, normalization of the received spectrum,
 * obtaining audio file metadata */
class Processing {

private:
    /* A structure containing FFT data */
    template <typename v, typename sV>
    struct Keepeth {
        /* The channel to which the conversion refers */
        int channel;
        /* The number of frequencies per spectral component */
        float freqPerBin;
        /* The time point for which the FFT was made */
        float time;
        /* Non-normalized FFT values for the current time moment 
         * that contain the kiss_fft_cpx structure:
         * r - the real part of the spectrum, 
         * i - the imaginary part of the spectrum */
        v values;
        /* Normalized FFT values for the current time moment */
        sV scaledValues;  
            
        Keepeth(int ch, float fpb, float t, v vls, sV sVls);
    };

public:
    /* A data type for public use, designed to simplify interaction 
     * and improve code readability. Serves as a storage 
     * for getpfftValues() and getpfftValues() values.
     *
     * Actually is std::vector, which contains a 
     * set of structures with the corresponding data fields 
     * of the FFT transformation:
     *
     * - int channel - the channel to which the conversion refers
     * 
     * - float freqPerBin - the number of frequencies per spectral component
     *
     * - float time - the time point for which the FFT was made
     *
     * - std::vector<kiss_fft_cpx> values - 
     * non-normalized FFT values for the current time moment 
     * that contain the kiss_fft_cpx structure:
     *
     * r - the real part of the spectrum, 
     * i - the imaginary part of the spectrum
     * 
     * The operator<< is overloaded for this structure
     * 
     * - std::vector<float> scaledValues - 
     * normalized FFT values for the current time moment */
    typedef std::vector<Keepeth<std::vector<kiss_fft_cpx>, 
                                std::vector<float>>> storage_t;
    
    Processing(int NFFT, const char* FILE);
    ~Processing();
    
    /* FFT window size */
    int getNFFT();
    
    /* The number of frequencies per spectral component
     * for a given FFT window size */
    float getFreqPerBin();

    /* Sampling rate of the audio file */
    int getSampleRate();
    
    /* Duration of the audio file in seconds */
    float getFileDuration();

    /* Number of frames per audio file channel */
    int getFramesPerChannel();

    /* Total count of frames of the audio file */
    int getTotalFrames();

    /* Number of channels of the audio file */
    int getChannels();
    
    /* Frame values of each channel of the audio file
     *  
     * [i][j] - i-th channel, j-th frame */
    std::vector<std::vector<float>> getFrames();
    
    /* Bit depth of the frame */
    int getBitDepth();
    
    bool isMono();
    
    /* Output summary data about an object 
     * and an audio file to the console */
    void printSummary();

    /* Values of the spectrum of each channel of the total audio file
     * 
     * Contains a std::vector of structures (see spectrum::Processing::storage_t) 
     * of the FFT values for full audio file, for each channel */
    storage_t getfftValues();
    
    /* Values of the spectrum for every time moment 
     * of every channel of an audio file
     *
     * Contains a std::vector of structures (see spectrum::Processing::storage_t), 
     * each of which is associated with a specific point in time 
     * for which the FFT was executed 
     *
     * The values for the channels are contained sequentially 
     * (one after the other) */
    storage_t getpfftValues();
    
    /* Spectrum values for each time point of the audio file channel
     *
     * Contains a std::vector of structures (see spectrum::Processing::storage_t), 
     * each of which is associated with a specific point in time 
     * for which the FFT was executed */
    storage_t getpfftValues(int channel);
       
    /* Performing FFT of the total audio file 
     * 
     * After successful execution of the method, allowed:
     * 
     * spectrum::Processing::getfftValues() */
    void FFT();
    
    /* Performing FFT audio file for each time point 
     *
     * timeScale is a parameter that determines the time scaling ratio. 
     * 
     * This is the value by which 1 second of the audio file is divided. 
     * For the received time value, the FFT will be performed.
     * 
     * For example: 
     * If timeScale = 10, then the FFT will be produced for every 0.1 second 
     * of the audio file, if timeScale = 1, then for every second
     *
     * After successful execution of the method, allowed:
     * 
     * spectrum::Processing::getpfftValues()
     * 
     * spectrum::Processing::getpfftValues(int channel) */
    void pFFT(int timeScale /* = 1 */);

private:
    typedef std::vector<Keepeth<std::unique_ptr<kiss_fft_cpx>, 
                                std::unique_ptr<kiss_fft_scalar>>> lstorage_t; 
     
    /* FFT window size */
    const int NFFT;

    /* Path to the audio file */
    const char* FILE;
    
    /* An object representing all the information
     * about the original audio file:
     *  - Metadata
     *  - Signal frames */
    AudioFile<float> file;
    
    /* Dynamic range
     * With a bit depth of 16 bits from 32767 to -32768 (65538) 
     * Is Equal to 96.33
     * We will use this value in the future to normalize the FFT values */
    float dynamicRange;
    
    /* A std::vector storing a structure that contains FFT data for a moment in time
     * pstorage[i].values.get() -
     * means getting a pointer to an array of spectrum values  */
    lstorage_t pstorage;
    
    /* A std::vector storing a structure that contains FFT data 
     * total audio file, by channels
     * storage[i].values.get() -
     * means getting a pointer to an array of spectrum values  */
    lstorage_t storage;

    /* Normalization of the resulting kiss_fft_cpx spectrum 
     * to the logarithmic scale
     * 
     * fft - pointer to an array of non-normalized spectrum
     * scaled - pointer to an empty array of normalized spectrum values 
     * Arrays size is NFFT / 2 + 1 */
    void scale(kiss_fft_cpx* fft, kiss_fft_scalar* scaled);
    
    /* Formula for normalization of spectrum values */
    float _scaleExpression(float r, float i);
    
    /* Copies an array of T* (not)normalized spectrum, signal frames 
     * to a std::vector  
     * S - array size */
    template<typename T>
    std::vector<T> _getVector(const T* t, const int S);
    
    /* Copies the lstorage_t containing pointers to the FFT data arrays 
     * to the storage_t in which the FFT data is stored as a std::vector, 
     * then returns the storage 
     * beg - begin index
     * end - end index (usually s.size()) */
    storage_t _peekValues(lstorage_t& s, const int beg, const int end);

    /* Terminate program with exitMessage */
    void _terminate(const char* exitMessage);
 
    };
}
std::ostream& operator<<(std::ostream& os, kiss_fft_cpx const& k);
