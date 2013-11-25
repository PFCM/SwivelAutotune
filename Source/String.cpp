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
    determined_pitch = std::numeric_limits<double>::signaling_NaN();
}

SwivelString::~SwivelString()
{
    free(magnitudes);
    free(input_buffer);
}

// initialises from parsed data
void SwivelString::initialiseFromBundle(SwivelStringFileParser::StringDataBundle* bundle)
{
    midiPitchBend = bundle->midi_pitchbend;
    targets = bundle->targets;
    fundamentals = bundle->fundamentals;
    measurements = bundle->measured_data;
    midiData = bundle->midiBuffer; // for now we hope the sample rates match up
    this->num = bundle->num;
    
    // figure out channel from the first MIDI message
    MidiBuffer::Iterator it(*midiData);
    const uint8* d;
    int num, off;
    it.getNextEvent(d, num, off);
    channel = (d[0] & 0xf) + 1; // channel is last 4 bits + 1
    
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
    
    // figure out the length of the midi buffer to see how long to wait before listening
    // Currently the idea is that we will start listening at the same time as the second to last midi message
    //
    MidiBuffer::Iterator i(*midiData);
    MidiMessage msg;
    int samplePos;
    int time=0;
    int num=0;
    int cap = midiData->getNumEvents();
    while (i.getNextEvent(msg, samplePos))
    {
        num++;
        if (num != cap)
            time = samplePos; // count up all the messages except the last
    }
    // now we are here, convert to ms
    delay = time / 44.1;
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
    if (RMS >= 0.01 && gate == false)
    {
        processing = true;
        gate = true;
        std::cout << "bang" <<std::endl;
    }
    if (freqs.size() >= 20 || (RMS <= 0.01 && gate == true))
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
            if (!std::isnan(freqs[i]))
                total += freqs[i];
            else
                ++nancount;
        }
        determined_pitch = total/(freqs.size()-nancount);
        
        // now that we have the pitch of the string we can start doing some interpolation
        // first step is to figure out where our newly determined fundamental fits within our measured data
        int above = -1;
        int below = -1;
        
        for (int i = 0; i < fundamentals->size(); i++)
        {
            double current = (*fundamentals)[i];
            
            if (determined_pitch >= current)
            {
                if (below == -1)
                    below = i;
                else if (current >= (*fundamentals)[below]) // finding nearest measurement underneath
                    below = i;
            }
            else // determined_pitch < current (finding nearest measurement above)
            {
                if (above == -1)
                    above = i;
                else if (current < (*fundamentals)[above])  // nearest
                    above = i;
            }
        }
        
        // now we need to determine our interpolation constant, assuming linear interpolation
        double lowest = (*fundamentals)[below];
        double highest = (*fundamentals)[above];
        double gapA = highest-lowest;
        double gapB = determined_pitch - lowest;
        double c = gapB / gapA;
        
        // now use c to fill in a lookup table of the extrapolated characteristics of the current tuning
        Array<double> low = *(*measurements)[below];
        Array<double> high = *(*measurements)[above]; // stupid asterisks are silly
        for (int i = 0; i < low.size(); i++)
        {
            // just to check let's lay these out for now
            double l = low[i];
            double h = high[i];
            double g = h-l;
            double d = g*c;
            double r = l+d;
            derived_data.add(r);
        }
        
        // now we can try to construct a table of pitch bend values for virtual frets
        // to which we can quantise incoming notes
        // TODO how to extrapolate?
        // TODO something better than linear interpolation for steps along the string
        
        // start by going through each target
        int dIndex=0;
        int number = num;
        // we are going to make a two octave lookup table of pitch bend values by note numbers
        
        for (int i = 0; i < targets->size(); i++,number++)
        {
            if ((*targets)[i] < determined_pitch || (*targets)[i] > derived_data[derived_data.size()-1]) // we can't do much with values lower than the open string
                note_key_table.set(number, INVALID_NOTE);
            else // must be greater than or equal to determined_pitch
            {
                double target = (*targets)[i];
                while (target >= derived_data[dIndex])
                    dIndex++;
                // issue here with the derived data being smaller than targets
                // will do some fancy extrapolation
                // but for now we will fill in those values with INVALID_NOTES
                double a = derived_data[dIndex] - derived_data[dIndex-1];
                double b = 0;
                if (dIndex == 0)
                    b = determined_pitch - derived_data[dIndex-1];
                else
                    b = target - derived_data[dIndex-1];
                double c = b/a;
                int start = (*midiPitchBend)[dIndex];
                int end   = 0;
                if (dIndex == 0)
                    end = 127;
                else
                    end = (*midiPitchBend)[dIndex-1];
                
                note_key_table.set(number, end + (start-end)*c);
            }
        }
        
        
        analysisThreadRef->notify(); // let's get out of here
    }
    
    if (processing)
    {
        
        // will need to:
        //          buffer audio to size of fft
        //
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
Array<double>* SwivelString::getCurrentPeaksAsFrequencies() const
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
double SwivelString::getBestFreq() const
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
MidiBuffer* SwivelString::getMidiBuffer() const
{
    return midiData.get(); // return it without messing with the scope and whatnot
}

void SwivelString::setAnalysisThread(juce::Thread *thread)
{
    analysisThreadRef = thread;
}

bool SwivelString::isFullyInitialised() const
{
    return bundleInit && audioInit;
}

double SwivelString::getWaitTime() const
{
    return delay;
}

int SwivelString::getMidiChannel() const
{
    return channel;
}

bool SwivelString::isReadyToTransform() const
{
    return bundleInit && audioInit && (std::isnormal(determined_pitch));
}

//===============================================================================
// Arguably the most important

MidiMessage SwivelString::transform(const juce::MidiMessage &msg) const
{
    const uint8* data = msg.getRawData();
#ifdef DEBUG // do some double checking
    if (msg.getChannel() != channel)
        throw std::logic_error("String on channel: " + std::to_string(channel) + " being asked to transform message on channel: "
                               + std::to_string(msg.getChannel()));
    if (msg.isNoteOn() && (msg.getNoteNumber() < num || msg.getNoteNumber() > num+24))
        throw std::logic_error("String on channel: " +
                               std::to_string(channel) +
                               "given note number: " +
                               std::to_string(msg.getNoteNumber()) +
                               "which is outside operating range of" +
                               std::to_string(num) + "--" + std::to_string(num+24));
#endif
    // get status half of the first byte
    uint8 status = data[0] & 0xf0;
    if (!(status == 176 || status == 144)) return msg; // if it is anything but a pitchbend or a noteon, just pass it straight through
    
    // otherwise transform it
    // get the pitch bend number
    if (status == 176)
    {
        int pitchIn = (data[2] << 7) | data[1]; // comes in LSB first?
        
        // this needs to scale the pitch bend to the actual pitch bend values that would produce
        // a couple of semitones of deviation.
        // Perhaps this could be controlled with a cc message, will check GM spec.
    }
    else if (status == 144) // note on, move to a calculated position
    {
        // grab pitch bend value for the note, velocity currently ignored, could be mapped to note pressure or something like that
        uint16 pbv = note_key_table[data[1]];
        uint8 d1 = pbv & 0x7f; // LSB
        uint8 d2 = (pbv >> 7) & 0x7f; // MSB
        
        return MidiMessage(176+channel-1, d1, d2);
    }
    
    return MidiMessage(); // default exit point does nothing just makes sure it compiles
}
