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
    setSize(700, 300);
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
    fftLabel = new Label("FFT Label", "FFT Size");
    fftLabel->setEditable(false);
    fftLabel->setBounds(310, 10, 100, 20);
    addAndMakeVisible(fftLabel);
    fftSizeBox = new ComboBox("FFT Size");
    for (int i =0; i < NUM_FFT_SIZES; i++)
    {
        fftSizeBox->addItem(String(FFTSizes[i]), i+1);
    }
    fftSizeBox->setBounds(310, 30, 100, 20);
    fftSizeBox->setSelectedId(5);
    fftSizeBox->addListener(this);
    addAndMakeVisible(fftSizeBox);
    
    //=========================================================================================
    // go button
    goButton = new TextButton("GO");
    goButton->setBounds(getWidth()/2 - 100, 200, 200, 30);
    goButton->addListener(this);
    addAndMakeVisible(goButton);
    
    //console
    console = new TextEditor("Console");
    console->setMultiLine(true);
    console->setReadOnly(true);
    console->setCaretVisible(false);
    console->setBounds(10, 240, 680, 50);
    addAndMakeVisible(console);
    
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

//===============================================================================================
void MainComponent::comboBoxChanged(juce::ComboBox *box)
{
    if (fftSizeBox == box)
    {
        fft_size = FFTSizes[box->getSelectedItemIndex()];
        log("FFT Size: " + String(fft_size) + "\n");
    }
}

void MainComponent::buttonClicked(juce::Button *button)
{
    if (goButton == button)
    {
        log("-----------------------------------------------------\n");
        log("                     BEGINNING                       \n");
        // BEGIN
        // allocate space for audio
        log("Allocating shared buffers\n");
        audio = (double*) fftw_malloc(sizeof(double)*fft_size);
        log("\t"+String((uint64)sizeof(double)*fft_size) + " bytes allocated for audio\n");
        spectra = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*fft_size);
        log("\t"+String((uint64)sizeof(fftw_complex)*fft_size) + " bytes allocated for FFT result\n");
        // determine plan for FFT
        log("Calculating FFT plan\n");
        plan = fftw_plan_dft_r2c_1d(fft_size, audio, spectra, FFTW_MEASURE);
        log("Done\n");
        //initialise string objects
        log("Initialising strings\n");
        swivelString = new SwivelString(plan, audio, spectra, fft_size);
        
        // actually start process (will require midi)
    }
}

//===============================================================================================
void MainComponent::log(juce::String text)
{
    console->setCaretPosition(console->getText().length());
    console->insertTextAtCaret(text);
}