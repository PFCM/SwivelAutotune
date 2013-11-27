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

/** A ComboBox that populates isself with available MIDI input devices */
class MidiInputDeviceSelector : public ComboBox,
                                private ComboBox::Listener, MidiInputCallback
{
public:
    MidiInputDeviceSelector(String name);
    
    /** Returns a reference to the selected MidiInput.
        Probably should not use this directly, it would be safer to register
        a callback with this object.
    */
    MidiInput* getSeletedInput();
    
    void comboBoxChanged(ComboBox* changed);
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage &message);
    void addMidiInputCallback(MidiInputCallback* newCallback);
    void removeMidiInputCallback(MidiInputCallback* oldCallback);
    
private:
    // current device
    ScopedPointer<MidiInput> inputDevice;
    Array<MidiInputCallback*> callbacks;
};

#endif /* defined(__SwivelAutotune__MidiDeviceSelector__) */
