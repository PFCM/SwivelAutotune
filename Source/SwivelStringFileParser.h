//
//  SwivelStringFileParser.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 20/11/13.
//
//

#ifndef __SwivelAutotune__SwivelStringFileParser__
#define __SwivelAutotune__SwivelStringFileParser__

#include "../JuceLibraryCode/JuceHeader.h"
#include <regex>

/*************************************************************
 Parses data files for the strings.
 The files are expected to follow the following grammar.
 Line separation of the file is tremendously important,
 see examples.
 
 FILE           ::= SWIVELSTRING
 SWIVELSTRING   ::= '<swivelstring ' S NUMATT S '>' S MEASUREMENTS+ S TARGETS S MSBS S MIDIMSGS S '</swivelstring>'
 NUMATT         ::= 'number=' NUMBER
 MEASUREMENTS   ::= '<measurements' S FUND S '>' FLOATLIST '</measurement>'
 TARGETS        ::= '<targets>' S FLOATLIST S '</targets>'
 MSBS           ::= '<midimsbs>' BYTELIST '</midimsbs>'
 MIDIMSGS       ::= '<midimessages>' MMSG+ '</midimessage>'
 MMSG           ::= '<message' S TIME S '>' BYTELIST '</message>'     // note the list of bytes needs to represent a valid MIDI message
 FLOATLIST      ::= comma separated list of numbers with optional decimal points and fractional parts
 BYTELIST       ::= comma separated list of numbers between 0 and 255
 NUMBER         ::= "[0-9]"
 S              ::= "\s"
 
 *************************************************************/

class SwivelStringFileParser
{
public:
    /** For transporting the data found in the files */
    struct StringDataBundle
    {
        /** The number of the string, used to tell them apart */
        int num;
        /** The measurements of the string at various frequencies */
        ScopedPointer<OwnedArray<Array<double>>> measured_data;
        /** The fundamental of the string, indices corresponding to measured_data */
        ScopedPointer<Array<double>> fundamentals;
        /** The target frequencies */
        ScopedPointer<Array<double>> targets;
        /** The pitch-bend MSBs that should produce these targets */
        ScopedPointer<Array<uint8>> midi_msbs;
        /** The sequence of MIDI messages needed to make the sounds required */
        ScopedPointer<MidiBuffer> midiBuffer;
        
        StringDataBundle()
        {
            measured_data = new OwnedArray<Array<double>>();
            fundamentals = new Array<double>();
            targets = new Array<double>();
            midi_msbs = new Array<uint8>();
            midiBuffer = new MidiBuffer();
        }
    };
    
    /** Parses the file, returns the data if it succeeds.
     *  Should hopefully print out some meaningful errors if it doesn't (to std::cerr most likely).
     */
    static Array<StringDataBundle*>* parseFile(const File& f);
    
    class ParseException : public std::exception
    {
    public:
        ParseException(String message) : msg(message) {};
        const char* what() const noexcept { return msg.getCharPointer(); };
            
    private:
        String msg;
    };
    
private:
    static void parseSwivelStringElement(BufferedInputStream& file, StringDataBundle* data);
    static void parseMeasurements(BufferedInputStream& file, StringDataBundle* data, String tag);
    static void parseTargets(BufferedInputStream& file, StringDataBundle* data, String tag);
    static void parseMidiMSBS(BufferedInputStream& file, StringDataBundle* data, String tag);
    static void parseMidiMsgs(BufferedInputStream& file, StringDataBundle* data, String tag);
    static void fail(String msg);
    
    
    // utilities
    /** Splits a string by occurrences of the given pattern.
     *  Does so recursively, so a particularly long string might cause some issues. */
    static StringArray split(String s, String pattern);
    /** internal recursive method for splitting the string, don't call from outside */
    static void split_recurse(String s, String& pattern, StringArray& result);
    /** trims characters that aren't numeric from either side of a string */
    static String trimToNumber(String &s);
    /** Returns true if the character is not a number (including if it is a decimal point */
    static bool isNotANumber(juce_wchar s);
    /** Will return a MidiMessage from a string containing a (comma separated) list of numbers */
    static MidiMessage midiMessageFromString(String& info);
    
};

#endif /* defined(__SwivelAutotune__SwivelStringFileParser__) */
