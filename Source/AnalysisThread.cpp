//
//  AnalysisThread.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 22/11/13.
//
//

#include "AnalysisThread.h"

AnalysisThread::AnalysisThread(AudioDeviceManager *manager, MidiOutput *mout, OwnedArray<SwivelString, CriticalSection> *strings)
: Thread("Analysis Thread"), deviceManager(manager), midiOut(mout), swivelStrings(strings), console(nullptr)
{
    
}

//====================================================================================================================
void AnalysisThread::run()
{
    if (swivelStrings->size() == 0)
        throw std::invalid_argument("Can't process without any info");
    
    log("New thread started\n");
    // set up buffers
    log("Allocating buffers\n");
    audio    = (double*)       fftw_malloc(sizeof(double)*fft_size);
    spectrum = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*fft_size);
    log("Checking for saved FFT plan\n");
    bool saveWisdom = false;
    if (!fftw_import_wisdom_from_filename("./fftwisdom"))
    {
        saveWisdom = true;
        log("Not found, will save, be patient while generating plan\n");
    }
    log("Generating FFT plan\n");
    int64 time = Time::getHighResolutionTicks();
    fftw_plan plan = fftw_plan_dft_r2c_1d(fft_size, audio, spectrum, FFTW_EXHAUSTIVE);
    log(String("Generated, took: ") + String(Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()-time)) + " s\n");
    
    if (saveWisdom)
    {
        log("saving plan for later\n");
        if(!fftw_export_wisdom_to_filename("./fftwisdom"))
            log("failed writing to file\n");
    }
    
    for (int i = 0; i < swivelStrings->size(); i++) // for each string
    {
        // make sure they're good to go on the audio front
        (*swivelStrings)[i]->initialiseAudioParameters(plan, audio, spectrum, fft_size, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(), overlap);
        // probably do some MIDI work
        
        deviceManager->addAudioCallback((*swivelStrings)[i]);
        
        // somehow know when it has done its work
        sleep(2000); // wait for it
        // maybe use wait and pass the string a ref to this thread to notify when done?
        
        deviceManager->removeAudioCallback((*swivelStrings)[i]);
    }
    
    free(audio);
    free(spectrum);
}


//=====================================================================================================================
void AnalysisThread::setConsole(TextEditor* where)
{
    console = where;
}

void AnalysisThread::setFFTParams(int size, int overlap)
{
    fft_size = size;
    this->overlap = overlap; // just to be clear
    
    hop_size = fft_size/overlap;
}

//=====================================================================================================================
void AnalysisThread::log(String msg)
{
    if (console == nullptr)
        return;
    
    MessageManagerLock mml; // get a lock on the message manager thread
    console->insertTextAtCaret(msg); // write the mesage
    //std::cout << msg;
}