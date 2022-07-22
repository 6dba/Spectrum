#include "Processing.h"

spectrum::Processing::Processing(int NFFT, const char* AUDIOFILE) 
    : NFFT(NFFT), 
    FILE(AUDIOFILE) 
{
    if (NFFT <= 0)
        this->_terminate(BAD_NFFT);

    /* Real input signal - all imaginary parts are zero
     * Putting the ith sample in frames[i]
     * Reading data from an audio file
     * file.samples - contains a vector of vectors,
     * which contains the frames of each channel */
    this->file.load(this->FILE);
    
    /* Dynamic range
     * With a bit depth of 16 bits from 32767 to -32768 (65536)
     * Is equal to 96.33Db
     * We will use this value in the future to normalize the FFT values */
    this->DYNAMICRANGE = std::abs(20.0f * log10f(1.0f / (float)std::pow(2, this->getFileBitDepth())));
};

spectrum::Processing::~Processing() {};

int spectrum::Processing::getNFFT() {
    return this->NFFT;
};

float spectrum::Processing::getFreqPerBin() {
    return ((float)this->getFileSampleRate() / (float)this->NFFT);
};

int spectrum::Processing::getFileSampleRate() {
    return this->file.getSampleRate();
};

float spectrum::Processing::getFileDuration() {
    return this->file.getLengthInSeconds();
};

int spectrum::Processing::getFileFramesPerChannel() {
    return this->file.getNumSamplesPerChannel();
};

int spectrum::Processing::getFileFramesCount() {
    return this->getFileFramesPerChannel() * this->getFileChannels();
};

int spectrum::Processing::getFileChannels() {
    return this->file.getNumChannels();
};

std::vector<std::vector<float>> spectrum::Processing::getFrames() {
    return this->file.samples;
};

int spectrum::Processing::getFileBitDepth() {
    return this->file.getBitDepth();
};

bool spectrum::Processing::isMono() {
    return this->file.isMono();
};

void spectrum::Processing::printSummary() {
    return this->file.printSummary();
};

std::vector<std::vector<kiss_fft_cpx>> spectrum::Processing::getfftSpectrum() {
    if (this->fft.empty()) 
        this->_terminate(EMPTY_CONTAINER);

    std::vector<std::vector<kiss_fft_cpx>> r;

    for (int i = 0; i < this->getFileChannels(); i++) {
        /* _getV() copies values to a vector 
         * from a pointer to array kiss_fft_cpx and returns vector */
        r.push_back(this->_getV<kiss_fft_cpx>(this->fft[i].get()));
    }
    return r;
};

std::vector<std::vector<float>> spectrum::Processing::getScaledfftSpectrum() {
    if (this->scaled.empty()) 
        this->_terminate(EMPTY_CONTAINER);

    std::vector<std::vector<float>> r;

    for (int i = 0; i < this->getFileChannels(); i++) {
        r.push_back(this->_getV<float>(this->scaled[i].get()));
    }
    return r;
};

std::vector<std::vector<std::pair<float, std::vector<kiss_fft_cpx>>>> spectrum::Processing::getpfftSpectrum() {
    if (this->pfft.empty()) 
        this->_terminate(EMPTY_CONTAINER);
        
    std::vector<std::vector<std::pair<float, std::vector<kiss_fft_cpx>>>> r; 
    r.resize(this->getFileChannels());

    for (int i = 0; i < this->getFileChannels(); i++) {
        for (int j = 0; j < this->pfft[i].size(); j++) {
            r[i].push_back(std::make_pair(this->pfft[i][j].first, this->_getV<kiss_fft_cpx>(this->pfft[i][j].second.get())));
        }
    }
    return r;
};

std::vector<std::vector<std::pair<float, std::vector<float>>>> spectrum::Processing::getScaledpfftSpectrum() {
    if (this->pscaled.empty()) 
        this->_terminate(EMPTY_CONTAINER);
    
    std::vector<std::vector<std::pair<float, std::vector<float>>>> r; 
    r.resize(this->getFileChannels());

    for (int i = 0; i < this->getFileChannels(); i++) {
        for (int j = 0; j < this->pscaled[i].size(); j++) { 
            r[i].push_back(std::make_pair(this->pscaled[i][j].first, this->_getV<float>(this->pscaled[i][j].second.get())));
        }
    }
    return r;
};

void spectrum::Processing::FFT() {
    kiss_fftr_cfg cfg = kiss_fftr_alloc(this->NFFT, false, 0, 0);
     
    if (!cfg)
        this->_terminate(BAD_ALLOCATE);
    
    for (int i = 0; i < this->getFileChannels(); i++){
        /* Declaring an array for future FFT values */
        this->fft.push_back(std::unique_ptr<kiss_fft_cpx>(new kiss_fft_cpx[this->NFFT / 2 + 1])); 
        /* Doing FFT for each channel of the audio file */
        kiss_fftr(cfg, this->file.samples[i].data(), this->fft[i].get());
    
        /* Array declaration for normalized spectrum values */
        this->scaled.push_back(std::unique_ptr<kiss_fft_scalar>(new kiss_fft_scalar[this->NFFT / 2 + 1]));
        /* Normalization of the spectrum to dB */
        this->scale(this->fft[i].get(), this->scaled[i].get());
    }
    kiss_fft_free(cfg);
};

void spectrum::Processing::pFFT(int timeScale) {
    kiss_fftr_cfg cfg = kiss_fftr_alloc(this->NFFT, false, 0, 0);

    if (!cfg)
        this->_terminate(BAD_ALLOCATE);

    if (timeScale < 1 || timeScale > 1000)
        this->_terminate(BAD_TIMESCALE);
    
    /* Allocate memory for each value of each channel */
    this->pfft.resize(this->getFileChannels()); 
    this->pscaled.resize(this->getFileChannels());

    for (int i = 0; i < this->getFileChannels(); i++) {
        /* The more we divide one second, the more total values we have. 
         * The final size of the array is found as the duration 
         * of the audio file * timeScale */
        for (float j = 0; j < (this->getFileDuration()) * timeScale; j++) { 
            
            /* Memory allocation for the values of non-normalized spectrum to the 
             * i-th channel at the j-th moment of time */
            this->pfft[i].push_back(std::make_pair((j / timeScale), std::unique_ptr<kiss_fft_cpx>(new kiss_fft_cpx[this->NFFT / 2 + 1])));

            /* Performing FFT for j-th moment of time 
             * (a moment of time an audio file is equal 
             * to the: 
             *        sample rate * (j / timeScale))        */
            kiss_fftr(cfg, this->file.samples[i].data() + (int)((j / timeScale) * this->getFileSampleRate()), this->pfft[i][j].second.get());
            
            /* Memory allocation for the values of normalized spectrum to the 
             * i-th channel at the j-th moment of time */
            this->pscaled[i].push_back(std::make_pair((j / timeScale), std::unique_ptr<kiss_fft_scalar>(new kiss_fft_scalar[this->NFFT / 2 + 1])));

            /* Spectrum normalization */
            this->scale(this->pfft[i][j].second.get(), this->pscaled[i][j].second.get());
        }
    }
    kiss_fft_free(cfg);
};

void spectrum::Processing::scale(kiss_fft_cpx* fft, kiss_fft_scalar* scaled) {
    for (int i = 0; i < this->NFFT / 2 + 1; i++) {
        scaled[i] = this->_scaleExpression(fft[i].r, fft[i].i);
    }
};

float spectrum::Processing::_scaleExpression(float r, float i) {
    /*                  x = sqrt(r^2 +i^2)
     * Distance between two points (Euclidean distance) 
     * calculated by Pythagorean theorem
     * 
     * We use the standard transformation - 20 * log10(x)
     * 
     * Then we sum the resulting number with the number 
     * mean the dynamic range (for the current "depth" of quantization)
     * 
     * Now the maximum amplitude is 0 = -96.33 + 96.33
     * 
     * Then divide by the same number, bringing all values from -inf to 1.0f, multiply by 100 */
    return (((20.0f * log10f(std::sqrt(std::pow(r, 2.0f) + std::pow(i, 2.0f))) + (-1 * this->DYNAMICRANGE)) / this->DYNAMICRANGE)) * 100;
};

template<typename T>
std::vector<T> spectrum::Processing::_getV(const T* t) {
    if (!t)
        this->_terminate(EMPTY_CONTAINER);

    /* Copy to the vector the values indicated by the pointers 
     * of the beginning and end of the array, then return this vector */
    return std::vector<T> (t, t + (this->NFFT / 2 + 1));
};

void spectrum::Processing::_terminate(const char* exitMessage) {
    std::cerr << "Error : " << exitMessage << std::endl;
    return exit(1);
};

std::ostream& operator<<(std::ostream& os, kiss_fft_cpx const& k) {
    /* In future move to spectrum::Export */
    return os << k.r << ';' << k.i;
};
