//
//  SwivelStringFileParser.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 20/11/13.
//
//

#include "SwivelStringFileParser.h"
#include <regex>


Array<SwivelStringFileParser::StringDataBundle*>* SwivelStringFileParser::parseFile(const juce::File& f)
{
    // first we need to turn the file into a useable stream of data
    ScopedPointer<FileInputStream> file = new FileInputStream(f);
    // and now we can parse it
    if (file->failedToOpen())
    {
        std::cerr << "Failed to open file " + f.getFileName() << std::endl;
        return nullptr;
    }
    
    
    ScopedPointer<BufferedInputStream> stream = (new BufferedInputStream(file.release(), 256, true)); // need to make sure this doesn't leak
    
    Array<StringDataBundle*>* data = new Array<StringDataBundle*>();
    
    while (!stream->isExhausted())
    {
        StringDataBundle* bundle = new StringDataBundle();
        parseSwivelStringElement(*stream, bundle);
        data->add(bundle);
    }
    return data;
}


void SwivelStringFileParser::parseSwivelStringElement(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data)
{
    String tag = file.readNextLine();
    if (!tag.startsWith("<swivelstring"))
    {
        fail("expected file to start with '<swivelstring'");
    }
    
    int nindex = tag.indexOfWholeWord("number=");
    
    if (nindex <= 0)
    {
        fail("missing number attribute");
    }
    
    String number = tag.substring(nindex+7); // should be the number and the end of the tag
    number = trimToNumber(number);
    int num = number.getIntValue();
    data->num = num; // NOTE - ERRORS HERE IN THE FILE WILL NOT CRASH THE PARSER, THE STRING WILL JUST HAVE VALUE ONE
    
    tag = file.readNextLine();
    
    while (!tag.endsWithIgnoreCase("</swivelstring>"))
    {
        tag = tag.trim();
        if (file.isExhausted())
            fail("missing '</swivelstring>' end tag");
        
        if (tag.startsWith("<measurements"))
            parseMeasurements(file, data, tag);
        else if (tag.startsWith("<targets"))
            parseTargets(file, data, tag);
        else if (tag.startsWith("<midimsbs"))
            parseMidiMSBS(file, data, tag);
        else if (tag.startsWith("<midimessages"))
            parseMidiMsgs(file, data, tag);
        else
            fail("expected 'measurements', 'targets', 'midimsbs' or 'midimessages' tag, got: " + tag);
        
        tag = file.readNextLine();
    }
    
    // error checking
    if (data->midi_pitchbend->size() == 0)
        fail("Did not find any '<midimsbs>");
    if (data->measured_data->size() < 2)
        fail("Need at least two '<measurements>' to work, found " + String(data->measured_data->size()));
    if (data->fundamentals->size() != data->measured_data->size())
        fail("Ended up with a different number of 'fundamental' attributes to the number of '<measurements>'");
    if (data->targets->size() == 0)
        fail("Did not find any '<targets>'");
    if (data->midiBuffer->isEmpty())
        fail("No '<midimessages>' found, how exactly did you propose to make sound?");
}

void SwivelStringFileParser::parseMeasurements(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    int findex = tag.indexOfWholeWord("fundamental="); // IF THIS IS -1, trimToNumber ENSURES WE STILL ACTUALLY GET A RESULT AS LONG AS THE NUMBER IS THERE
    String attr = tag.substring(findex+12);
    // remove quotes and >
    // using built in string methods does not in fact remove anything
    // so have to do it by hand
    attr = trimToNumber(attr);
    double num = attr.getDoubleValue();
    // 0 has to be an illegal value because it is also the result of an unsuccessful parse
    if (num == 0)
        fail("Couldn't find a number for the fundamental. Or it is 0");
    data->fundamentals->add(num);
    
    // now grab the list
    String line = file.readNextLine();
    StringArray splitLine = split(line.trim(), ",");
    
    Array<double>* array = new Array<double>();
    
    for (int i = 0; i < splitLine.size(); i++)
        array->add(splitLine[i].getDoubleValue());
    
    data->measured_data->add(array);
    
    // make sure end tag is present
    String end = file.readNextLine();
    if (end.trim() != "</measurements>")
        fail("expected '</measurements>', got: " + end);
}

void SwivelStringFileParser::parseMidiMSBS(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    if (data->midi_pitchbend->size() != 0) fail("probably more than one <midimsbs> tag in the file");
    String line = file.readNextLine();
    StringArray splitLine = split(line, ",");
    
    // NOTE
    // IF THE NUMBERS ARE MISSING COMMAS OR TOO LARGE ETC, THEY
    // WILL JUST WRAP AROUND 255, MIGHT BE TOUGH TO SPOT
    for (int i = 0; i < splitLine.size(); i++)
        data->midi_pitchbend->add((uint16)(splitLine[i].getIntValue()<<7));
    
    line = file.readNextLine();
    line = line.trim();
    if (line != "</midimsbs>")
        fail("expected '</midimsbs>', got: " + line);
}

void SwivelStringFileParser::parseTargets(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    if (data->targets->size() != 0) fail("probably more than one '<targets>' tag in the file");
    String line = file.readNextLine();
    StringArray splitLine = split(line, ",");
    
    for (int i = 0; i < splitLine.size(); i++)
        data->targets->add(splitLine[i].getDoubleValue());
    
    line = file.readNextLine();
    line = line.trim();
    if (line != "</targets>")
        fail("expected '</targets>', got: " + line);
}

void SwivelStringFileParser::parseMidiMsgs(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    if (!data->midiBuffer->isEmpty())
        std::cerr << "More than one '<midimessages>' found on a particular string, might want to double check\n";
    
    String line = file.readNextLine();
    line = line.trim();
    while (line != "</midimessages>")
    {
        // actually read the message
        if (!line.startsWith("<message"))
            fail("Expected a MIDI message but did not find a tag starting with <message");
        
        int tindex = line.indexOf("time");
        
        String time = line.substring(tindex+4);
        time = trimToNumber(time);
        int timestamp = time.getFloatValue()*44100; // TODO not hardcode the sample rate
        line = file.readNextLine(); // should be the list of bytes
        line = line.trim();
        data->midiBuffer->addEvent(midiMessageFromString(line), timestamp);
        line = file.readNextLine();
        line = line.trim();
        if (!line.equalsIgnoreCase("</message>"))
            fail("Missing </message> at the end of MIDI message");
        // advance through the file
        line = file.readNextLine();
        line = line.trim();
    }
}

//=======================================STRING UTILITIES=======================================================

StringArray SwivelStringFileParser::split(juce::String s, juce::String pattern)
{
    StringArray array;
    
    split_recurse(s, pattern, array);
    
    return array;
}

void SwivelStringFileParser::split_recurse(juce::String s, juce::String& pattern, juce::StringArray& result)
{
    if (s.length() == 0) return;
    
    int index = s.indexOf(pattern);
    
    if (index <= 0)
    {
        result.add(s);
        return;
    }
    
    result.add(s.substring(0, index));
    split_recurse(s.substring(index+1,s.length()), pattern, result);
}

String SwivelStringFileParser::trimToNumber(juce::String &s)
{
    int start = 0;
    while (isNotANumber(s.getCharPointer()[start]))
        start++;
    
    int end = s.length()-1;
    while (isNotANumber(s.getCharPointer()[end]))
        end--;
    
    return s.substring(start, end+1); // need to add one to the end because it is not inclusive
}

bool SwivelStringFileParser::isNotANumber(juce_wchar s)
{
    return !(CharacterFunctions::isDigit(s));
}

MidiMessage SwivelStringFileParser::midiMessageFromString(juce::String &info)
{
    StringArray data = split(info.trim(), ",");
    uint8 bytes[4];
    for (int i  = 0; i < data.size(); i++)
    {
        String b = data[i];
        bytes[i] = (b.getIntValue());
    }
    
    return MidiMessage(bytes, data.size()); // TODO will this work?
}

//=======================FAIL CODE===============================================================================

void SwivelStringFileParser::fail(juce::String msg)
{
    throw ParseException(msg);
}