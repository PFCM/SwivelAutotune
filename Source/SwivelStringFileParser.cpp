//
//  SwivelStringFileParser.cpp
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 20/11/13.
//
//

#include "SwivelStringFileParser.h"
#include <regex>

using namespace std;

SwivelStringFileParser::StringDataBundle* SwivelStringFileParser::parseFile(const juce::File& f)
{
    // first we need to turn the file into a useable stream of data
    FileInputStream file(f);
    // and now we can parse it
    if (file.failedToOpen())
    {
        std::cerr << "Failed to open file " + f.getFileName() << std::endl;
        return nullptr;
    }
    
    StringDataBundle* data = new StringDataBundle();
    
    BufferedInputStream* stream = (new BufferedInputStream(&file, 256, true));
    
    parseSwivelStringElement(*stream, data);
    
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
    number = number.trimCharactersAtEnd(">");
    data->num = number.getIntValue();
    
    tag = file.readNextLine();
    
    while (!tag.endsWithIgnoreCase("</swivelstring>"))
    {
        if (file.isExhausted())
            fail("missing end tag");
        
        if (tag.startsWith("<measurements"))
            parseMeasurements(file, data, tag);
        else if (tag.startsWith("<targets"))
            parseTargets(file, data, tag);
        else if (tag.startsWith("<midimsbs"))
            parseMidiMSBS(file, data, tag);
    }
}

void SwivelStringFileParser::parseMeasurements(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    int findex = tag.indexOfWholeWord("fundamental=");
    String attr = tag.substring(findex+12);
    attr.trimCharactersAtEnd(">");
    data->fundamentals->add(attr.getDoubleValue());
    
    // now grab the list
    String line = file.readNextLine();
    StringArray splitLine = split(line, ",");
    
    Array<double>* array = new Array<double>();
    
    for (int i = 0; i < splitLine.size(); i++)
        array->add(splitLine[i].getDoubleValue());
}

StringArray SwivelStringFileParser::split(juce::String s, juce::String pattern)
{
    StringArray array;
    
    split_recurse(s, pattern, array);
    
    return array;
}

void SwivelStringFileParser::split_recurse(juce::String s, juce::String pattern, juce::StringArray result)
{
    if (s.length() == 0) return;
    
    result.add(s.substring(0, s.indexOf(pattern)));
    split_recurse(s.fromFirstOccurrenceOf(pattern, false, true), pattern, result);
}

void SwivelStringFileParser::fail(juce::String msg)
{
    throw ParseException(msg);
}