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
    number = number.unquoted();
    int num = number.getIntValue(); // NEED TO FIX THIS
    data->num = num;
    
    tag = file.readNextLine();
    
    while (!tag.endsWithIgnoreCase("</swivelstring>"))
    {
        tag = tag.trim();
        if (file.isExhausted())
            fail("missing end tag");
        
        if (tag.startsWith("<measurements"))
            parseMeasurements(file, data, tag);
        else if (tag.startsWith("<targets"))
            parseTargets(file, data, tag);
        else if (tag.startsWith("<midimsbs"))
            parseMidiMSBS(file, data, tag);
        else
            fail("expected 'measurements', 'targets' or 'midimsbs' tag, got: " + tag);
        
        tag = file.readNextLine();
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
    
}

void SwivelStringFileParser::parseTargets(juce::BufferedInputStream &file, SwivelStringFileParser::StringDataBundle *data, juce::String tag)
{
    
}

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
    split_recurse(s.substring(index+1,s.length()-1), pattern, result);
}

void SwivelStringFileParser::fail(juce::String msg)
{
    throw ParseException(msg);
}