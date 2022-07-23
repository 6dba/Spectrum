#pragma once

#include "kiss_fftr.h"
#include "AudioFile.h"
#include <iostream>
#include <memory>
#include <ostream>
#include <cmath>
#include <vector>
#include <utility>

#define EMPTY_CONTAINER "An empty container of the audio file spectrum, you did FFT or pFFT?\nYou may have called the wrong FFT Spectrum return method" 
#define BAD_DURATION "The requested value exceeds the duration of the audio file"
#define BAD_ALLOCATE "Memory resources cannot be allocated"
#define BAD_NFFT "The size of the FFT window must be greater than 0"
#define BAD_TIMESCALE "The entered time scaling ratio should not be less than 1 or more than 1000"


namespace spectrum {

/* A class representing the processing of an audio file:
 * 
 * data reading, FFT implementation, normalization of the received spectrum,
 * btaining audio file metadata */
class Processing {

    /* FFT window size */
    const int NFFT;

    /* Path to the audio file */
    const char* FILE;
    
    /* An object representing all the information
     * about the original audio file:
     * Metadata
     * Signal frames */
    AudioFile<float> file;
    
    /* Dynamic range
     *
     * With a bit depth of 16 bits from 32767 to -32768 (65538)
     * 
     * Is Equal to 96.33
     * 
     * We will use this value in the future to normalize the FFT values */
    float DYNAMICRANGE;
     
    /* A structure containing FFT data */
    template <typename v, typename sV>
    struct Keepeth {
        friend class Processing;
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
        private: 
            Keepeth(int ch, float fpb, float t, v vls, sV sVls);
    };
    
    typedef std::vector<Keepeth<std::vector<kiss_fft_cpx>, std::vector<float>>> rtstorage_t;
    typedef std::vector<Keepeth<std::unique_ptr<kiss_fft_cpx>, std::unique_ptr<kiss_fft_scalar>>> storage_t;

    /* A vector storing a structure that contains FFT data for a moment in time
     *
     * pstorage[i].values.get() -
     * means getting a pointer to an array of spectrum values  */
    storage_t pstorage;
    
    /* A vector storing a structure that contains FFT data 
     * total audio file, by channels
     *
     * storage[i].values.get() -
     * means getting a pointer to an array of spectrum values  */
    storage_t storage;

    /* Normalization of the obtained kiss_fft spectrum to the logarithmic scale
     * by full NFFT counts
     *
     * fft - pointer to an array of non-normalized spectrum
     * 
     * scaled - pointer to an array of normalized spectrum values */
    void scale(kiss_fft_cpx* fft, kiss_fft_scalar* scaled);
    
    /* Formula for normalization of spectrum values */
    float _scaleExpression(float r, float i);
    
    /* Converts arrays of T* (not)normalized spectrum, signal frames 
     * in a vector  */
    template<typename T>
    std::vector<T> _getVector(const T* t);
    
    /* Copies a value from a dynamic array to a vector 
     * in order to return it */
    rtstorage_t _peekValues(storage_t& s);

    /* Terminate program with exitMessage */
    void _terminate(const char* exitMessage);
    
public:
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
     * The size of getTotalFrames() 
     * 
     * [i][j] - i-th channel, j-th frame*/
    std::vector<std::vector<float>> getFrames();
    
    /* Bit depth of the frame */
    int getBitDepth();
    
    bool isMono();
    
    /* Output a summary of the audio file metadata to the console */
    void printSummary();

    /* Values of the spectrum of each channel of the total audio file
     * 
     * Vector that contains audio file channels,
     * which contains a struct objects of the FFT values of the total audio file
     *
     * size [NFFT / 2 + 1] 
     * 
     * kiss_fft_cpx is a structure that contains:
     * r - the real part of the spectrum, 
     * i - the imaginary part of the spectrum */
    rtstorage_t getfftValues();
    
    /* Values of the spectrum for every time moment 
     * of every channel of an audio file
     *
     * A vector of objects of a structure that contains time values of the FFT 
     *
     *
     * size [NFFT / 2 + 1] 
     * 
     *
     * std::pair<float - moment of time, 
     *          std::vector<kiss_fft_cpx> - FFT values>
     *
     *
     * For example:
     * 
     * i - channel, j - the ordinal number of the moment in time (just an index)
     *
     * [i][j].first - value of the current time moment
     *
     * [i][j].second - vector of the FFT values for the current time moment
     *
     * kiss_fft_cpx is a structure that contains:
     * r - the real part of the spectrum, 
     * i - the imaginary part of the spectrum */
    rtstorage_t getpfftValues();
    
    rtstorage_t getpfftValues(int channel);
       
    /* Performing an entire FFT audio file 
     * 
     * After successful execution of the method, allowed:
     *
     * spectrum::Processing::getfftSpectrum() - 
     *  std::vector<std::vector<kiss_fft_cpx>> values of the non-normalized 
     *  spectrum of each channel of the audio file
     *
     * spectrum::Processing::getScaledfftSpectrum() - 
     *  std::vector<std::vector<float>> values of the normalized to db 
     *  spectrum of each channel of the audio file */
    void FFT();
    
    /* Performing FFT audio file for every i-th second 
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
     * spectrum::Processing::getpfftSpectrum() - 
     *  std::vector<std::vector<std::pair<float, std::vector<kiss_fft_cpx>>>> 
     *  values of the non-normalized spectrum every time moment
     *  of every channel of an audio file 
     *
     * spectrum::Processing::getScaledpfftSpectrum() - 
     *  std::vector<std::vector<std::pair<float, std::vector<float>>>> 
     *  values of the normalized to db spectrum every time moment
     *  of every channel of an audio file */
    void pFFT(int timeScale /* = 1 */);
    
    };
}
std::ostream& operator<<(std::ostream& os, kiss_fft_cpx const& k);
