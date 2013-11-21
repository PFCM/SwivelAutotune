//
//  AnalysisThread.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 22/11/13.
//
//

#include "AnalysisThread.h"

AnalysisThread::AnalysisThread(AudioDeviceManager *manager, MidiOutput *mout, OwnedArray<SwivelString> *strings)
: Thread("Analysis Thread"), deviceManager(manager), midiOut(mout), swivelStrings(strings), console(nullptr)
{
    
}

void AnalysisThread::run()
{
    for (int i = 0; i < swivelStrings->size; i++) // for each string
    {
        // probably do some MIDI work
        
        
    }
}

void AnalysisThread::setConsole(TextEditor* where)
{
    console = where;
}

void AnalysisThread::log(String msg)
{
    if (console == nullptr)
        return;
    
    MessageManagerLock mml; // get a lock on the message manager thread
    console->insertTextAtCaret(msg); // write the mesage
}