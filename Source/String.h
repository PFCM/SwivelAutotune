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
#include "SwivelStringFileParser.h"


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
    void initialiseFromBundle(SwivelStringFileParser::StringDataBundle* bundle);
    
    //===========================================
    /** Returns current list of peaks in Hz */
    Array<double>* getCurrentPeaksAsFrequencies();
    /** Returns the midi data required to make things go */
    MidiBuffer* getMidiBuffer();
    
private:
    //===========================================
    // how to make it go
    MidiBuffer midiData;
    
    //=====INFO OF THE STRING====================
    /** the measured characteristics (2 or more) */
    ScopedPointer<OwnedArray<Array<double>>> measurements;
    /** The open frequencies of the strings where the measurements
     *   were taken, matches up by index to the measurements */
    ScopedPointer<Array<double>> fundamentals;
    /** The desired frequencies per ``fret'' */
    ScopedPointer<Array<double>> targets;
    /** The midi pitchbend values (msb only) that created the 
     *  characterisation tables, essentially the values which
     *  we want to produce the target frequencies             */
    ScopedPointer<Array<uint8>> midiMsbs;
    
    //=====FFT STUFF=============================
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
