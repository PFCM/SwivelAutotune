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