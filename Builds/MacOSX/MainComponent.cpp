//
//  MainComponent.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#include "MainComponent.h"
#define FFT_SIZE 8192

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
    
    
    //==========================================================================================
    // fft
    audio = (double*) fftw_malloc(sizeof(double)*FFT_SIZE);
    spectra = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*FFT_SIZE);
    plan = fftw_plan_dft_r2c_1d(FFT_SIZE, audio, spectra, FFTW_MEASURE);
    
    swivelString = new SwivelString(plan, audio, spectra, FFT_SIZE);
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