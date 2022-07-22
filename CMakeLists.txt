CMAKE_MINIMUM_REQUIRED(VERSION 3.6)

PROJECT(Spectrum
    VERSION 1.0.0 
    DESCRIPTION "Open-Source C++ library for Fast Fourier Transform (FFT) of WAV/AIFF audio files"
    HOMEPAGE_URL "https://github.com/6dba/Spectrum"
)

ADD_LIBRARY(kissfft STATIC 
    libs/kissfft-131.1.0/kfc.c
    libs/kissfft-131.1.0/kiss_fft.c
    libs/kissfft-131.1.0/kiss_fftnd.c
    libs/kissfft-131.1.0/kiss_fftndr.c
    libs/kissfft-131.1.0/kiss_fftr.c
) 

ADD_LIBRARY(${PROJECT_NAME} STATIC 
    src/Processing.cpp
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} kissfft)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC 
    include 
    libs/kissfft-131.1.0 
    libs/AudioFile-1.1.0
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION DESCRIPTION HOMEPAGE_URL 
    ${PROJECT_VERSION} ${PROJECT_DESCRIPTION} ${PROJECT_HOMEPAGE_URL}
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PRIVATE_HEADER 
    "Processing.h;"
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER Spectrum.h)
