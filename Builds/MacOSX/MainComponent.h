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

class MainComponent : public    Component,
                      private   ComboBox::Listener,
                                Button::Listener
{
public:
    MainComponent();
    ~MainComponent();
    
    void paint(Graphics& g);
    void resized();
    
    //=============================================================
    void comboBoxChanged(ComboBox* box);
    void buttonClicked(Button* button);
    
    
    enum WindowType {
        placeholder,
        HANN,
        HAMMING,
        BLACKMAN,
        RECTANGULAR
    };
    
private:
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
    
    // bit of output
    ScopedPointer<TextEditor> console;
    
    // for now just one string
    ScopedPointer<SwivelString> swivelString;
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
    //==========================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


#endif /* defined(__SwivelAutotune__MainComponent__) */
