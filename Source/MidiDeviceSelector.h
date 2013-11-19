//
//  MidiDeviceSelector.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 19/11/13.
//
//

#ifndef __SwivelAutotune__MidiDeviceSelector__
#define __SwivelAutotune__MidiDeviceSelector__

#include "../JuceLibraryCode/JuceHeader.h"

/** A ComboBox that populates itself with available MIDI output devices */
class MidiOutputDeviceSelector : public ComboBox,
                                 private ComboBox::Listener
{
public:
    MidiOutputDeviceSelector(String name);
    
    /** Returns a reference to the selected MidiOutput.
        It is still owned by this class and will be cleaned up by
        this class. */
    MidiOutput* getSelectedOutput();
    
    void comboBoxChanged(ComboBox* changed);
    
private:
    /** The current output device */
    ScopedPointer<MidiOutput> outputDevice;
};

#endif /* defined(__SwivelAutotune__MidiDeviceSelector__) */
