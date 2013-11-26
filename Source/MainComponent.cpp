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
MainComponent::MainComponent() : running(false)
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
    addAndMakeVisible(midiInLabel);
    addAndMakeVisible(midiInBox);
    
    //midi thru
    midiThroughButton = new TextButton("Start MIDI Thru", "Begins receiving MIDI and sending it through, transforming it if necessary");
    midiThroughButton->setBounds(2*getWidth()/3-100, 190, 200, 30);
    midiThroughButton->addListener(this);
    addAndMakeVisible(midiThroughButton);
    
    //=========================================================================================
    // file chooser button
    chooseFileButton = new TextButton("Choose Data File");
    chooseFileButton->setBounds(getWidth()-100, 10, 80, 20);
    chooseFileButton->addListener(this);
    addAndMakeVisible(chooseFileButton);
    
    
    //=========================================================================================
    // go button
    goButton = new TextButton("GO");
    goButton->setBounds(getWidth()/3 - 100, 190, 200, 30);
    goButton->addListener(this);
    addAndMakeVisible(goButton);
    
    //console
    console = new TextEditor("Console");
    console->setMultiLine(true);
    console->setReadOnly(true);
    console->setCaretVisible(false);
    console->setBounds(10, 220, 680, 70);
    addAndMakeVisible(console);
    
}

MainComponent::~MainComponent()
{
    // only one of these, only destroyed when application exits
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
    if (goButton == button)
    {
        if (running == false)
        {
            begin();
            running = true;
        }
        else // running must == true
        {
            endPrematurely();
            running = false;
        }
    }
    else if (chooseFileButton == button)
    {
        openFile();
    }
    else if (midiThroughButton == button)
    {
        if (button->getButtonText() == "Start MIDI Thru")
        {
            button->setButtonText("Stop MIDI Thru");
            midiInBox->addMidiInputCallback(this);
        }
        else if (button->getButtonText() == "Stop MIDI Thru")
        {
            button->setButtonText("Start MIDI Thru");
            midiInBox->removeMidiInputCallback(this);
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
    //initialise string objects
    log(" Initialising background thread\n", console);
    analysisThread = new AnalysisThread(deviceManager, midiOutBox->getSelectedOutput(), &swivelStrings, this);
    midiOutBox->getSelectedOutput()->startBackgroundThread();
#ifdef DEBUG
    analysisThread->setConsole(console);
#endif
    analysisThread->setFFTParams(fft_size, overlap);
    // BEGIN
    // MOVED THIS TO OTHER THREAD
/*    // allocate space for audio
    log("Allocating shared buffers\n", console);
    audio = (double*) fftw_malloc(sizeof(double)*fft_size);
    log("\t"+String((uint64)sizeof(double)*fft_size) + " bytes allocated for audio\n", console);
    spectra = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*fft_size);
    log("\t"+String((uint64)sizeof(fftw_complex)*fft_size) + " bytes allocated for FFT result\n", console);
    // determine plan for FFT
    log("Calculating FFT plan\n", console);
    plan = fftw_plan_dft_r2c_1d(fft_size, audio, spectra, FFTW_MEASURE);
    log("Done\n", console);
    // for each one, do some stuff
    // start listening
    for (int i = 0; i < bundles.size(); i++)
        deviceManager->addAudioCallback(swivelStrings[i]);
*/
    
    analysisThread->startThread(0);
    goButton->setButtonText("STOP");
/*    reporter->setString(swivelStrings[0]);
    reporter->setConsole(console);
    reporter->startTimer(700);
    
   // try out some MIDI
    MidiOutput* midi = midiOutBox->getSelectedOutput();
    midi->startBackgroundThread();
    midi->sendBlockOfMessages(*swivelStrings[0]->getMidiBuffer(), Time::getMillisecondCounter()+1000, deviceManager->getCurrentAudioDevice()->getCurrentSampleRate());
*/
}

void MainComponent::end(bool success)
{
    if (success)
    {
        
        log("-----------------------------------------------------\n", console);
        log("------------------------DONE-------------------------\n", console);
        log("-----------------------------------------------------\n", console);
        analysisThread->stopThread(100);
        running = false;
        goButton->setButtonText("GO");
    
        for (SwivelString*& string : swivelStrings)
        {
            if (string->isReadyToTransform())
                log("String on channel: " + String(string->getMidiChannel()) + " is at " + String(string->getBestFreq()) + "Hz\n", console);
            else
                log("String on channel: " + String(string->getMidiChannel()) + " is not ready somehow.\n", console);
        }
    }
    else
    {
        log("-----------------------------------------------------\n", console);
        log("------------STOPPING--WITH-ERROR---------------------\n", console);
        log("-----------------------------------------------------\n", console);
        goButton->setButtonText("GO");
        analysisThread->stopThread(100);
        midiOutBox->getSelectedOutput()->stopBackgroundThread();
        /*reporter->stopTimer();
     for (int i = 0; i < swivelStrings.size(); i++)
        deviceManager->removeAudioCallback(swivelStrings[i]);
     goButton->setButtonText("GO");
     MidiOutput* midi = midiOutBox->getSelectedOutput();
     midi->stopBackgroundThread();*/
    }
}

void MainComponent::endPrematurely()
{
    log("Ending, will probably cause analysis thread to crash\n", console);
    analysisThread->stopThread(1);
}


//===============================================================================================
void MainComponent::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    // if we want to print the data to the console, we are going to have to lock the message thread
#ifdef DEBUG
    const MessageManagerLock mmLock; // should do it
    const uint8* data = message.getRawData();
    log("Received MIDI: " + String(data[0]) + " " + String(data[1]) + " " + String(data[2]) + "\n", console);
    try {
#endif
    // essentially have to switch on the channel to pass it to the right string
    // would make sense to have a map of strings by channel
    // as this is potentially a bottleneck
    // depending how much midi we are piping through
    // for now the ugly linear version
    if (!analysisThread->isThreadRunning())
        for (int i = 0; i < swivelStrings.size(); i++)
        {
            if (swivelStrings[i]->getMidiChannel() == message.getChannel())
                midiOutBox->getSelectedOutput()->sendMessageNow(swivelStrings[i]->transform(message));
        }
    //midiOutBox->getSelectedOutput()->sendMessageNow(message);
        
#ifdef DEBUG
    } catch (std::logic_error const &e) {
        std::cerr << e.what();
    }
#endif
}

void MainComponent::notifyResult(juce::Result result)
{
    if (result)
    {
        end(true);
    }
    else
    {
        log(result.getErrorMessage(), console);
        end(false);
    }
}


//===============================================================================================
void MainComponent::log(juce::String text, TextEditor* console)
{
    console->setCaretPosition(console->getText().length());
    console->insertTextAtCaret(text);
}

//============FILE FUNCTIONS=====================================================================
void MainComponent::openFile()
{
    File chosen = showDialogue(String("*.xml"));
    if (chosen.existsAsFile())
    {
        try
        {
            Array<StringDataBundle*>* data = SwivelStringFileParser::parseFile(chosen);
            
            for (int i = 0; i < data->size(); i++)
            {
                StringDataBundle* bundle = data->getReference(i);
                cout << "Check data, string number: " << bundle->num << endl;
                for (int i  = 0; i < bundle->fundamentals->size(); i++)
                {
                    cout << " | Fundamental: " << (*bundle->fundamentals)[i] << endl;
                    for (int j = 0; j < (*bundle->measured_data)[i]->size(); j++)
                        cout << " | | " << (*(*bundle->measured_data)[i])[j] << endl;
                }
                cout << " | Targets\n";
                for (int i = 0; i < bundle->targets->size(); i++)
                    cout << " | | " << (*bundle->targets)[i] << endl;
                
                cout << " | Midi Pitchbend (14bit)\n";
                for (int i = 0; i < bundle->midi_pitchbend->size(); i++)
                    cout << " | | " << (int)(*bundle->midi_pitchbend)[i] <<endl;
                
                cout << " | Midi Messages\n";
                MidiMessage msg;
                int sampleoffset;
                MidiBuffer::Iterator it(*bundle->midiBuffer);
                while (it.getNextEvent(msg, sampleoffset))
                    cout << " | | " << (int)msg.getRawData()[0] << "," << (int)msg.getRawData()[1] << "," << (int)msg.getRawData()[2] << "\tTime: " << sampleoffset << endl;
            }
            
            bundles.addArray(*data);
            
            swivelStrings.clear(true);
            
            log("Initialising strings\n", console);
            for (int i = 0; i < bundles.size(); i++)
            {
                swivelStrings.add(new SwivelString());
                swivelStrings[i]->initialiseFromBundle((bundles)[i]);
            }
            
        }
        catch (SwivelStringFileParser::ParseException const &e)
        {
            log(String("Parse Error: ") + e.what()  + "\n", console);
        }
    }
    else
        log("No file chosen", console);
}

File MainComponent::showDialogue(const juce::String &pattern)
{
    FileChooser chooser("Choose file", // title
                        File::getSpecialLocation(File::userDocumentsDirectory), // directory
                        pattern); // pattern (also bool to use native dialog, default true)
    
    if (chooser.browseForFileToOpen())
    {
        return chooser.getResult();
    }
    return File();
}

//===========REPORTER MEMBER FUNCTIONS===========================================================
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
    const Array<double>* peaksPtr = swString->getCurrentPeaksAsFrequencies();
    log("lowest: " + String((*peaksPtr)[peak_index]) + "\n", console);
}