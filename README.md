# Spectrum

**Open-Source C++ library for Fast Fourier Transform (FFT) of WAV/AIFF audio files.**

Current supported formats:

* WAV
* AIFF

Current supported channels:

* Stereo
* Mono

# Usage
### CMakelists.txt example:

	CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

	PROJECT(example)

	SET(CMAKE_CXX_STANDARD 14)

	ADD_SUBDIRECTORY(libs/Spectrum) # path to library folder

	ADD_EXECUTABLE(program src/main.cpp)
	TARGET_LINK_LIBRARIES(program Spectrum) 

### Creating an object
	#include "Spectrum.h"
	/*...*/
    const char* filePath = "res/sine_1000Hz.wav";
    const int NFFT = 1024;

    spectrum::Processing fftr(NFFT, filePath);

### Fourier Transform
	/* For partial FFT of every time moment of an audio file */	
	fftr.pFFT(int timeScale);
    /* timeScale is a parameter that determines the time scaling ratio. 
     * 
     * This is the value by which 1 second of the audio file is divided. 
     * For the received time value, the FFT will be performed.
     * 
     * For example: 
     * If timeScale = 10, then the FFT will be produced for every 0.1 second 
     * of the audio file, if timeScale = 1, then for every second
	
	/* For an entire FFT audio file */
	fftr.FFT();

### Getting conversion results
	/* For initialization conatiner of vales you may use "auto" */
	/* kiss_fft_cpx is a structure that contains:
         * r - the real part of the spectrum, 
         * i - the imaginary part of the spectrum */
	/* Size of FFT values is [NFFT / 2 + 1] */
	
	/** If the FFT of the entire audio file is used (FFT()) **/
	
	/* Values of the non-normalized spectrum of each channel of the audio file
     * 
     * std::vector<std::vector<kiss_fft_cpx>> 
     * Vector that contains audio file channels, which contains a vector 
     * of the values of the non-normalized FFT */
	fftr.getfftSpectrum();
	
	/* Values of the normalized 
     * spectrum of each channel of the audio file 
     * 
     * std::vector<std::vector<float>> 
     * Vector that contains audio file channels, which contains a vector 
     * of the values of the normalized FFT */
	fftr.getScaledfftSpectrum();

	/* */
	
	/** If the partial FFT of every second of an audio file is used (pFFT()) **/
    
    /* Values of the non-normalized spectrum
     * for every time moment of every channel of an audio file
     *
     * A vector containing the channels of an audio file, 
     * which contains a vector containing a pair of elements 
     * of the time values of the non-normalized FFT
     * 
     * std::vector<std::vector<std::pair<float, std::vector<kiss_fft_cpx>>>>
     *
     * std::pair<float - moment of time, 
     *          std::vector<kiss_fft_cpx> - FFT values>
     *
     * For example:
     * 
     * i - channel, j - the ordinal number of the moment in time (just an index)
     * [i][j].first - value of the current time moment
     * [i][j].second - vector of FFT values for the current time moment */
	fftr.getpfftSpectrum();
	
    /* Values of the normalized to db spectrum
     * for every time moment of every channel of an audio file
     *
     * A vector containing the channels of an audio file, 
     * which contains a vector containing a pair of elements 
     * of the time values of the normalized FFT
     *
     * std::pair<float - moment of time, 
     *          std::vector<float> - FFT values>
     *
     * For example:
     * 
     * i - channel, j - the ordinal number of the moment in time (just an index)
     * [i][j].first - value of the current time moment
     * [i][j].second - vector of the FFT values for the current time moment */
	fftr.getScaledpfftSpectrum();
	
### Other useful methods
    /* FFT window size */
    fftr.getNFFT();
    
    /* The number of frequencies per spectral component
     * for a given FFT window size */
    fftr.getFreqPerBin();

    /* Sampling rate of the audio file */
    fftr.getFileSampleRate();

    /* The original bit depth of the sample */
    fftr.getFileFormat();
    
    /* Duration of the audio file in seconds */
    fftr.getFileDuration();

    /* Number of frames per audio file channel */
    fftr.getFileFramesPerChannel();

    /* Total count of frames of the audio file */
    fftr.getFileFramesCount();

    /* Number of channels of the audio file */
    fftr.getFileChannels();
    
    /* Frame values of each channel of the audio file
     * 
     * The size of getFileFramesCount() 
     * 
     * [i][j] - i-th channel, j-th frame*/
    fftr.getFrames();
    
    /* Bit depth of the frame */
    fftr.getFileBitDepth();
    
    fftr.isMono();
    
    /* Output a summary of the audio file metadata to the console */
    fftr.printSummary();

# Example
	#include "Spectrum.h"
	
	int main(int argc, char* argv[]) {
    
	    /** Init **/
	    spectrum::Processing p(512, "res/sine_1000Hz.wav");     
	    /*  Doing a partial FFT, for every 1 second of audio file */
	    p.pFFT(1); // 
	    /* Getting the processing result on channel 0 (mono) */
	    auto v = p.getScaledpfftSpectrum()[0];
	    
	    /** Export to .csv file **/ 
	    std::ofstream fout; fout.open("sine_1000Hz.csv", std::ios::trunc);
	
	    fout << "channel;second;frequency;value" << std::endl;
	    /* Iteration on a pair values in the channel */
	    for (int i = 0; i < v.size(); i++) {
	        /* Iteration over a vector of values in a pair */
	        for (int j = 0; j < v[i].second.size(); j++) {
	            /* data usage */
	            /*   channel        second      */
	            fout << i <<';'<< v[i].first <<';'
	            << j * p.getFreqPerBin() <<';'<< v[i].second[j] << std::endl;
	            /*    frequency (Hz)               FFT value               */
	        }
	    }
	    fout.close();
	    return 0;
	};
### sine_1000Hz.csv
![Data](https://i.imgur.com/VICcWDE.png)
### Plot
![Plot](https://i.imgur.com/QkfWuN0.png)
# Attention
**⚠️ Undefined behavior or a long processing execution is possible with large values of the FFT window size, 
long audio files and a high *timeScale* ratio for *pFFT()*.** 

**If you have found a problem or have any suggestions, please describe it in [Issues](https://github.com/6dba/Spectrum/issues). Problems and comments will be solved as far as possible, please treat with understanding :)**

# Future functionality
* Implementation of export to .csv
* Interface for more convenient access to the resulting data

# Dependencies used
* [AudioFile](https://github.com/adamstark/AudioFile)
* [KISS FFT](https://github.com/mborgerding/kissfft)
