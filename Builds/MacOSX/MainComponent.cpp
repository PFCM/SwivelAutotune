//
//  MainComponent.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#include "MainComponent.h"

//==============================================================================================
MainComponent::MainComponent()
{
    setSize(300, 300);
    deviceManager = new AudioDeviceManager();
    deviceManager->initialise(2, 0, nullptr, true);
    audioSelector = new AudioDeviceSelectorComponent(*deviceManager,
                                                     2, 2, //input
                                                     0, 0, //output
                                                     false,
                                                     false,
                                                     true,
                                                     true);
    audioSelector->setBounds(0, 10, 300, 300);
    addAndMakeVisible(audioSelector);
}

MainComponent::~MainComponent()
{
    // clean up
    deviceManager = 0;
    audioSelector = 0;
}

//==============================================================================================
void MainComponent::paint(juce::Graphics &g)
{
    
}

void MainComponent::resized()
{
    
}