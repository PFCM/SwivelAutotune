//
//  String.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//
/**
    Represents a single string. Implements juce::AudioIODeviceCallback, so in order to do its calculations it must be
    added as a callback to the current audio device. Also requires an fftw plan as a constructor argument in order to 
    facilitate sharing plans between strings.
*/

#ifndef __SwivelAutotune__String__
#define __SwivelAutotune__String__

#include <iostream>
#include "../JuceLibraryCode/JuceHeader.h"
#include <fftw3.h>

class SwivelString : public AudioIODeviceCallback
{
public:
    SwivelString(fftw_plan fft_plan);
    ~SwivelString();
    //===========================================
    void audioDeviceIOCallback(const float** inputChannelData,
                               int numInputChannels,
                               float** outputChannelData,
                               int numOutputChannels,
                               int numSamples);
    void audioDeviceAboutToStart(AudioIODevice* device);
    void audioDeviceStopped();
    
private:
    fftw_plan fft_plan;
};

#endif /* defined(__SwivelAutotune__String__) */
