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
    
    /* Pointer to the array non-normalized spectrum values 
     * of each channel of the audio file
     * 
     * size [NFFT / 2 + 1] 
     * 
     * kiss_fft_cpx is a structure that contains:
     * r - the real part of the spectrum, 
     * i - the imaginary part of the spectrum */
    std::vector<std::unique_ptr<kiss_fft_cpx>> fft;
    
    /* A data vector that contains an array of audio file channels,
     * which contain pointer to the array of normalized to db FFT values */
    std::vector<std::unique_ptr<kiss_fft_scalar>> scaled;

    /* particular fft - vector storing the FFT values for every channel
     *
     * Non-normalized spectrum values of the audio file.
     * 
     * pair - FFT values of current moment of time.
     * 
     * pfft[i][j].second.get() -
     * means getting a pointer to an array of spectrum values 
     * for the i-th channel and j-th moment of time */
    std::vector<std::vector<std::pair<float, std::unique_ptr<kiss_fft_cpx>>>> pfft;

    /* particular fft - vector storing the FFT values for every channel
     *
     * Normalized to db spectrum values of the audio file.
     * 
     * pair - FFT values of current moment of time.
     * 
     * pscaled[i][j].second.get() -
     * means getting a pointer to an array of spectrum values 
     * for the i-th channel and j-th moment of time */
    std::vector<std::vector<std::pair<float, std::unique_ptr<kiss_fft_scalar>>>> pscaled;
    
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
     * in std::vector  */
    template<typename T>
    std::vector<T> _getV(const T* t);
    
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
    int getFileSampleRate();

    /* The original bit depth of the sample */
    int getFileFormat();
    
    /* Duration of the audio file in seconds */
    float getFileDuration();

    /* Number of frames per audio file channel */
    int getFileFramesPerChannel();

    /* Total count of frames of the audio file */
    int getFileFramesCount();

    /* Number of channels of the audio file */
    int getFileChannels();
    
    /* Frame values of each channel of the audio file
     * 
     * The size of getFileFramesCount() 
     * 
     * [i][j] - i-th channel, j-th frame*/
    std::vector<std::vector<float>> getFrames();
    
    /* Bit depth of the frame */
    int getFileBitDepth();
    
    bool isMono();
    
    /* Output a summary of the audio file metadata to the console */
    void printSummary();

    /* Values of the non-normalized spectrum of each channel of the audio file
     * 
     * Vector that contains audio file channels,
     * which contains a vector of the values of the non-normalized FFT
     *
     * size [NFFT / 2 + 1] 
     * 
     * kiss_fft_cpx is a structure that contains:
     * r - the real part of the spectrum, 
     * i - the imaginary part of the spectrum */
    std::vector<std::vector<kiss_fft_cpx>> getfftSpectrum();
    
    /* Vector that contains audio file channels,
     * which contains a vector of the values of the normalized FFT
     * 
     * size [NFFT / 2 + 1] */
    std::vector<std::vector<float>> getScaledfftSpectrum();

    /* Values of the non-normalized spectrum
     * for every time moment of every channel of an audio file
     *
     * A vector containing the channels of an audio file, 
     * which contains a vector containing a pair of elements 
     * of the time values of the non-normalized FFT
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
    std::vector<std::vector<std::pair<float, std::vector<kiss_fft_cpx>>>> getpfftSpectrum();
    

    /* Values of the normalized to db spectrum
     * for every time moment of every channel of an audio file
     *
     * A vector containing the channels of an audio file, 
     * which contains a vector containing a pair of elements 
     * of the time values of the normalized FFT
     *
     *
     * size [NFFT / 2 + 1] 
     * 
     *
     * std::pair<float - moment of time, 
     *          std::vector<float> - FFT values>
     *
     *
     * For example:
     * 
     * i - channel, j - the ordinal number of the moment in time (just an index)
     *
     * [i][j].first - value of the current time moment
     *
     * [i][j].second - vector of the FFT values for the current time moment */
    std::vector<std::vector<std::pair<float, std::vector<float>>>> getScaledpfftSpectrum();    
    
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
