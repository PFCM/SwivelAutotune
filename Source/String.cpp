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
#include "ElementComparator.h"

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
    if (magnitudes == nullptr)
        free(magnitudes);
    if (input_buffer == nullptr)
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
    if (RMS >= 0.001 && gate == false)
    {
        processing = true;
        gate = true;
        std::cout << "bang" <<std::endl;
    }
    if (freqs.size() >= 20 || (RMS <= 0.001 && gate == true))
    {
        processing = false;
        gate = false;
        std::cout << "off" <<std::endl;
        
        // make table -- this could be in a background thread, meaning we could start the next one a bit sooner
        // although while this is a lot of code, it isn't really that much heavy lifting
        processFrequencies();
        
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
// populate final lookup table
void SwivelString::processFrequencies()
{
    determined_pitch = calculateBestFrequency();
    // now that we have the pitch of the string we can start doing some interpolation
    // first step is to figure out where our newly determined fundamental fits within our measured data
    int above = -1;
    int below = -1;
    
    Array<double> derived_data; // make this here b/c it doesn't need to hand around beyond the construction of the lookup table
    
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
    Array<double>* low = (*measurements)[below];
    Array<double>* high = (*measurements)[above]; // stupid asterisks are silly
    
    // temporary variables
    double l;
    double h;
    double g;
    double d;
    double r;
    for (int i = 0; i < low->size(); i++)
    {
        // just to check let's lay these out for now
        l = (*low)[i];
        h = (*high)[i];
        g = h-l;
        d = g*c;
        r = l+d;
        derived_data.add(r);
        //std::cout << r<< std::endl;
    }
    
    
    // now we can try to construct a table of pitch bend values for virtual frets
    // TODO how to extrapolate?
    // TODO something better than linear interpolation for steps along the string
    fillLookupTable(derived_data);
    
    /*for (int i = num; i < num+24; i++)
     {
     uint16 note = note_key_table[i];
     if (note == OFFSTRING_NOTE)
     std::cout << "NOTE OFF STRING\n";
     else if (note == INVALID_NOTE)
     std::cout << "INVALID NOTE\n";
     else
     std::cout << note << std::endl;
     }*/
}

double SwivelString::calculateBestFrequency()
{
    
    //probably an unnecessarily intimidating way to do this, but lambdas are fun
    auto compare =
    [] (double a, double b) -> int
    {
        if (b-a > 0)
            return -1;
        if (b-a < 0)
            return 1;
        else
            return 0;
    };
    
    ElementComparator<double> e(compare);
    
    freqs.sort(e);
    
    OwnedArray<Range> ranges; // a range is an tuple of lowest(double), highest(double), start index(int) and end(int)
    ranges.add(new std::tuple<double, double, int, int>(freqs[0], 0.0, 0, 1));
    
    double threshold = 1.0; // maximum size of range
    int rindex = 0;
    for (int i = 1; i < freqs.size(); i++)
    {
        if ((freqs[i]-std::get<0>(*ranges[rindex])) < threshold)
        {
            std::get<3>(*ranges[rindex]) = i+1;
            if ((freqs[i] - std::get<1>(*ranges[rindex])) > 0)
                std::get<1>(*ranges[rindex]) = freqs[i];
        }
        else // this particular range is done
        {
            ranges.add(new Range(freqs[i], 0.0, i, i+1));
            rindex++;
        }
    }
    
    // find the range with the most data
    Range bestRange(0,0,0,0);
    for (Range*& r : ranges)
    {
        if (std::get<3>(*r)-std::get<2>(*r) > std::get<3>(bestRange)-std::get<2>(bestRange))
        {
            bestRange = *r; // copy the best into bestRange
        }
    }
    double total = 0;
    for (int i = std::get<2>(bestRange); i < std::get<3>(bestRange); i++)
    {
        total += freqs[i];
    }
    
    return total / (std::get<3>(bestRange)-std::get<2>(bestRange));
}

void SwivelString::fillLookupTable(Array<double>& derived_data)
{
    
    // start by going through each target
    int dIndex=0;
    int number = num;
    // we are going to make a two octave lookup table of pitch bend values by note numbers
    int gtcount = 0; // how many times we've had to go over the top
    for (int i = 0; i < targets->size(); i++,number++)
    {
        if ((*targets)[i] < determined_pitch) // we can't do much with values lower than the open string
            note_key_table.set(number, INVALID_NOTE);
        if ((*targets)[i] > derived_data[derived_data.size()-1]) // for now we will just take the gradient between the highest two
        {
            ++gtcount;
            double m = note_key_table[number-gtcount] - note_key_table[number-gtcount-1]; // need to stop this wrapping
            int amount = gtcount*m;
            int result = note_key_table[number-gtcount] + amount; // store it in a signed int to clamp it easier
            if (result < 0)
            {
                result = 0;
            }
            note_key_table.set(number, result);
        }
        else // must be greater than or equal to determined_pitch
        {
            double target = (*targets)[i];
            while (target >= derived_data[dIndex])
                dIndex++;
            
            if (dIndex == 0)
            {
                if (std::fabs(1-(target/determined_pitch)) < 0.001)
                    note_key_table.set(number, OPEN_NOTE);
                else
                    note_key_table.set(number, OFFSTRING_NOTE);
            }
            else
            {
                double a = derived_data[dIndex] - derived_data[dIndex-1];
                double b/* = 0;
                         if (dIndex == 0)
                         b = target - determined_pitch;
                         else
                         b */= target - derived_data[dIndex-1];
                double c = b/a;
                int start = (*midiPitchBend)[dIndex];
                int end   /*= 0;
                           if (dIndex == 0)
                           end = 127<<7;
                           else
                           end */= (*midiPitchBend)[dIndex-1];
                
                note_key_table.set(number, end + (start-end)*c);
            }
        }
    }
}

//===============================================================================
/** Returns the current peaks */
const Array<double>* SwivelString::getCurrentPeaksAsFrequencies() const
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
const MidiBuffer* SwivelString::getMidiBuffer() const
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

void SwivelString::reset()
{
    // undo calculations
    determined_pitch = std::numeric_limits<double>::signaling_NaN();
    processing = false;
    gate = false;
    peaks.clear();
    freqs.clear();
    analysisThreadRef = nullptr;
    note_key_table.clear();
    lastphase = 0;
    // undo audio init
    free(input_buffer);
    free(magnitudes);
    input_buffer = nullptr;
    magnitudes = nullptr;
    audioInit = false;
    
}

//===============================================================================
// Arguably the most important

MidiMessage SwivelString::transform(const juce::MidiMessage &msg) const
{
    static uint8 current_note;
    
    const uint8* data = msg.getRawData();
#ifdef DEBUG // do some double checking
    if (msg.getChannel() != channel)
        throw std::logic_error("String on channel: " + std::to_string(channel) + " being asked to transform message on channel: "
                               + std::to_string(msg.getChannel()));
    if (msg.isNoteOn() && (msg.getNoteNumber() < num || msg.getNoteNumber() > num+24))
        throw std::logic_error("String on channel: " +
                               std::to_string(channel) +
                               " given note number: " +
                               std::to_string(msg.getNoteNumber()) +
                               " which is outside operating range of" +
                               std::to_string(num) + "--" + std::to_string(num+24) + "\n'");
#endif
    // get status half of the first byte
    uint8 status = data[0] & 0xf0;
    if (!(status == 224 || status == 144)) return msg; // if it is anything but a pitchbend or a noteon, just pass it straight through
    
    // otherwise transform it
    // get the pitch bend number
    if (status == 224)
    {
        int pitchIn = (data[2] << 7) | data[1]; // comes in LSB first?
        
        // this needs to scale the pitch bend to the actual pitch bend values that would produce
        // a couple of semitones of deviation.
        // Perhaps this could be controlled with a cc message, will check GM spec.
        
        // for now lets set it at 2 semitones
        // we have a 14 bit value coming in
        // we want a total range of ±2 semitones from current_note
        // ideally this should move linearly in perceived pitch space (ie midi note number space)
        // for now it won't quite
        // range of input is 0-16384 (0-2^14)
        // we can divide into 4 sections of 4096 for each semitone and interpolate appropriately for each
        // TODO propoerly deal with notes outside the range in note_key_table
        uint16 target = 0;
        uint16 start = 0;
        double c = 0;
        if (pitchIn >= 12288) // and < 16384
        {
            // determine interpolation constant
            c = (pitchIn-12288)/4096.0;
            target = note_key_table[current_note+2];
            start = note_key_table[current_note+1];
        }
        else if (pitchIn >= 8192) // and < 12288
        {
            c = (pitchIn-8192)/4096.0;
            target = note_key_table[current_note+1];
            start = note_key_table[current_note];
        }
        else if (pitchIn >= 4096) // and < 8192
        {
            c = (pitchIn-4096)/4096.0;
            target = note_key_table[current_note-1];
            start  = note_key_table[current_note];
        }
        else // pitchIn > 0 && pitchIn < 4096
        {
            c = pitchIn/4096.0;
            target = note_key_table[current_note-2];
            start = note_key_table[current_note-1];
        }
        
        uint16 val = start + (start-target)*c;
        // now pack into a midi message
        uint8 d1 = val & 0x7f;
        uint8 d2 = (val >> 7) & 0x7f;
        
        return MidiMessage(224+channel-1, d1, d2);
    }
    else if (status == 144) // note on, move to a calculated position
    {
        // grab pitch bend value for the note, velocity currently ignored, could be mapped to pressure or something
        uint16 pbv = note_key_table[data[1]];
        if (pbv == INVALID_NOTE) return MidiMessage();
        if (pbv == OFFSTRING_NOTE)
        {
            std::cout << "note becomes open string\n";
            return MidiMessage();
        }
        uint8 d1 = pbv & 0x7f; // LSB
        uint8 d2 = (pbv >> 7) & 0x7f; // MSB
        
        current_note = data[1]; // store for calculating pitch bend
        
        return MidiMessage(224+channel-1, d1, d2);
    }
    
    return MidiMessage(); // default exit point does nothing just makes sure it compiles
}
