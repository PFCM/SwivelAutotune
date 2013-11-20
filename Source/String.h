//
//  String.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//
/**
    Represents a single string. Implements juce::AudioIODeviceCallback, so in order to do its calculations it must be
    added as a callback to the current audio device.
*/

#ifndef __SwivelAutotune__String__
#define __SwivelAutotune__String__

#include <iostream>
#include "../JuceLibraryCode/JuceHeader.h"
#include <fftw3.h>


class SwivelString : public AudioIODeviceCallback
{
public:
    SwivelString(fftw_plan, double* input, fftw_complex* output, int fft_size, double sr, int ol);
    ~SwivelString();
    //===========================================
    void audioDeviceIOCallback(const float** inputChannelData,
                               int numInputChannels,
                               float** outputChannelData,
                               int numOutputChannels,
                               int numSamples);
    void audioDeviceAboutToStart(AudioIODevice* device);
    void audioDeviceStopped();
    
    //===========================================
    Array<double>* getCurrentPeaksAsFrequencies();
private:
    fftw_plan fft_plan;
    double* input;
    fftw_complex* output;
    int fft_size;
    int overlap;
    int hop_size;
    int minBin, maxBin;
    double* input_buffer;
    double* magnitudes;
    Array<int, CriticalSection> peaks;
    double sample_rate;
    int windowType;
    //=============================================
    double magnitude(fftw_complex);
    void window(double* input, int size);
    
};

#endif /* defined(__SwivelAutotune__String__) */
