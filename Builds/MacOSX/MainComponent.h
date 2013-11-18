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

class MainComponent : public Component
{
public:
    MainComponent();
    ~MainComponent();
    
    void paint(Graphics& g);
    void resized();
    void buttonClicked(Button* button);
    
private:
    //stuff
    ScopedPointer<AudioDeviceManager> deviceManager;
    ScopedPointer<AudioDeviceSelectorComponent> audioSelector; // this exists
    // for now just one string
    ScopedPointer<SwivelString> swivelString;
    // data (this is shared between strings)
    double* audio;
    fftw_complex* spectra;
    fftw_plan plan;
    //==========================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


#endif /* defined(__SwivelAutotune__MainComponent__) */
