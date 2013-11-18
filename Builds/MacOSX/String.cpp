//
//  String.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#include "String.h"
#include "Windowing.h"

//===========================================================
SwivelString::SwivelString(fftw_plan plan, double* in, fftw_complex* out, int size) : fft_plan(plan),
                                                                                      input(in),
                                                                                      output(out),
                                                                                      fft_size(size)
{
    input_buffer = (double*) malloc(sizeof(double)*fft_size);
    magnitudes = (double*) malloc(sizeof(double)*fft_size/2);
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
    //              TODO: overlap-add
    //          perform fft
    //          analyse
    static int input_index = 0;
    static int remaining = 0;
    // a number of cases here
    // 1) the buffer has remaining space >= numSamples
    //          just copy it in
    // 2) the buffer has some remaining space < numSamples
    //          copy part of it in, process, copy the rest in to the front
    if ((fft_size-1) - input_index >= numSamples)
    {
        // would love to vectorise but going between types isn't so much fun
        for (int i =0; i < numSamples; i++)
        {
            input_buffer[input_index+i] = inputChannelData[0][i];
        }
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
    if (input_index == fft_size-1)
    {
        memcpy(input, input_buffer, fft_size*sizeof(double));
        Windowing::hann(input, fft_size);
        fftw_execute(fft_plan);
        int peaks[6];
        int t=0;
        // find best peak (probably lowest)
        for (int i =minBin; i < maxBin; i++)
        {
            magnitudes[i] = magnitude(output[i]);
        }
        for (int i = minBin+1; i < maxBin-1; i++)
        {
            if (magnitudes[i] > magnitudes[i+1] &&
                magnitudes[i] > magnitudes[i-1])
            {
                peaks[t++] = i;
            }
        }
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
double SwivelString::magnitude(fftw_complex in)
{
    return sqrt(in[0]*in[0]+in[1]*in[1]);
}