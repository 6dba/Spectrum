![Spectrum](https://i.imgur.com/LAUeqpH.jpg)
# Spectrum

**Open-Source C++ library for Fast Fourier Transform (FFT) of WAV/AIFF audio files.**

![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/6dba/Spectrum?color=purple&style=for-the-badge)
![GitHub commits since latest release (by date) for a branch](https://img.shields.io/github/commits-since/6dba/Spectrum/latest/develop?style=for-the-badge)
![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/6dba/Spectrum?color=green&style=for-the-badge)

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
    /* Performing FFT audio file for each time point 
     *
     * timeScale is a parameter that determines the time scaling ratio. 
     * This is the value by which 1 second of the audio file is divided. 
     * For the received time value, the FFT will be performed.
     * 
     * For example: 
     * If timeScale = 10, then the FFT will be produced for every 0.1 second 
     * of the audio file, if timeScale = 1, then for every second
     *
     * After successful execution of the method, allowed:
     * spectrum::Processing::getpfftValues()
     * spectrum::Processing::getpfftValues(int channel) */
	fftr.pFFT(int timeScale);
	
	/* Performing FFT of the total audio file
     *
     * After successful execution of the method, allowed:
     * spectrum::Processing::getfftValues() */
	fftr.FFT();

### Getting conversion results
	/** If the FFT of the total audio file is used (FFT()) **/
	
    /* Values of the spectrum of each channel of the total audio file
     * 
     * Contains a std::vector of structures (see "Storing the received values" below) 
     * of the FFT values for full audio file, for each channel */
    fftr.getfftValues();

	/* */
	
	/** If the partial FFT of every second of an audio file is used (pFFT()) **/
    
    /* Values of the spectrum for every time moment 
     * of every channel of an audio file
     *
     * Contains a vector of structures (see "Storing the received values" below), 
     * each of which is associated with a specific point in time 
     * for which the FFT was executed 
     *
     * The values for the channels are contained sequentially 
     * (one after the other) */
    fftr.getpfftValues();
    
    /* Spectrum values for each time point of the audio file channel
     *
     * Contains a vector of structures (see "Storing the received values" below), 
     * each of which is associated with a specific point in time 
     * for which the FFT was executed */
    fftr.getpfftValues(int channel);

### Storing the received values
    /* A data type for public use, designed to simplify interaction 
     * and improve code readability. Serves as a storage 
     * for getpfftValues() and getpfftValues() values.
     *
     * Actually is std::vector, which contains a 
     * set of structures with the corresponding data fields 
     * of the FFT transformation:
     *
     * - int channel - the channel to which the conversion refers

     * - float freqPerBin - the number of frequencies per spectral component
     *
     * - float time - the time point for which the FFT was made
     *
     * - std::vector<kiss_fft_cpx> values - 
     * non-normalized FFT values for the current time moment 
     * that contain the kiss_fft_cpx structure:
     *
     * 	r - the real part of the spectrum, 
     * 	i - the imaginary part of the spectrum
     * The operator<< is overloaded for this structure
     * 
     * - std::vector<float> scaledValues - 
     * normalized FFT values for the current time moment */
 
    typedef std::vector<Keepeth<std::vector<kiss_fft_cpx>, 
                                std::vector<float>>> storage_t;
 
    /* Or you just may use "auto" :) */

### Other methods
    /* FFT window size */
    fftr.getNFFT();
    
    /* The number of frequencies per spectral component
     * for a given FFT window size */
    fftr.getFreqPerBin();

    /* Sampling rate of the audio file */
    fftr.getSampleRate();
    
    /* Duration of the audio file in seconds */
    fftr.getFileDuration();

    /* Number of frames per audio file channel */
    fftr.getFramesPerChannel();

    /* Total count of frames of the audio file */
    fftr.getTotalFrames();

    /* Number of channels of the audio file */
    fftr.getChannels();
    
    /* Returns a vector of vectors with a frame values 
     * of each channel of the audio file
     * [i][j] - i-th channel, j-th frame */
    fftr.getFrames();
    
    /* Bit depth of the frame */
    fftr.getBitDepth();
    
    fftr.isMono();
    
    /* Output summary data about an object 
     * and an audio file to the console */
    fftr.printSummary();

# Example
	#include "Spectrum.h"
		
	int main(int argc, char* argv[]) {
    
		/* init */
		spectrum::Processing p(1024, "res/sine_1000Hz.wav");     
		/*  Doing a partial FFT, for every 1 second of audio file */
		p.pFFT(1);
		/* Getting the processing result of all channels */
    	spectrum::Processing::storage_t v = p.getpfftValues();
    	/* or you may use "auto" */
 
		/* Export to .csv file */ 
		std::ofstream fout; fout.open("sine_1000Hz.csv", std::ios::trunc);

		fout << "channel;time;frequency;value" << std::endl;
		for (int i = 0; i < v.size(); i++) {
	        /* Iteration over a vector of scaled values in the v object */
	        for (int j = 0; j < v[i].scaledValues.size(); j++) {
	            /*        channel         A moment in time              */
	            fout << v[i].channel <<';'<< v[i].time <<';'
	                 << j * v[i].freqPerBin <<';'<< v[i].scaledValues[j] 
	            /*         frequency (Hz)               FFT value       */
	                 << std::endl; 
		    }
		}
		fout.close();
		return 0;
	};
### sine_1000Hz.csv
![Data](https://i.imgur.com/FxIeh9H.png)
### Plot
![Plot](https://i.imgur.com/OHcg7jT.png)
# Attention
**⚠️ Undefined behavior or a long processing execution is possible with large values of the FFT window size, 
long audio files and a high *timeScale* ratio for *pFFT()*.** 

**If you have found a problem or have any suggestions, please describe it in [Issues](https://github.com/6dba/Spectrum/issues). Problems and comments will be solved as far as possible, please treat with understanding :)**

# Future functionality
* Implementation of export to .csv

# Dependencies used
* [AudioFile](https://github.com/adamstark/AudioFile)
* [KISS FFT](https://github.com/mborgerding/kissfft)