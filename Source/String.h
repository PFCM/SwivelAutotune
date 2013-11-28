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
    Each string has a separate midi channel, and once it has done its 
    analysis can be passed midi messages to transform them according to the results.
*/

#ifndef __SwivelAutotune__String__
#define __SwivelAutotune__String__

#include <iostream>
#include <tuple>
#include "../JuceLibraryCode/JuceHeader.h"
#include <fftw3.h>
#include "SwivelStringFileParser.h"


class SwivelString : public AudioIODeviceCallback
{
public:
    SwivelString();
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
    void initialiseAudioParameters(fftw_plan, double* input, fftw_complex* output, int fft_size, double sr, int ol, double upThresh, double downThresh);
    
    //===========================================
    /** Returns current list of peaks in Hz */
    const Array<double>* getCurrentPeaksAsFrequencies() const;
    /** Returns the midi data required to make things go */
    const MidiBuffer* getMidiBuffer() const;
    
    /** Set the thread to notify when processing is complete */
    void setAnalysisThread(Thread* thread);
    
    /** Returns the best guess at the end of the analysis stage */
    double getBestFreq() const;
    
    /** Gets the length of time to wait before adding this string as an audio callback after starting the MIDI buffer */
    double getWaitTime() const;
    
    /** Returns true iff both initialisation routines have completed and the final initialisation succeeded */
    bool isFullyInitialised() const;
    
    //============================================
    // MIDI transformation functions
    /** Transforms MIDI if it is for this string and this string is in a state where it is happy to do it */
    MidiMessage transform(const MidiMessage &msg) const;
    /** Returns true if this string has been initialised and done sufficient processing to want to work on MIDI */
    bool isReadyToTransform() const;
    /** Gets the MIDI channel this string is working on, this is derived from the given MIDI data in the data file used to construct 
        the string */
    int getMidiChannel() const;
    
    //===========================================
    /** Resets all calculated data in preparation for recalculation.
        This keeps the bundle initialisation but resets the audio information.*/
    void reset();
    
private:
    //===========================================
    typedef std::tuple<double, double, int, int> Range;
    
    //===========================================
    // how to make it go
    ScopedPointer<MidiBuffer> midiData;
    
    //=====INFO OF THE STRING====================
    /** the measured characteristics (2 or more) */
    ScopedPointer<OwnedArray<Array<double>>> measurements;
    /** The open frequencies of the strings where the measurements
     *   were taken, matches up by index to the measurements */
    ScopedPointer<Array<double>> fundamentals;
    /** The desired frequencies per ``fret'' */
    ScopedPointer<Array<double>> targets;
    /** The midi pitchbend values that created the 
     *  characterisation tables, essentially the values which
     *  we want to produce the target frequencies             */
    ScopedPointer<Array<uint16>> midiPitchBend;
    
    //=====FFT STUFF=============================
    bool processing;
    bool gate;
    
    fftw_plan fft_plan;
    double* input;
    fftw_complex* output;
    int fft_size;
    int overlap;
    int hop_size;
    int minBin, maxBin;
    double rmsUp, rmsDown;
    double* input_buffer;
    double* magnitudes;
    Array<int, CriticalSection> peaks;
    Array<double> freqs;
    double sample_rate;
    int windowType;
    //=============================================
    double magnitude(fftw_complex);
    void window(double* input, int size);
    double binToFreq(double bin);
    int freqToBin(double freq);
    double preciseBinToFreq(int bin, double phasedelta);
    //=============================================
    // takes the frequencies and populates the note lookup table
    void processFrequencies();
    // gets the best frequency from the calculated ones
    double calculateBestFrequency();
    // actually fill in note_key_table, takes an array of frequency estimates for the determined fundamental
    void fillLookupTable(Array<double>& derived_data);
    //=============================================
    // some misc. internal variables etc
    bool bundleInit;
    bool audioInit;
    
    
    void finalInit();
    
    double delay;
    
    Thread* analysisThreadRef;
    
    double phase;
    double lastphase;
    double determined_pitch;
    
    // some constants to save time
    // 1/PI, useful for the frequency calculation
    static constexpr double ONEDIVPI       = 1.0/M_PI;
    // PI/2
    static constexpr double HALFPI         = 0.5*M_PI;
    // An invalid note for some reason, most likely too high pitched for this string
    static constexpr uint16  INVALID_NOTE   = 0xffff; // could be anything > 16384
    // A note too low for the string (or too low for the servo to reach)
    static constexpr uint16  OFFSTRING_NOTE = 0xfffe;
    // A note that is near enough to the open string that it is worth playing
    static constexpr uint16  OPEN_NOTE      = 0xfffd;
    // returns distance in cents (100th of an equal-tempered semitone)
    static double cents(double a, double b);
    //===============================================
    // some MIDI info
    int channel;
    // the lookup table of notes to pitchbend values
    HashMap<uint8, uint16> note_key_table;
    // beginning MIDI note number
    int num;
};

#endif /* defined(__SwivelAutotune__String__) */
