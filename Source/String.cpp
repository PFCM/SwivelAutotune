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
#include "MainComponent.h"

//===========================================================
// Constructs a new string.
// Note that the storage space for the input and output of the fft is
// allocated OUTSIDE this class, it can be shared between strings but
// will have to be careful only one of them is running at the same time
// or awkwardness might ensue.
// This shouldn't be a problem given if more than one string is grabbing the audio,
// there are probably some other serious issues
SwivelString::SwivelString()
{
    bundleInit = false;
    audioInit = false;
    processing = false;
    gate = false;
    lastphase = 0;
    
    // for now we will just stuff some dummy data into the midibuffer
    for (int i = 0; i < 5; i++)
    {
        midiData.addEvent(MidiMessage(144, 66, i), i*sample_rate/2);
    }
}

SwivelString::~SwivelString()
{
    free(magnitudes);
    free(input_buffer);
}

// initialises from parsed data
void SwivelString::initialiseFromBundle(SwivelStringFileParser::StringDataBundle* bundle)
{
    midiMsbs = bundle->midi_msbs;
    targets = bundle->targets;
    fundamentals = bundle->fundamentals;
    measurements = bundle->measured_data;
    bundleInit = true;
    
    if (audioInit)
        finalInit();
}
// initialises audio requirements
void SwivelString::initialiseAudioParameters(fftw_plan p, double *in, fftw_complex *out, int fft_size, double sr, int ol)
{
    fft_plan = p;
    input = in;
    output = out;
    this->fft_size = fft_size;
    sample_rate = sr;
    overlap = ol;
    
    
    input_buffer = (double*) malloc(sizeof(double)*fft_size);
    magnitudes = (double*) malloc(sizeof(double)*fft_size/2);
    
    hop_size = fft_size/overlap;
    
    minBin = -1;
    maxBin = -1;
    
    audioInit = true;
    
    if (bundleInit)
        finalInit();
}

// final intialisation steps
void SwivelString::finalInit()
{
    double min=99999999,max=-1;
    
    for (int i = 0; i < fundamentals->size(); i++) // find min and max measured frequency
    {
        if ((*fundamentals)[i] > max)
            max = (*fundamentals)[i];
        if ((*fundamentals)[i] < min)
            min = (*fundamentals)[i];
    }
    
    minBin = std::max(0, freqToBin(4.0/5.0 * min));
    maxBin = std::min(fft_size-1, freqToBin(6.0/5.0 * max));
}

//============================================================
void SwivelString::audioDeviceIOCallback(const float **inputChannelData,
                                         int numInputChannels,
                                         float **outputChannelData,
                                         int numOutputChannels,
                                         int numSamples)
{
    // we are only interested if there is a bit of sound
    float RMS =0;
    vDSP_rmsqv(inputChannelData[0], 1, &RMS, numSamples);
    //std::cout << RMS <<std::endl;
    if (RMS >= 0.15 && gate == false)
    {
        processing = true;
        gate = true;
        std::cout << "bang" <<std::endl;
    }
    if (freqs.size() >= 20 || (RMS <= 0.1 && gate == true))
    {
        processing = false;
        gate = false;
        std::cout << "off" <<std::endl;
        
        //TODO this better
        // for now we will just average all of these
        // not ideal, because we tend to get jumps
        // in one direction only
        
        // also getting some weird nans, so we will ignore those
        int nancount = 0;
        double total = 0;
        for (int i = 0; i < freqs.size(); i++)
        {
            if (freqs[i] != NAN)
                total += freqs[i];
            else
                ++nancount;
        }
        determined_pitch = total/(freqs.size()-nancount);
        
        analysisThreadRef->notify(); // let's get out of here
    }
    
    if (processing)
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
            window(input, fft_size);
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
                    peaks.add(i+1);
                }
            }
            
            int best_peak = 0;
            for (int i = 0; i < peaks.size(); i++)
                if (magnitudes[peaks[i]] > magnitudes[peaks[best_peak]])
                    best_peak = i;
            
            fftw_complex best = {output[peaks[best_peak]][0], output[peaks[best_peak]][1]};
            phase = atan2(best[1], best[0]);
            if (lastphase != 0)
            {
                freqs.add(preciseBinToFreq(peaks[best_peak], lastphase-phase));
#ifdef DEBUG
                std::cout << freqs.getLast() << std::endl;
#endif
            }
            
            
            
            lastphase = phase;
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

/** returns the best guess */
double SwivelString::getBestFreq()
{
    return determined_pitch;
}

//===============================================================================
double SwivelString::magnitude(fftw_complex in)
{
    return sqrt(in[0]*in[0]+in[1]*in[1]);
}

double SwivelString::binToFreq(double bin)
{
    return (bin * sample_rate)/(double)fft_size;
}

// calculates more accurately the frequency if you give the change in phase across frames
double SwivelString::preciseBinToFreq(int bin, double phasedelta)
{
    // make sure the phase is appropriately wrapped
    if (phasedelta > HALFPI)
        phasedelta -= M_PI;
    if (phasedelta < HALFPI)
        phasedelta += M_PI;
    
    return binToFreq(bin - (phasedelta*ONEDIVPI));
}

// approx
int SwivelString::freqToBin(double freq)
{
    return (freq*(double)fft_size)/sample_rate;
}

void SwivelString::window(double *input, int size)
{
    switch (windowType)
    {
        case MainComponent::WindowType::HANN:
            Windowing::hann(input, size);
            break;
            
        case MainComponent::WindowType::HAMMING:
            Windowing::hamming(input, size);
            break;
            
        case MainComponent::WindowType::BLACKMAN:
            Windowing::blkman(input, size);
            break;
            
        case MainComponent::WindowType::RECTANGULAR: // rectangular is no windowing
        default: // default is no windowing
            break;
    }
}

//================================================================================
MidiBuffer* SwivelString::getMidiBuffer()
{
    return &midiData;
}

void SwivelString::setAnalysisThread(juce::Thread *thread)
{
    analysisThreadRef = thread;
}


