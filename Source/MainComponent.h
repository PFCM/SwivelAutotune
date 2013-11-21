//
//  MainComponent.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#ifndef __SwivelAutotune__MainComponent__
#define __SwivelAutotune__MainComponent__

#include "../JuceLibraryCode/JuceHeader.h"
#include "String.h"
#include "MidiDeviceSelector.h"
#include "AnalysisThread.h"

class MainComponent : public    Component,
                      private   ComboBox::Listener,
                                Button::Listener,
                                MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent();
    
    void paint(Graphics& g);
    void resized();
    
    //=============================================================
    void comboBoxChanged(ComboBox* box);
    void buttonClicked(Button* button);
    
    void handleIncomingMidiMessage(MidiInput* input, const MidiMessage& msg);
    
    enum WindowType {
        placeholder,
        HANN,
        HAMMING,
        BLACKMAN,
        RECTANGULAR
    };
    
private:
    // for ease of use
    typedef SwivelStringFileParser::StringDataBundle StringDataBundle;
    
    //GUI stuff
    ScopedPointer<AudioDeviceManager> deviceManager;
    ScopedPointer<AudioDeviceSelectorComponent> audioSelector; // this exists
    
    ScopedPointer<Label> fftLabel;
    ScopedPointer<ComboBox> fftSizeBox;
    int FFTSizes[7] = {
        512,
        1024,
        2048,
        4096,
        8192,
        16384,
        32768
    };
    const int NUM_FFT_SIZES = 7;
    int fft_size = 8192;
    ScopedPointer<Label> overlapLabel;
    ScopedPointer<ComboBox> overlapBox;
    int overlapOptions[4] = {
      1,2,3,4
    };
    const int NUM_OVERLAPS = 4;
    int overlap = 2;
    
    // window
    ScopedPointer<Label> windowLabel;
    ScopedPointer<ComboBox> windowBox;
    WindowType window;
    
    ScopedPointer<TextButton> goButton;
    
    // MIDI
    ScopedPointer<MidiOutputDeviceSelector> midiOutBox;
    ScopedPointer<Label> midiOutLabel;
    
    ScopedPointer<MidiInputDeviceSelector> midiInBox;
    ScopedPointer<Label> midiInLabel;
    
    // bit of output
    ScopedPointer<TextEditor> console;
    
    // Strings!
    OwnedArray<SwivelString, CriticalSection> swivelStrings;
    
    // at the moment, let's have a button to choose a data file for this string
    ScopedPointer<TextButton> chooseFileButton;
    // we need some collection to hold the data
    OwnedArray<StringDataBundle> bundles;
    
    // data (this is shared between strings)
    double* audio;
    fftw_complex* spectra;
    fftw_plan plan;
    //==========================================================
    /** Appends text to end of console */
    static void log(String text, TextEditor* console);
    //==========================================================
    /** Reports data at a given interval */
    class Reporter : public Timer
    {
    public:
        Reporter(SwivelString* producer, TextEditor* log);
        void timerCallback();
        void setString(SwivelString* str);
        void setConsole(TextEditor* log);
    private:
        SwivelString* swString;
        TextEditor* console;
    };
    ScopedPointer<Reporter> reporter;
    
    //=========================================================
    // the thread which does the calculation work
    ScopedPointer<AnalysisThread> analysisThread;
    
    //============MEMBER FUNCTIONS=============================
    /** Opens a file and attempts to parse it, adding all the results to the
        array of bundles */
    void openFile();
    /** Shows a file chooser dialogue to search for files with the given pattern, returns choice
        if made otherwise an invalid file. */
    File showDialogue(const String& pattern);
    //==========================================================
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    void begin();/////////////////////////////////////////////////////////
    void end();  /////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //==========================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


#endif /* defined(__SwivelAutotune__MainComponent__) */
