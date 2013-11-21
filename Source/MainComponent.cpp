//
//  MainComponent.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#include "MainComponent.h"
#include "SwivelStringFileParser.h"

using namespace std;

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
    overlapBox->setBounds(310, 75, 100, 20);
    overlapBox->setSelectedId(2);
    overlapBox->addListener(this);
    addAndMakeVisible(overlapBox);
    
    // windowing
    windowLabel = new Label("Windowing", "Windowing");
    windowLabel->setBounds(310, 100, 100, 20);
    addAndMakeVisible(windowLabel);
    windowBox = new ComboBox("Window Box");
    windowBox->addItem("Hann", WindowType::HANN);
    windowBox->addItem("Hamming", WindowType::HAMMING);
    windowBox->addItem("Blackman", WindowType::BLACKMAN);
    windowBox->addItem("Rectangle", WindowType::RECTANGULAR);
    windowBox->setSelectedId(WindowType::HANN);
    windowBox->setBounds(310, 120, 100, 20);
    windowBox->setSelectedId(WindowType::HANN);
    windowBox->addListener(this);
    addAndMakeVisible(windowBox);
    
    
    //========================================================================================
    // midi out
    midiOutBox = new MidiOutputDeviceSelector("Midi Out Box");
    midiOutBox->setBounds(420, 30, 100, 20);
    addAndMakeVisible(midiOutBox);
    midiOutLabel = new Label("Midi out label", "MIDI Out: ");
    midiOutLabel->setBounds(420, 10, 100, 20);
    addAndMakeVisible(midiOutLabel);
    
    //midi in
    midiInLabel = new Label("Midi in label", "MIDI In: ");
    midiInLabel->setBounds(420, 55, 100, 20);
    midiInBox = new MidiInputDeviceSelector("Midi In Box");
    midiInBox->setBounds(420, 75, 100, 20);
    midiInBox->addMidiInputCallback(this);
    addAndMakeVisible(midiInLabel);
    addAndMakeVisible(midiInBox);
    
    //=========================================================================================
    // file chooser button
    chooseFileButton = new TextButton("Choose Data File");
    chooseFileButton->setBounds(getWidth()-100, 10, 80, 20);
    chooseFileButton->addListener(this);
    addAndMakeVisible(chooseFileButton);
    
    
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
    for (int i = 0; i < 6; i++)
        stringData.add(new XmlElement("null"));
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
    else if (windowBox == box)
    {
        window = (WindowType)windowBox->getSelectedId();
        String message = "Window: ";
        
        switch (window) {
            case HANN:
                message += "Hann.\n";
                break;
            case BLACKMAN:
                message += "Blackman.\n";
                break;
            case HAMMING:
                message += "Hamming.\n";
                break;
            case RECTANGULAR:
                message += "Rectangle.\n";
                break;
                
            default:
                message += "Unknown. Be worried.\n";
                break;
        }
    }
}

void MainComponent::buttonClicked(juce::Button *button)
{
    static bool running = false;
    if (goButton == button)
    {
        if (running == false)
        {
            begin();
            running = true;
        }
        else // running must == true
        {
            end();
            running = false;
        }
    }
    else if (chooseFileButton == button)
    {
        FileChooser chooser("Choose XML data file", // title
                            File::getSpecialLocation(File::userHomeDirectory), // directory
                            "*.xml"); // pattern (also bool to use native dialog, default true)
        
        if (chooser.browseForFileToOpen())
        {
            File chosen (chooser.getResult());
            try
            {
            ScopedPointer<SwivelStringFileParser::StringDataBundle> bundle = SwivelStringFileParser::parseFile(chosen);
            cout << "Check data, string number: " << bundle->num << endl;
            for (int i  = 0; i < bundle->fundamentals->size(); i++)
            {
                cout << "\tFundamental: " << (*bundle->fundamentals)[i] << endl;
                for (int j = 0; j < (*bundle->measured_data)[i]->size(); j++)
                    cout << "\t\t" << (*(*bundle->measured_data)[i])[j] << endl;
            }
            cout << "\tTargets\n";
            for (int i = 0; i < bundle->targets->size(); i++)
                cout << "\t\t" << (*bundle->targets)[i] << endl;
            
            cout << "\tMidi MSBS\n";
            for (int i = 0; i < bundle->midi_msbs->size(); i++)
                cout << "\t\t" << (int)(*bundle->midi_msbs)[i] <<endl;
            }
            catch (SwivelStringFileParser::ParseException &e)
            {
                log(String("Parse Error: ") + e.what() + "\n", console);
            }
        }
    }
}

//===============================================================================================
// the all important

void MainComponent::begin()
{
    
    log("-----------------------------------------------------\n", console);
    log("---------------------BEGINNING-----------------------\n", console);
    log("-----------------------------------------------------\n", console);
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
    
    goButton->setButtonText("STOP");
    reporter->setString(swivelString);
    reporter->setConsole(console);
    reporter->startTimer(700);
    
    // try out some MIDI
    
    double sr = deviceManager->getCurrentAudioDevice()->getCurrentSampleRate();
    MidiBuffer messages;
    for (int i =0; i < 10; i++)
    {
        messages.addEvent(MidiMessage(146,i,i), i*(int)sr);
    }
    MidiOutput* midi = midiOutBox->getSelectedOutput();
    midi->startBackgroundThread();
    midi->sendBlockOfMessages(messages, Time::getMillisecondCounter()+1000, sr);
}

void MainComponent::end()
{
    log("-----------------------------------------------------\n", console);
    log("----------------------STOPPING-----------------------\n", console);
    log("-----------------------------------------------------\n", console);
    reporter->stopTimer();
    deviceManager->removeAudioCallback(swivelString);
    goButton->setButtonText("GO");
    MidiOutput* midi = midiOutBox->getSelectedOutput();
    midi->stopBackgroundThread();
}

//===============================================================================================
void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    // if we want to print the data to the console, we are going to have to lock the message thread
    const MessageManagerLock mmLock; // should do it
    const uint8* data = message.getRawData();
    log("Received MIDI: " + String(data[0]) + " " + String(data[1]) + " " + String(data[2]) + "\n", console);
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
    int peak_index = 1;
    Array<double>* peaksPtr = swString->getCurrentPeaksAsFrequencies();
    log("lowest: " + String((*peaksPtr)[peak_index]) + "\n", console);
}