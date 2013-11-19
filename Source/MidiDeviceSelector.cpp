//
//  MidiDeviceSelector.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 19/11/13.
//
//

#include "MidiDeviceSelector.h"


//============MidiOutputDeviceSelector===========================================
MidiOutputDeviceSelector::MidiOutputDeviceSelector(String name) : ComboBox(name)
{
    // populate self with available devices
    StringArray options = MidiOutput::getDevices();
    addItemList(options, 1);
    addListener(this);
    setSelectedItemIndex(MidiOutput::getDefaultDeviceIndex());
}

void MidiOutputDeviceSelector::comboBoxChanged(juce::ComboBox *changed)
{
    if (outputDevice!=nullptr)
    {
        delete outputDevice;
    }
    
    outputDevice = MidiOutput::openDevice(changed->getSelectedItemIndex());
}

MidiOutput* MidiOutputDeviceSelector::getSelectedOutput()
{
    return outputDevice.get();
}

//=============MidiInputDeviceSelector============================================
MidiInputDeviceSelector::MidiInputDeviceSelector(String name) : ComboBox(name)
{
    // get list of devices
    StringArray devices = MidiInput::getDevices();
    addItemList(devices, 1);
    addListener(this);
    setSelectedItemIndex(MidiInput::getDefaultDeviceIndex());
}

void MidiInputDeviceSelector::comboBoxChanged(juce::ComboBox *changed)
{
    if (inputDevice != nullptr)
        delete inputDevice;
    
    inputDevice = MidiInput::openDevice(changed->getSelectedItemIndex(), this);
}

void MidiInputDeviceSelector::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    for (int i = 0; i < callbacks.size(); i++)
        callbacks[i]->handleIncomingMidiMessage(source, message);
}

void MidiInputDeviceSelector::addMidiInputCallback(MidiInputCallback* callback)
{
    callbacks.addIfNotAlreadyThere(callback);
}

void MidiInputDeviceSelector::removeMidiInputCallback(juce::MidiInputCallback *oldCallback)
{
    callbacks.removeFirstMatchingValue(oldCallback);
}