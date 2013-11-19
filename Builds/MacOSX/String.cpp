//
//  String.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#include "String.h"
#include "Windowing.h"
#include <Accelerate/Accelerate.h>

//===========================================================
SwivelString::SwivelString(fftw_plan plan, double* in, fftw_complex* out, int size, double sr, int ol) :
    fft_plan(plan),input(in),output(out),fft_size(size),overlap(ol),sample_rate(sr)
{
    input_buffer = (double*) malloc(sizeof(double)*fft_size);
    magnitudes = (double*) malloc(sizeof(double)*fft_size/2);
    
    hop_size = fft_size/overlap;
    
    minBin = 0;
    maxBin = fft_size/2;
}

SwivelString::~SwivelString()
{
    free(magnitudes);
    free(input_buffer);
}

//============================================================
void SwivelString::audioDeviceIOCallback(const float **inputChannelData,
                                         int numInputChannels,
                                         float **outputChannelData,
                                         int numOutputChannels,
                                         int numSamples)
{
    // will need to:
    //          buffer audio to size of fft
    //              TODO: overlap
    //          perform fft
    //          analyse
    static int input_index = 0;
    static int remaining = 0;
    // a number of cases here
    // 1) the buffer has remaining space >= numSamples
    //          just copy it in
    // 2) the buffer has some remaining space < numSamples
    //          copy part of it in, process, copy the rest in to the front
    if ((fft_size) - input_index >= numSamples)
    {
       /* for (int i =0; i < numSamples; i++)
        {
            input_buffer[input_index+i] = inputChannelData[0][i];
        }*/
        // convert to double
        vDSP_vspdp(inputChannelData[0], 1,       // input, stride
                   input_buffer+input_index, 1,  // output, stride
                   numSamples);                  // size
        input_index += numSamples;
        remaining = 0;
    }
    else if (input_index < fft_size-1)
    {
        int i;
        for (i = 0; i+input_index < fft_size; i++)
        {
            input_buffer[input_index+i] = inputChannelData[0][i];
        }
        remaining = i;
        input_index += remaining;
    }
    // do fft & process
    // copy into actual fft buffer and window
    if (input_index == fft_size)
    {
        // could possibly do with a filter
        memcpy(input, input_buffer, fft_size*sizeof(double));
        Windowing::hann(input, fft_size);
        peaks.clearQuick();
        fftw_execute(fft_plan);
        // find best peak (probably lowest)
        for (int i =minBin; i < maxBin; i++)
        {
            magnitudes[i] = magnitude(output[i]);
        }
        for (int i =minBin+1; i < maxBin; i++)
        {
            magnitudes[i] = (magnitudes[i]+magnitudes[i-1]) / 2.0 ;
        }
        for (int i = minBin+1; i < maxBin-1; i++)
        {
            if (magnitudes[i] > magnitudes[i+1] &&
                magnitudes[i] > magnitudes[i-1])
            {
                peaks.add(i);
            }
        }
        
        
        // shift buffer across by the hop size
        // set index to the new end of the buffer
        // hop size is fft_size/overlap samples
        // memmove is like memcpy but is safe with overlapping regions
        memmove(input_buffer, input_buffer+hop_size, sizeof(double)*(fft_size-hop_size)); // roll across one hop
        input_index = fft_size-hop_size;
    
    }
    
    if (remaining != 0)
    {
        input_index=0;
        for (int i = remaining; i < numSamples; i++)
        {
            input_buffer[input_index++] = inputChannelData[0][i];
        }
        remaining = 0;
    }
}

void SwivelString::audioDeviceAboutToStart(juce::AudioIODevice *device)
{
    // nothing I can think of immediately
}

void SwivelString::audioDeviceStopped()
{
    // as above...
}

//===============================================================================
/** Returns the current peaks */
Array<double>* SwivelString::getCurrentPeaksAsFrequencies()
{
    static Array<double>* peaksHz = new Array<double>();
    peaksHz->clear();
    double f = sample_rate/(double)fft_size; // fundamental of the series
    for (int i =0; i < peaks.size(); i++)
    {
        peaksHz->add(f*peaks[i]);
    }
    
    return peaksHz;
}

//===============================================================================
double SwivelString::magnitude(fftw_complex in)
{
    return sqrt(in[0]*in[0]+in[1]*in[1]);
}


