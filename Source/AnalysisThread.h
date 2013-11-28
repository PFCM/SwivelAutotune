//
//  AnalysisThread.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 22/11/13.
//
//

#ifndef __SwivelAutotune__AnalysisThread__
#define __SwivelAutotune__AnalysisThread__

#include "../JuceLibraryCode/JuceHeader.h"
#include "String.h"

class MainComponent;
#include "MainComponent.h"

/** A thread to perform our analysis.
 *  Note that this thread will likely not
 *  respond to 'threadShouldExit()'
 *  messages, because it doesn't make 
 *  much sense to
 */
class AnalysisThread : public Thread
{
public:
    // Constructs a new thread, needs a few references to get going
    AnalysisThread(AudioDeviceManager *manager, MidiOutput *mout, OwnedArray<SwivelString, CriticalSection> *strings, MainComponent* m);
    ~AnalysisThread();
    
    /** Run method, gets called in a new thread when start() is called,
     *  Don't call this yourself, unless you don't actually want it to 
     *  run in a new thread */
    void run() override;
    
    /** Sets the console to use for output. If nullptr nothing is output */
    void setConsole(TextEditor* where);
    /** Sets the FFT size and overlap, onset threshold up and onset threshold down (in that order)*/
    void setProcessingParams(int size, int overlap, double rmsUp, double rmsDown);
    
    class AnalysisEndMessage : public CallbackMessage
    {
    public:
        AnalysisEndMessage(Result r, MainComponent* m) : result(r), main(m) {};
        void messageCallback();
        
    private:
        Result result;
        MainComponent* main;
    };
    
private:
    // The audio input device
    AudioDeviceManager* deviceManager;
    MidiOutput* midiOut;
    OwnedArray<SwivelString, CriticalSection>* swivelStrings;
    TextEditor* console;
    MainComponent* main;
    
    // processing buffers
    double* audio;
    fftw_complex* spectrum;
    // processing params
    int fft_size;
    int overlap;
    int hop_size;
    bool failed;
    double rmsUp;
    double rmsDown;
    
    
    void log(String message);
    void exitThread();
};

#endif /* defined(__SwivelAutotune__AnalysisThread__) */
