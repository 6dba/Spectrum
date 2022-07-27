#include "Processing.h"

spectrum::Processing::Processing(int NFFT, const char* AUDIOFILE) 
    : NFFT(NFFT), 
    FILE(AUDIOFILE) 
{
    if (NFFT <= 0 || NFFT % 2 != 0)
        this->_terminate(BAD_NFFT);

    /* Real input signal - all imaginary parts are zero 
     * Reading data from an audio file
     * file.samples - contains a vector of vectors,
     * which contains the frames of each channel */
    this->file.load(this->FILE);
    
    /* Dynamic range
     * With a bit depth of 16 bits from 32767 to -32768 (65536)
     * Is equal to 96.33Db
     * Use this value in the future to normalize the FFT values */
    this->dynamicRange = std::abs(20.0f * log10f(1.0f / (float)std::pow(2, this->getBitDepth())));
};

template<typename v, typename sV>
spectrum::Processing::Keepeth<v, sV>::Keepeth(int ch, float fpb, float t, v vls, sV sVls)
    : channel(ch),
    freqPerBin(fpb),
    time(t),
    /* Transfer of ownership of the pointer in the initializer (std::move) */
    values(std::move(vls)),
    scaledValues(std::move(sVls)) {};

spectrum::Processing::~Processing() {};

int 
spectrum::Processing::getNFFT() {
    return this->NFFT;
};

float 
spectrum::Processing::getFreqPerBin() {
    return ((float)this->getSampleRate() / (float)this->NFFT);
};

int 
spectrum::Processing::getSampleRate() {
    return this->file.getSampleRate();
};

float 
spectrum::Processing::getFileDuration() {
    return this->file.getLengthInSeconds();
};

int 
spectrum::Processing::getFramesPerChannel() {
    return this->file.getNumSamplesPerChannel();
};

int 
spectrum::Processing::getTotalFrames() {
    return this->getFramesPerChannel() * this->getChannels();
};

int 
spectrum::Processing::getChannels() {
    return this->file.getNumChannels();
};

std::vector<std::vector<float>> 
spectrum::Processing::getFrames() {
    return this->file.samples;
};

int 
spectrum::Processing::getBitDepth() {
    return this->file.getBitDepth();
};

bool 
spectrum::Processing::isMono() {
    return this->file.isMono();
};

void 
spectrum::Processing::printSummary() {
    std::cout << "FFT Window size: " << this->getNFFT()
              << "\nFrequency per bin: " << this->getFreqPerBin()
              << "\nSample rate: " << this->getSampleRate()
              << "\nDuration in seconds: " << this->getFileDuration()
              << "\nFrames per channel: " << this->getFramesPerChannel()
              << "\nTotal frames: " << this->getTotalFrames()
              << "\nNumber of channels: " << this->getChannels()
              << "\nBit depth: " << this->getBitDepth()
              << std::endl;
};

spectrum::Processing::storage_t 
spectrum::Processing::getfftValues() {
    return this->_peekValues(this->storage, 0, this->storage.size());
};

spectrum::Processing::storage_t 
spectrum::Processing::getpfftValues() {
    return this->_peekValues(this->pstorage, 0, this->pstorage.size());
};

spectrum::Processing::storage_t 
spectrum::Processing::getpfftValues(int channel) {
    if (channel >= this->getChannels() || channel < 0)
        this->_terminate(BAD_CHANNEL);
    
    /* Due to the fact that each channel has the same number of samples, 
     * we calculate the number of samples per channel, 
     * then shift along the original vector, 
     * return the range of values of the desired channel */ 
    const int binsPerChannel = this->pstorage.size() / this->getChannels();
    return this->_peekValues(this->pstorage, channel * binsPerChannel,
                                             (channel + 1) * binsPerChannel);
};

void 
spectrum::Processing::FFT() {
    kiss_fftr_cfg cfg = kiss_fftr_alloc(this->NFFT, false, 0, 0);
     
    if (!cfg)
        this->_terminate(BAD_ALLOCATE);
    
    for (int i = 0; i < this->getChannels(); i++) {
        /* Creating a structure object that contains all the necessary 
         * properties for storing conversion values 
         * at a (j/timeScale) moment in time, 
         * allocating memory for arrays of FFT values */
        this->storage.push_back(
                    Keepeth<std::unique_ptr<kiss_fft_cpx>, 
                            std::unique_ptr<kiss_fft_scalar>>
                            (i, this->getFreqPerBin(), -1, 
                            std::unique_ptr<kiss_fft_cpx>(new kiss_fft_cpx[this->NFFT / 2 + 1]),
                            std::unique_ptr<kiss_fft_scalar>(new kiss_fft_scalar[this->NFFT / 2 + 1]))
        ); 
        /* Doing FFT for each channel of the audio file */
        kiss_fftr(cfg, this->file.samples[i].data(), this->storage[i].values.get());
    
        /* FFT normalization to db */
        this->scale(
                this->storage[i].values.get(), 
                this->storage[i].scaledValues.get()
        );
    }
    kiss_fft_free(cfg);
};

void 
spectrum::Processing::pFFT(int timeScale) {
    kiss_fftr_cfg cfg = kiss_fftr_alloc(this->NFFT, false, 0, 0);

    if (!cfg)
        this->_terminate(BAD_ALLOCATE);

    if (timeScale < 1 || timeScale > 1000 )
        this->_terminate(BAD_TIMESCALE);
    
    /* If the storage is not empty and if the time scaling factor 
     * of the previous transformation is not equal to the current one, 
     * then we clear the storage for the new values */
    if (!this->pstorage.empty()) {
        if (timeScale != 1 / this->pstorage[1].time)
           this->pstorage.clear();
        return;
    }

    /* Performing FFT for j-th moment of time 
     * (a moment of time an audio file is equal 
     * to the: */
    const int segment = this->getSampleRate() / timeScale;
     
    /* i - iterated by channels, 
     * k - iterator for the storage of FFT values, 
     * in which the FFT of each channel is stored sequentially 
     * (one after the other), 
     * j - iterated by frames of a particular channel */
    for (int i = 0, k = 0; i < this->getChannels(); i++) {
        /* The more we divide one second, the more total values of time moments 
         * we have. The final size of the array is found as the duration 
         * of the audio file * timeScale */
        for (int j = 0; (float)j < this->getFileDuration() * timeScale; j++,k++) { 
            
            /* Creating a structure object that contains all the necessary 
             * properties for storing conversion values 
             * at a (j/timeScale) moment in time, 
             * allocating memory for arrays of FFT values */
            this->pstorage.push_back(
                    Keepeth<std::unique_ptr<kiss_fft_cpx>, 
                            std::unique_ptr<kiss_fft_scalar>>
                            (i, this->getFreqPerBin(), (float)j / timeScale, 
                            std::unique_ptr<kiss_fft_cpx>(new kiss_fft_cpx[this->NFFT / 2 + 1]),
                            std::unique_ptr<kiss_fft_scalar>(new kiss_fft_scalar[this->NFFT / 2 + 1]))
            );
            
            /* We select the segment of the audio file 
             * for which the FFT will be performed */
            std::vector<float> v(
                    this->file.samples[i].data() + (segment * j), 
                    this->file.samples[i].data() + (segment * (j + 1))
            );
            
            kiss_fftr(cfg, v.data(), this->pstorage[k].values.get());
 
            /* FFT normalization to db */
            this->scale(
                    this->pstorage[k].values.get(), 
                    this->pstorage[k].scaledValues.get()
            );
        }
    }
    kiss_fft_free(cfg);
};

void 
spectrum::Processing::scale(kiss_fft_cpx* fft, kiss_fft_scalar* scaled) {
    for (int i = 0; i < this->NFFT / 2 + 1; i++) {
        scaled[i] = this->_scaleExpression(fft[i].r, fft[i].i);
    }
};

float 
spectrum::Processing::_scaleExpression(float r, float i) {
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
    return (((20.0f * log10f(std::sqrt(std::pow(r, 2.0f) + std::pow(i, 2.0f))) + (-1 * this->dynamicRange)) / this->dynamicRange)) * 100;
};

template<typename T>
std::vector<T> 
spectrum::Processing::_getVector(const T* t, const int S) {
    /* Copy to the std::vector the values indicated by the pointers 
     * of the beginning and end of the array, then return this std::vector */
    return std::move(std::vector<T> (t, t + S));
};

spectrum::Processing::storage_t 
spectrum::Processing::_peekValues(lstorage_t& s, const int beg, const int end) {
    if (s.empty()) 
        this->_terminate(EMPTY_CONTAINER);
    storage_t r; 

    for (int i = beg; i < end; i++) {
        r.push_back(Keepeth<std::vector<kiss_fft_cpx>, std::vector<float>>(
                    s[i].channel, 
                    s[i].freqPerBin,
                    s[i].time, 
                    this->_getVector<kiss_fft_cpx>(s[i].values.get(), this->NFFT / 2 + 1),
                    this->_getVector<float>(s[i].scaledValues.get(), this->NFFT / 2 + 1)
            )
        );
    }

    return std::move(r);
};

void 
spectrum::Processing::_terminate(const char* exitMessage) {
    std::cerr << "Error : " << exitMessage << std::endl;
    return exit(1);
};

std::ostream& 
operator<<(std::ostream& os, kiss_fft_cpx const& k) {
    /* In future move to spectrum::Export */
    return os << k.r << ';' << k.i;
};
