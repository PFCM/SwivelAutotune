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
    
    //overlap
    overlapLabel = new Label("Overlap Label", "Overlap");
    overlapLabel->setEditable(false);
    overlapLabel->setBounds(310, 55, 100, 20);
    addAndMakeVisible(overlapLabel);
    overlapBox = new ComboBox("Overlap Amount");
    overlapBox->setTooltip(String("The amount of overlap between successive FFT frames.\n") +
                           "1 is no overlap, 4 means that the origin of the analysis\n" +
                           "moves by 1/4 each frame.");
    for (int i = 0; i < NUM_OVERLAPS; i++)
    {
        overlapBox->addItem(String(overlapOptions[i]), i+1);
    }
    overlapBox->setBounds(310, 80, 100, 20);
    overlapBox->setSelectedId(2);
    overlapBox->addListener(this);
    addAndMakeVisible(overlapBox);
    
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
    
    
    //=========================================================================================
    //misc
    reporter = new Reporter(swivelString, console);
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
        log("FFT Size: " + String(fft_size) + "\n", console);
    }
    else if (overlapBox == box)
    {
        overlap = overlapOptions[box->getSelectedItemIndex()];
        log("Overlap: "+ String(overlap) + "\n", console);
    }
}

void MainComponent::buttonClicked(juce::Button *button)
{
    static bool running = false;
    if (goButton == button)
    {
        if (running == false)
        {
            log("-----------------------------------------------------\n", console);
            log("---------------------BEGINNING-----------------------\n", console);
            // BEGIN
            // allocate space for audio
            log("Allocating shared buffers\n", console);
            audio = (double*) fftw_malloc(sizeof(double)*fft_size);
            log("\t"+String((uint64)sizeof(double)*fft_size) + " bytes allocated for audio\n", console);
            spectra = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*fft_size);
            log("\t"+String((uint64)sizeof(fftw_complex)*fft_size) + " bytes allocated for FFT result\n", console);
            // determine plan for FFT
            log("Calculating FFT plan\n", console);
            plan = fftw_plan_dft_r2c_1d(fft_size, audio, spectra, FFTW_MEASURE);
            log("Done\n", console);
            //initialise string objects
            log("Initialising strings\n", console);
            swivelString = new SwivelString(plan, audio, spectra, fft_size, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate(), overlap);
            // actually start process (will require midi)
            deviceManager->addAudioCallback(swivelString);
            
            running = true;
            goButton->setButtonText("STOP");
            reporter->setString(swivelString);
            reporter->setConsole(console);
            reporter->startTimer(700);
        }
        else // running must == true
        {
            log("-----------------------------------------------------\n", console);
            log("----------------------STOPPING-----------------------\n", console);
            log("-----------------------------------------------------\n", console);
            reporter->stopTimer();
            deviceManager->removeAudioCallback(swivelString);
            running = false;
            goButton->setButtonText("GO");
        }
    }
}

//===============================================================================================
void MainComponent::log(juce::String text, TextEditor* console)
{
    console->setCaretPosition(console->getText().length());
   console->insertTextAtCaret(text);
}

MainComponent::Reporter::Reporter(SwivelString* r, TextEditor* log) : swString(r), console(log)
{
    
}

void MainComponent::Reporter::setConsole(juce::TextEditor *log)
{
    console = log;
}

void MainComponent::Reporter::setString(SwivelString *str)
{
    swString = str;
}

void MainComponent::Reporter::timerCallback()
{
    Array<double>* peaksPtr = swString->getCurrentPeaksAsFrequencies();
    log("lowest: " + String((*peaksPtr)[0]) + "\n", console);
}