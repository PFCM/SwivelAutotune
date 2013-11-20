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
 SWIVELSTRING   ::= '<swivelstring ' S NUMATT S '>' S MEASUREMENTS+ S TARGETS S MSBS S '</swivelstring>'
 NUMATT         ::= 'number=' NUMBER
 MEASUREMENTS   ::= '<measurements' S FUND S '>' FLOATLIST '</measurement>'
 TARGETS        ::= '<targets>' S FLOATLIST S '</targets>'
 MSBS           ::= '<midimsbs>' BYTELIST '</midimsbs>'
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
        Array<Array<double>*>* measured_data;
        /** The fundamental of the string, indices corresponding to measured_data */
        Array<double>* fundamentals;
        /** The target frequencies */
        Array<double>* targets;
        /** The pitch-bend MSBs that should produce these targets */
        Array<uint8>* midi_msbs;
        
        StringDataBundle()
        {
            measured_data = new Array<Array<double>*>();
            fundamentals = new Array<double>();
            targets = new Array<double>();
            midi_msbs = new Array<uint8>();
        }
    };
    
    /** Parses the file, returns the data if it succeeds.
     *  Should hopefully print out some meaningful errors if it doesn't (to std::cerr most likely).
     */
    static StringDataBundle* parseFile(const File& f);
    
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
    static void fail(String msg);
    static StringArray split(String s, String pattern);
    static void split_recurse(String s, String pattern, StringArray result);
    
};

#endif /* defined(__SwivelAutotune__SwivelStringFileParser__) */
