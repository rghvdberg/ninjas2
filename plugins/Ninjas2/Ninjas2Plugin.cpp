/*
 * Copyright (C) 2018-2019 Rob van den Berg <rghvdberg at gmail dot org>
 *
 * This file is part of Ninjas2
 *
 * Nnjas2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ninjas2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ninjas2.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "DistrhoPlugin.hpp"
#include "Ninjas2Plugin.hpp"
#include <sndfile.hh>
#include <vector>
#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <samplerate.h>
#include "aubio.h"
#include <stdio.h>
#include <cstdlib>
//#include <cerrno>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "minimp3_ex.h"

#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------


// constructor
NinjasPlugin::NinjasPlugin()
    : Plugin ( paramCount, 0, stateCount ) // parameters, programs (presets) , states
{
    // pitchbend
    pitchbend       = 8192 ;  // center
    pitchbendDepth = 24 ;
    pitchbendStep  = 16384.0f / pitchbendDepth;

    //
    gain = 1.0f;
    filepath = "";
    bypass   = true;

    // slices
    slicemode    = RAW;
    sliceSensitivity = 0.5;

    samplerate = getSampleRate();
    sampleChannels = 1;
    sampleSize = 0;

    std::fill_n ( voices,128,Voice() );
    
    // Program Grid
    std::fill_n ( p_Grid, 16, 0 );
    p_Grid[0] = 1;
    programNumber = 0;
    programGrid=0;
    
    sig_SampleLoaded = false;
    sig_LoadProgram = false;
    sig_currentSlice = -1;
    
    // init mixer
    mixer.leftChannel = 0.0f;
    mixer.rightChannel = 0.0f;
    mixer.samples = 0;

    initPrograms();
    //for debugging , autoload sample
    /*
    filepath = "/home/rob/git/ninjas2/plugins/Ninjas2/Drumloop4.wav";
    loadSample ( filepath, true );

    getOnsets ();
    createSlicesRaw();
    bypass = false;
    */
}

// Init

void NinjasPlugin::initParameter ( uint32_t index, Parameter& parameter )
{
    switch ( index ) {
    case paramProgramNumber: {
        parameter.hints = kParameterIsOutput;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 15.0f;
        parameter.name   = "programNumber (output)";
        parameter.symbol  = "programNumberOut";
        break;
    }
    case paramSigSampleLoaded: {
        parameter.hints = kParameterIsOutput|kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "sampleLoaded";
        parameter.symbol = "sampleLoaded";
        break;
    }
    case paramSigLoadProgram: {
        parameter.hints = kParameterIsOutput|kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "sigLoadProgram";
        parameter.symbol = "sigLoadProgram";
        break;
    }
    case paramCurrentSlice: {
        parameter.hints = kParameterIsOutput|kParameterIsInteger;
        parameter.ranges.def = -1.0f;
        parameter.ranges.min = -1.0f;
        parameter.ranges.max = 127.0f;
        parameter.name   = "Current Slice (output)";
        parameter.symbol  = "currentSlice";
        break;
    }
    case paramNumberOfSlices: {
        parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 1.0f;
        parameter.ranges.max = 128.0f;
        parameter.name   = "Slices";
        parameter.symbol  = "number_of_slices";
        parameter.midiCC = 102;
        break;
    }
    case paramAttack: {
        parameter.hints      = kParameterIsAutomable ;
        parameter.ranges.def = 0.001f;
        parameter.ranges.min = 0.001f;
        parameter.ranges.max = 1.0f;
        parameter.name   = "Attack";
        parameter.symbol = "attack";
        parameter.midiCC = 103;
        break;
    }
    case paramDecay: {
        parameter.hints      = kParameterIsAutomable ;
        parameter.ranges.def = 0.001f;
        parameter.ranges.min = 0.001f;
        parameter.ranges.max = 1.0f;
        parameter.name   = "Decay";
        parameter.symbol =  "decay";
        parameter.midiCC = 104;
        break;
    }
    case paramSustain: {
        parameter.hints      = kParameterIsAutomable ;
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "Sustain";
        parameter.symbol = "sustain";
        // parameter.midiCC = 105;
        break;
    }
    case paramRelease: {
        parameter.hints      = kParameterIsAutomable ;
        parameter.ranges.def = 0.001f;
        parameter.ranges.min = 0.001f;
        parameter.ranges.max = 1.0f;
        parameter.name   = "Release";
        parameter.symbol = "release";
        parameter.midiCC = 106;
        break;
    }
    case paramPlayMode: {
        parameter.hints = kParameterIsAutomable|kParameterIsInteger;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 3.0f;
        parameter.enumValues.count = 4;
        parameter.enumValues.restrictedMode = true;
        parameter.enumValues.values = new ParameterEnumerationValue[4] {
            ParameterEnumerationValue ( 0.0f,"One Shot Forward" ),
            ParameterEnumerationValue ( 1.0f,"One Shot Reverse" ),
            ParameterEnumerationValue ( 2.0f,"Loop Forward" ),
            ParameterEnumerationValue ( 3.0f,"Loop Reverse" )
        };

        parameter.name = "Play Mode";
        parameter.symbol = "playmode";
        break;
    }
    case paramLoadSample: {
        parameter.hints = kParameterIsAutomable|kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name   = "Load Sample";
        parameter.symbol  = "load";
        break;
    }
    case paramSliceMode: {
        parameter.hints = kParameterIsAutomable|kParameterIsInteger;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name   = "Slice Mode";
        parameter.symbol  = "slicemode";
        parameter.midiCC = 111;
        break;
    }
    case paramSliceSensitivity: {
        parameter.hints = kParameterIsAutomable;
        parameter.ranges.def = 0.5f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "Slice Onset Detection Sensitivity";
        parameter.symbol = "sliceSensitivity";
        break;
    }
    case paramProgramGrid: {
        parameter.hints = kParameterIsInteger|kParameterIsInteger; // FIXME not automate ?? is this the correct way of operation
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 65535.0f; // 16 bits
        parameter.name = "Program Grid";
        parameter.symbol = "programGrid";
        break;
    }
    case paramPitchbendDepth: {
        parameter.hints = kParameterIsAutomable|kParameterIsInteger;
        parameter.ranges.def = 12.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 12.0f;
        parameter.name = "Pitchbend Depth";
        parameter.symbol = "pitchbendDepth";
        break;
    }
    case paramOneShotForward: {
        parameter.hints = kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "One Shot Forward";
        parameter.symbol = "OneShotForward";
        break;
    }
    case paramOneShotReverse: {
        parameter.hints = kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "One Shot Reverse";
        parameter.symbol = "OneShotReverse";
        break;
    }
    case paramLoopForward: {
        parameter.hints = kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "Loop Forward";
        parameter.symbol = "LoopForward";
        break;
    }
    case paramLoopReverse: {
        parameter.hints = kParameterIsBoolean;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.name = "Loop Reverse";
        parameter.symbol = "LoopReverse";

        break;
    }
    default: {
        printf ( "unexpected parameter %i \n",index );
        parameter.name = "describe it";
        parameter.symbol = "describeIt";
    }
    }
}

void NinjasPlugin::initState ( uint32_t index, String& stateKey, String& defaultStateValue )
{
    switch ( index ) {
    case stateFilepathFromUi: {
        stateKey ="filepathFromUI";
        defaultStateValue = String ( filepath.c_str() );
        break;
    }
    case statePathFromState: {
        stateKey ="filepathFromState";
        defaultStateValue = String ( filepath.c_str() );
        break;
    }
    case stateSlices: {
        stateKey = "slices";
        defaultStateValue = "empty";
        break;
    }
    case stateStoreProgram: {
        stateKey = "storeProgram";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram00: {
        stateKey = "program00";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram01: {
        stateKey = "program01";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram02: {
        stateKey = "program02";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram03: {
        stateKey = "program03";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram04: {
        stateKey = "program04";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram05: {
        stateKey = "program05";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram06: {
        stateKey = "program06";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram07: {
        stateKey = "program07";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram08: {
        stateKey = "program08";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram09: {
        stateKey = "program09";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram10: {
        stateKey = "program10";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram11: {
        stateKey = "program11";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram12: {
        stateKey = "program12";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram13: {
        stateKey = "program13";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram14: {
        stateKey = "program14";
        defaultStateValue = "empty";
        break;
    }
    case stateProgram15: {
        stateKey = "program15";
        defaultStateValue = "empty";
        break;
    }
    case stateProgramNumber: {
        stateKey = "programNumber";
        defaultStateValue = "empty";
        break;
    }
    case stateCurrentSlice: {
        stateKey = "currentSlice";
        defaultStateValue = "empty";
        break;
    }
    case stateSliceButton: {
        stateKey = "sliceButton";
        defaultStateValue = "empty";
        break;
    }
    case stateSigSampleLoaded: {
        stateKey = "sig_SampleLoaded";
        defaultStateValue = "empty";
        break;
    }
    case stateSigProgramLoaded: {
        stateKey = "sig_ProgramLoaded";
        defaultStateValue = "empty";
        break;
    }
    case stateSigCurrentSlice: {
        stateKey = "sig_CurrentSlice";
        defaultStateValue = "-1";
    }

    } // switch
} // initState

String NinjasPlugin::getState ( const char* key ) const
{
    if ( std::strcmp ( key, "filepathFromState" ) == 0 ) {
        return String ( filepath.c_str() );
    }

    if ( std::strcmp ( key, "filepathFromUI" ) == 0 ) {
             return String ( filepath.c_str() );
        }
    if ( std::strcmp ( key, "programNumber" ) == 0 ) {
        return String ( programNumber );
    }
    if ( std::strcmp ( key, "currentSlice" ) == 0 ) {
        return String ( Programs[programNumber].currentSlice );
    }
    if ( std::strcmp ( key, "program00" ) == 0 ) {
        return String ( serializeProgram ( 0 ).c_str() );
    }
    if ( std::strcmp ( key, "program01" ) == 0 ) {
        return String ( serializeProgram ( 1 ).c_str() );
    }
    if ( std::strcmp ( key, "program02" ) == 0 ) {
        return String ( serializeProgram ( 2 ).c_str() );
    }
    if ( std::strcmp ( key, "program03" ) == 0 ) {
        return String ( serializeProgram ( 3 ).c_str() );
    }
    if ( std::strcmp ( key, "program04" ) == 0 ) {
        return String ( serializeProgram ( 4 ).c_str() );
    }
    if ( std::strcmp ( key, "program05" ) == 0 ) {
        return String ( serializeProgram ( 5 ).c_str() );
    }
    if ( std::strcmp ( key, "program06" ) == 0 ) {
        return String ( serializeProgram ( 6 ).c_str() );
    }
    if ( std::strcmp ( key, "program07" ) == 0 ) {
        return String ( serializeProgram ( 7 ).c_str() );
    }
    if ( std::strcmp ( key, "program08" ) == 0 ) {
        return String ( serializeProgram ( 8 ).c_str() );
    }
    if ( std::strcmp ( key, "program09" ) == 0 ) {
        return String ( serializeProgram ( 9 ).c_str() );
    }
    if ( std::strcmp ( key, "program10" ) == 0 ) {
        return String ( serializeProgram ( 10 ).c_str() );
    }
    if ( std::strcmp ( key, "program11" ) == 0 ) {
        return String ( serializeProgram ( 11 ).c_str() );
    }
    if ( std::strcmp ( key, "program12" ) == 0 ) {
        return String ( serializeProgram ( 12 ).c_str() );
    }
    if ( std::strcmp ( key, "program13" ) == 0 ) {
        return String ( serializeProgram ( 13 ).c_str() );
    }
    if ( std::strcmp ( key, "program14" ) == 0 ) {
        return String ( serializeProgram ( 14 ).c_str() );
    }
    if ( std::strcmp ( key, "program15" ) == 0 ) {
        return String ( serializeProgram ( 15 ).c_str() );
    }

    if ( std::strcmp ( key, "sig_SampleLoaded" ) == 0 ) {
        return String ( !bypass ); // if not bypassed sample is loaded. UI should reload sample when state restored
    }
    return String ( "empty" );
}

void NinjasPlugin::setState ( const char* key, const char* value )
{
    
    if ( strcmp ( value, "empty" ) == 0 ) {
        return;
    }

    if ( strcmp ( key, "sliceSensitivity" ) == 0 ) {
        getOnsets();
    }

    if ( strcmp ( key, "sliceButton" ) == 0 ) {
        if ( strcmp ( value, "true" ) == 0 ) {
            std::fill_n ( Programs[programNumber].a_slices, 128, Slice() );
            float attack = getParameterValue( paramAttack );
            float decay = getParameterValue( paramDecay );
            float sustain = getParameterValue( paramSustain );
            float release = getParameterValue( paramRelease );
            std::fill_n ( Programs[programNumber].Attack, 128, attack );
            std::fill_n ( Programs[programNumber].Decay,128, decay );
            std::fill_n ( Programs[programNumber].Sustain, 128, sustain );
            std::fill_n ( Programs[programNumber].Release, 128, release );
            switch ( slicemode ) {
            case RAW:
                createSlicesRaw();
                break;
            case ONSETS:
                createSlicesOnsets();
                break;
            default:
                printf ( "unexpected slicemode :%i\n",slicemode );
            }
            sig_LoadProgram = true;
        }
    }

    if ( strcmp ( key, "filepathFromState" ) == 0 ) {
        filepath = value;
        // load file in sample memory
        if ( !loadSample ( filepath, false ) ) {
            // sample loaded ok, slice it up and set bool
            getOnsets ();
            bypass = false;
            sig_SampleLoaded = true;
            //setParameterValue ( paramLoadSample, 1.0f );
        } else {
            bypass = true;
            //setParameterValue ( paramLoadSample, 0.0f );
        }
    }

    if ( strcmp ( key, "filepathFromUI" ) == 0 ) {
        if ( strcmp ( value, "empty" ) == 0 )
            return;

        filepath = value;
        // load file in sample memory
        if ( !loadSample ( filepath, true ) ) {
            // sample loaded ok, slice it up and set bool
            initPrograms();
            getOnsets ();
            if ( slicemode == RAW )
                createSlicesRaw ();
            else
                createSlicesOnsets ();
            bypass = false;
            sig_SampleLoaded = true;

            for (int i=1; i <16 ; i++)
            {
                Programs[i].a_slices[0].sliceStart=0;
                Programs[i].a_slices[0].sliceEnd=sampleSize*sampleChannels;
            }
        } else {
            bypass = true;
            //setParameterValue ( paramLoadSample, 0.0f );
        }
    }

    if ( strcmp ( key, "slices" ) == 0 ) {
        const char* p = value;
        char * end;
        bool start = true;

        for ( int iValue = std::strtol ( p, &end,10 ), index = 0; p != end; iValue = std::strtol ( p, &end, 10 ) ) {
            p = end;
            if ( errno == ERANGE ) {
                std::cout << "slices range error, got " << p << std::endl;
                errno = 0;
            }
            if ( start )
                Programs[programNumber].a_slices[index].sliceStart = iValue * sampleChannels;
            else {
                Programs[programNumber].a_slices[index].sliceEnd = iValue * sampleChannels;
                index++;
            }
            start = !start;
        }

    }

    if ( strcmp ( key, "storeprogram" ) == 0 ) {
        const char* p = value;
        char * end;
        std::vector<int> v; // 0 = oldProgram, 1 = newProgram
        for ( int iValue = std::strtol ( p, &end,10 ); p != end; iValue = std::strtol ( p, &end, 10 ) ) {
            p = end;
            if ( errno == ERANGE ) {
                std::cout << "storeProgram range error, got " << p << std::endl;
                errno = 0;
            }
            v.push_back ( iValue );
        }
        setProgram ( v[0],v[1] );
    }

    if ( strcmp ( key, "program00" ) == 0 ) {
        deserializeProgram ( 0,value );
    }
    if ( strcmp ( key, "program01" ) == 0 ) {
        deserializeProgram ( 1,value );
    }
    if ( strcmp ( key, "program02" ) == 0 ) {
        deserializeProgram ( 2,value );
    }
    if ( strcmp ( key, "program03" ) == 0 ) {
        deserializeProgram ( 3,value );
    }
    if ( strcmp ( key, "program04" ) == 0 ) {
        deserializeProgram ( 4,value );
    }
    if ( strcmp ( key, "program05" ) == 0 ) {
        deserializeProgram ( 5,value );
    }
    if ( strcmp ( key, "program06" ) == 0 ) {
        deserializeProgram ( 6,value );
    }
    if ( strcmp ( key, "program07" ) == 0 ) {
        deserializeProgram ( 7,value );
    }
    if ( strcmp ( key, "program08" ) == 0 ) {
        deserializeProgram ( 8,value );
    }
    if ( strcmp ( key, "program09" ) == 0 ) {
        deserializeProgram ( 9,value );
    }
    if ( strcmp ( key, "program10" ) == 0 ) {
        deserializeProgram ( 10,value );
    }
    if ( strcmp ( key, "program11" ) == 0 ) {
        deserializeProgram ( 11,value );
    }
    if ( strcmp ( key, "program12" ) == 0 ) {
        deserializeProgram ( 12,value );
    }
    if ( strcmp ( key, "program13" ) == 0 ) {
        deserializeProgram ( 13,value );
    }
    if ( strcmp ( key, "program14" ) == 0 ) {
        deserializeProgram ( 14,value );
    }
    if ( strcmp ( key, "program15" ) == 0 ) {
        deserializeProgram ( 15,value );
    }

    if ( strcmp ( key,  "programNumber" ) == 0 ) {
        programNumber = std::stoi ( value );
    }

    if ( strcmp ( key, "currentSlice" ) == 0 ) {
        Programs[programNumber].currentSlice = std::stoi ( value );
     }

    if ( strcmp ( key, "sig_SampleLoaded" ) == 0 ) {
        sig_SampleLoaded = false;
    }

    if ( strcmp ( key, "sig_LoadProgram" ) == 0 ) {
        sig_LoadProgram = false;
    }

    if ( strcmp ( key, "sig_CurrentSlice" ) == 0 ) {
        sig_currentSlice = -1;
    }

}
/* --------------------------------------------------------------------------------------------------------
* Internal data
*/

/**
Get the current value of a parameter.
The host may call this function from any context, including realtime processing.
*/
float NinjasPlugin::getParameterValue ( uint32_t index ) const

{
    float return_Value = 0.0f;
    int voice = ( Programs[programNumber].currentSlice + 60 ) %128;

    switch ( index ) {
    case paramProgramNumber: {
        return_Value = ( float ) programNumber;
        break;
    }
    case paramSigSampleLoaded: {
        return_Value = ( float ) sig_SampleLoaded;
        break;
    }
    case paramSigLoadProgram: {
        return_Value = ( float ) sig_LoadProgram;
        break;
    }

    case paramCurrentSlice: {
        return_Value = sig_currentSlice;
        break;
    }

    case paramNumberOfSlices:
        return_Value = ( float ) Programs[programNumber].slices;
        break;
    case paramAttack:
        return_Value = Programs[programNumber].Attack[voice];
        break;
    case paramDecay:
        return_Value = Programs[programNumber].Decay[voice];
        break;
    case paramSustain:
        return_Value = Programs[programNumber].Sustain[voice];
        break;
    case paramRelease:
        return_Value = Programs[programNumber].Release[voice];
        break;
    case paramSliceMode:
        return_Value = slicemode;
        break;
    case paramSliceSensitivity:
        return_Value = sliceSensitivity;
        break;
    case paramProgramGrid:
        return_Value = programGrid;
        break;
    case paramPitchbendDepth:
        return_Value = pitchbendDepth/2;
    }
    return return_Value;

}

/**
Change a parameter value.
The host may call this function from any context, including realtime processing.
When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
@note This function will only be called for parameter inputs.
*/
void NinjasPlugin::setParameterValue ( uint32_t index, float value )
{
    int voice = ( Programs[programNumber].currentSlice + 60 ) % 128;
    switch ( index ) {
    case paramNumberOfSlices:
        Programs[programNumber].slices = ( int ) value;
        break;
    case paramAttack:
        Programs[programNumber].Attack[voice] = value;
        break;
    case paramDecay:
        Programs[programNumber].Decay[voice] = value;
        break;
    case paramSustain:
        Programs[programNumber].Sustain[voice] = value;
        break;
    case paramRelease:
        Programs[programNumber].Release[voice] = value;
        break;
    case paramSliceMode:
        slicemode = ( SLICEMODE ) value;
        break;
    case paramSliceSensitivity:
        sliceSensitivity = value;
        getOnsets();
        break;
    case paramProgramGrid:
        programGrid = value;
        break;
    case paramProgramNumber: {
        programNumber = value;
        break;
    }
    case paramPlayMode: {
        Programs[programNumber].a_slices[Programs[programNumber].currentSlice].playmode = static_cast<slicePlayMode> ( value );
        break;
    }
    case paramPitchbendDepth: {
        pitchbendDepth = value*2;
        pitchbendStep  = 16384.0f / pitchbendDepth;
        break;
    }
    case paramOneShotForward: {
        Programs[programNumber].a_slices[Programs[programNumber].currentSlice].playmode = ONE_SHOT_FWD;
        break;
    }
    case paramOneShotReverse: {
        Programs[programNumber].a_slices[Programs[programNumber].currentSlice].playmode = ONE_SHOT_REV;
        break;
    }
    case paramLoopForward: {
        Programs[programNumber].a_slices[Programs[programNumber].currentSlice].playmode = LOOP_FWD;
        break;
    }
    case paramLoopReverse: {
        Programs[programNumber].a_slices[Programs[programNumber].currentSlice].playmode = LOOP_REV;
        break;
    }
    case paramLoadSample : 
        break;
    default:
        std::cerr << "NinjasPlugin::setParameterValue unexpected parameter. index = " << index << "value = " <<value<< std::endl;
    }
} // setParameterValue

/* --------------------------------------------------------------------------------------------------------
* Audio/MIDI Processing */

//------------------- inputs unused   , outputs        , size of block we process, pointer to midi data       , number of midi events in current block
void NinjasPlugin::run ( const float**, float** outputs, uint32_t frames,          const MidiEvent* midiEvents, uint32_t midiEventCount )
{
    float* const outL = outputs[0]; // output ports , stereo
    float* const outR = outputs[1];
    uint32_t framesDone = 0;
    uint32_t curEventIndex = 0; // index for midi event to process
    while ( framesDone < frames ) { // we have frames to process !!
        if ( !bypass ) {
            // process any ready midi events
            // we have midi data to proces, at precisly the current audio frame in the loop
            while ( curEventIndex < midiEventCount && framesDone == midiEvents[curEventIndex].frame ) { // the .frame is the offset of the midi event in current block
                if ( midiEvents[curEventIndex].size > MidiEvent::kDataSize ) // not excatly shure what's happening here. this is in both Nekobi and Kars sourcecode
                    continue;

                int status = midiEvents[curEventIndex].data[0]; // midi status
                // int channel = status & 0x0F ; // get midi channel
                int message = status & 0xF0 ; // get midi message
                int data1 = midiEvents[curEventIndex].data[1]; // note number
                int data2 = midiEvents[curEventIndex].data[2]; //

                switch ( message ) {
                case 0x80 : { // note off
                    int index = data1;
                    bool voice_playing = voices[index].active;
                    if ( voice_playing == false ) {
                        break; // note wasn't playing anyway .. ignore
                    }
                    if ( voice_playing )
                        voices[index].adsr.adsr_stage = stage_of_ADSR::RELEASE;
                    break;
                }

                case 0x90 : { // note on
                    //c4 is 60
                    int index = ( data1 + 68 ) % 128;
                    if ( index < 0 || index > Programs[programNumber].slices -1 ) {
                        break;
                    }
                    // if velocity == 0 it's actually a note off event
                    if (data2 == 0)
                    {
                        voices[data1].adsr.adsr_stage = stage_of_ADSR::RELEASE;
                        break;
                    }
                    // new note .. let's activate
                    voices[data1].active = true;
                    voices[data1].notenumber = index;
                    voices[data1].velocity = data2;
                    voices[data1].gain = ( float ) data2 / 127.0f;

                    // init adsr
                    voices[data1].adsr.adsr_stage = ATTACK;
                    voices[data1].adsr.adsr_gain = 0.0f;

                    // set adsr values
                    voices[data1].adsr.attack  = Programs[programNumber].Attack[data1];
                    voices[data1].adsr.decay   = Programs[programNumber].Decay[data1];
                    voices[data1].adsr.sustain = Programs[programNumber].Sustain[data1];
                    voices[data1].adsr.release = Programs[programNumber].Release[data1];

                    // calculate gain
                    voices[data1].adsr.attack_gain  = ( 1.0f / voices[data1].adsr.attack )  / samplerate;
                    voices[data1].adsr.decay_gain   = - ( 1.0f / voices[data1].adsr.decay )  / samplerate;
                    voices[data1].adsr.release_gain = - ( 1.0f / voices[data1].adsr.release )  / samplerate;

                    // check playmode
                    // if LOOP_REV or ONE_SHOT_REV set playback indici to end of slice
                    if ( Programs[programNumber].a_slices[index].playmode == LOOP_REV || Programs[programNumber].a_slices[index].playmode == ONE_SHOT_REV ) {
                        voices[data1].playbackIndex = Programs[programNumber].a_slices[index].sliceEnd - Programs[programNumber].a_slices[index].sliceStart;
                        voices[data1].framePosition = ( Programs[programNumber].a_slices[index].sliceEnd - Programs[programNumber].a_slices[index].sliceStart ) / sampleChannels;

                    }

                    else {   // playmode is forward .. playback indici to start
                        voices[data1].playbackIndex = 0;
                        voices[data1].framePosition = 0;
                    }

                    // set sig_currentSlice
                    sig_currentSlice = index;
                    Programs[programNumber].currentSlice = index;

                    float transpose = ( pitchbend / pitchbendStep ) - ( pitchbendDepth/2 );
                    voices[data1].frameDelta=pow ( 2.0, transpose / 12.0 );
                    break;

                } // case 0x90

                case 0xe0: { // pitchbend
                    pitchbend = ( data2 * 128 ) + data1;
                    break;
                }

                case 0xc0: { // program change
                    programNumber = data1 % 16;
                    break;
                }

                } // switch

                curEventIndex++; // we've processed a midi event,increase index so we know which midi event to process next
            }
            // loop through active voices
            int voice_count {0};
            for ( int i = 0 ; i < 128 ; i++ ) {
                if ( voices[i].active ) {
                    voice_count++;
                    // get the raw samples from the voice
                    // float* pointer will allow any amount of samples to be pulled in
                    //
                    int slice = ( i+68 ) %128;
                    int sliceStart = Programs[programNumber].a_slices[slice].sliceStart; //fix this !
                    int sliceEnd = Programs[programNumber].a_slices[slice].sliceEnd;
                    int pos = voices[i].playbackIndex;
                    if ( ( sliceStart + pos ) > ( sampleVector.size()-sampleChannels ) ) {
                        pos = 0;
                    }

                    float* sample = &sampleVector.at ( sliceStart+pos );
                    float sampleL { *sample };
                    float sampleR { * ( sample + ( sampleChannels -1 ) ) };
                    // process adsr to get the gain back
                    switch ( voices[i].adsr.adsr_stage ) {
                    case ATTACK: {
                        if ( voices[i].adsr.adsr_gain < 1.0f )
                            voices[i].adsr.adsr_gain += voices[i].adsr.attack_gain;
                        else {
                            voices[i].adsr.adsr_gain = 1.0f;
                            voices[i].adsr.adsr_stage = DECAY;
                        }
                        break;
                    }
                    // now go into decayg
                    case DECAY: {
                        if ( voices[i].adsr.adsr_gain > voices[i].adsr.sustain )
                            voices[i].adsr.adsr_gain += voices[i].adsr.decay_gain;
                        else {
                            voices[i].adsr.adsr_gain = voices[i].adsr.sustain;
                            voices[i].adsr.adsr_stage = SUSTAIN;
                        }
                        break;
                    }
                    // sustain
                    case SUSTAIN: {
                        //	return adsr_gain;
                        break;
                    }
                    // release phase ; should be triggered by note off
                    case RELEASE: {
                        if ( voices[i].adsr.adsr_gain > 0.0f )
                            voices[i].adsr.adsr_gain += voices[i].adsr.release_gain;
                        else {
                            // gain might have dipped below 0.0
                            voices[i].adsr.adsr_gain = 0.0f;
                            voices[i].active = false;
                        }
                        break;
                    }

                    }

                    gain = voices[i].gain * voices[i].adsr.adsr_gain;

                    sampleL = sampleL * gain;
                    sampleR = sampleR * gain;

                    // put samples in mixer

                    mixer.leftChannel  += sampleL;
                    mixer.rightChannel += sampleR;
                    mixer.samples++;

                    // increase sample reading position
                    float transpose = ( pitchbend/pitchbendStep ) - ( pitchbendDepth/2 );
                    voices[i].frameDelta=pow ( 2.0, transpose / 12.0 );
                    float frameDelta = voices[i].frameDelta;

                    // set frameDelta to negative if direction is reverse

                    if ( Programs[programNumber].a_slices[slice].playmode == LOOP_REV || Programs[programNumber].a_slices[slice].playmode == ONE_SHOT_REV )
                        frameDelta=-frameDelta;

                    // add the frameDelta, when it's negative this should substract

                    voices[i].framePosition += frameDelta;
                    int tmp = ( int ) voices[i].framePosition;
                    tmp = tmp * sampleChannels;

                    // check bounderies according to playmode: loop or oneshot.
                    switch ( Programs[programNumber].a_slices[slice].playmode ) {
                    case LOOP_FWD: {
                        if ( sliceStart + tmp >= ( sliceEnd-sampleChannels ) ) {
                            voices[i].playbackIndex = 0;
                            voices[i].framePosition = 0;
                        } else {
                            voices[i].playbackIndex = tmp;
                        }
                        break;
                    }

                    case LOOP_REV: {
                        if ( sliceStart + tmp <= sliceStart ) {
                            voices[i].playbackIndex = sliceEnd - sliceStart;
                            voices[i].framePosition = ( sliceEnd -sliceStart ) / sampleChannels;
                        } else
                            voices[i].playbackIndex = tmp;
                        break;
                    }
                    case ONE_SHOT_FWD: {
                        if ( sliceStart + tmp >= ( sliceEnd-sampleChannels ) ) {
                            voices[i].active=false;
                        } else voices[i].playbackIndex = tmp;
                        break;
                    }
                    case ONE_SHOT_REV: {
                        if ( sliceStart + tmp <= sliceStart ) {
                            voices[i].active=false;
                        } else
                            voices[i].playbackIndex = tmp;
                        break;
                    }

                    } //switch
                }// if voices[i].active
            } // end for loop through active voices

            if ( voice_count == 0 ) {
                mixer.leftChannel += 0;
                mixer.rightChannel += 0;
                ++mixer.samples;
            }
            float left = mixer.leftChannel /   sqrt ( ( float ) mixer.samples );
            float right = mixer.rightChannel / sqrt ( ( float ) mixer.samples );
            // reset mixer
            mixer.samples = 0;
            mixer.leftChannel = 0.0f;
            mixer.rightChannel = 0.0f;

            outL[framesDone] = left;
            outR[framesDone] = right;
        } else {
            // no voices playing
            outL[framesDone] = 0; // output 0 == silence
            outR[framesDone] = 0;
        }
        ++framesDone;

    }
}// run()

// slice funtions

void NinjasPlugin::createSlicesRaw ()
{
    for ( int s = 0 ; s < 128 ; s++ ) {
        Programs[programNumber].a_slices[s].sliceStart = 0;
        Programs[programNumber].a_slices[s].sliceEnd = 0;
    }

    long double sliceSize = ( long double ) ( sampleSize * sampleChannels ) / ( long double ) Programs[programNumber].slices;
    for ( int i = 0 ; i < Programs[programNumber].slices; i++ ) {
        Programs[programNumber].a_slices[i].sliceStart = ( int ) i * sliceSize;
        Programs[programNumber].a_slices[i].sliceStart = Programs[programNumber].a_slices[i].sliceStart;
        Programs[programNumber].a_slices[i].sliceEnd   = ( ( int ) ( i+1 ) * sliceSize ) - 1;
        Programs[programNumber].a_slices[i].sliceEnd = Programs[programNumber].a_slices[i].sliceEnd;
    }
}

void NinjasPlugin::getOnsets ()
{
    // temp sample vector
    std::vector<float> tmp_sample_vector;
    int hop_size = 256;
    int win_s = 512;
    fvec_t ftable;               // 1. create fvec without allocating it
    uintptr_t readptr = 0;
    ftable.length = hop_size;    // 2. set ftable length
    fvec_t * out = new_fvec ( 2 ); // output position
    if ( sampleChannels == 2 ) { // create mono sample
        for ( int i=0, j=0 ; i <= sampleSize; i++ ) {
            // sum to mono
            float sum_mono = ( sampleVector[j] + sampleVector[j+1] ) * 0.5f;
            tmp_sample_vector.push_back ( sum_mono );
            j+=2;
        }
    } else {
        tmp_sample_vector = sampleVector;
    }

    // create onset object
    aubio_onset_t  * onset = new_aubio_onset ( "specdiff", win_s, hop_size, samplerate );
    aubio_onset_set_threshold ( onset, 1 - sliceSensitivity ) ;
    onsets.clear();
    while ( readptr + hop_size <= tmp_sample_vector.size() ) {
        ftable.data = &tmp_sample_vector[readptr];
        aubio_onset_do ( onset, &ftable, out );
        if ( out->data[0] != 0 ) {
            onsets.push_back ( aubio_onset_get_last ( onset ) );
        }
        readptr += hop_size;
    }
    
    del_aubio_onset ( onset );
    aubio_cleanup();
}

void NinjasPlugin::createSlicesOnsets ()
{
    if ( sampleSize == 0 ) {
        return;
    }
    long double sliceSize = ( long double ) sampleSize / ( long double ) Programs[programNumber].slices;

    // raw slicing
    for ( int i = 0 ; i < Programs[programNumber].slices; i++ ) {
        int64_t start = ( ( int ) i * sliceSize );
        int64_t end = ( ( ( int ) ( i+1 ) * sliceSize ) - 1 );
        // map to nearest onset
        int64_t onset_start = find_nearest ( onsets,start );
        int64_t onset_end = find_nearest ( onsets,end )-1;

        Programs[programNumber].a_slices[i].sliceStart =  onset_start * sampleChannels ;
        Programs[programNumber].a_slices[i].sliceEnd = onset_end * sampleChannels ;
        // set slice end of last slice to end of sample
        if ( i == Programs[programNumber].slices -1 ) {
            Programs[programNumber].a_slices[i].sliceEnd = end * sampleChannels ;
        }
    }
    // purge zero lenght slices

    int totalSlices = 0;
    Slice temp[Programs[programNumber].slices];
    for ( int i = 0; i < Programs[programNumber].slices; i++ ) {
        int64_t start = Programs[programNumber].a_slices[i].sliceStart;
        int64_t end = Programs[programNumber].a_slices[i].sliceEnd;
        int64_t length = end - start;
         if ( length > 0 ) {
            temp[totalSlices]=Programs[programNumber].a_slices[i];
            totalSlices++;
        }
    }
    Programs[programNumber].slices = totalSlices;
    for ( int i = 0 ; i < totalSlices; i++ ) {
        Programs[programNumber].a_slices[i]=temp[i];
    }
    //FIXME paramNumberOfSlices isn't updated
}

int64_t NinjasPlugin::find_nearest ( std::vector<uint_t> & haystack, uint_t needle )
{
    auto distance_to_needle_comparator = [&] ( int64_t && a,  int64_t && b ) {
        return std::abs ( a - needle ) < std::abs ( b - needle );
    };

    return *std::min_element ( std::begin ( haystack ), std::end ( haystack ), distance_to_needle_comparator );
}

int NinjasPlugin::loadSample ( std::string fp, bool fromUser )
{
    int file_samplerate ( 0 );
    // get extension
    std::string ext = fp.substr ( fp.find_last_of ( "." ) + 1 );
    std::transform ( ext.begin(), ext.end(), ext.begin(), ::tolower );
    
    if ( ext == "mp3" ) {
        mp3dec_t mp3d;
        mp3dec_file_info_t info;
        if ( mp3dec_load ( &mp3d, fp.c_str(), &info, NULL, NULL ) ) {
            std::cout << "Can't load sample " << fp << std::endl;
            return -1;
        }
        file_samplerate = info.hz;
        sampleChannels = info.channels;
        // fill samplevector
        sampleVector.clear();
        for ( uint i =0 ; i < info.samples ; i++ ) {
            sampleVector.push_back ( * ( info.buffer+i ) );

        }

        sampleSize = sampleVector.size() /sampleChannels;
    } else {
        SndfileHandle fileHandle ( fp, SFM_READ,  SF_FORMAT_WAV | SF_FORMAT_FLOAT, 2, 44100 );

        // get the number of frames in the sample
        sampleSize = fileHandle.frames();

        if ( sampleSize == 0 ) {
            //file doesn't exist or is of incompatible type, main handles the -1
            std::cout << "Can't load sample " << fp << std::endl;
            return -1;
        }
        // get some more info of the sample

        sampleChannels = fileHandle.channels();
        file_samplerate = fileHandle.samplerate();

        // resize vector
        sampleVector.resize ( sampleSize * sampleChannels );

        // load sample memory in samplevector
        fileHandle.read ( &sampleVector.at ( 0 ), sampleSize * sampleChannels );
    }

    // check if samplerate != host_samplerate
    if ( file_samplerate != samplerate ) {
        // temporary sample vector
        std::vector<float> tmp_sample_vector = sampleVector;

        SRC_DATA src_data;
        src_data.data_in = & tmp_sample_vector.at ( 0 );
        src_data.src_ratio = samplerate / file_samplerate;
        src_data.output_frames = sampleSize * src_data.src_ratio;

        sampleVector.resize ( src_data.output_frames * sampleChannels );

        src_data.data_out = & sampleVector.at ( 0 );
        src_data.input_frames = sampleSize;

        int err = src_simple ( & src_data, SRC_SINC_BEST_QUALITY, sampleChannels );
        if ( err )
            std::cout << "Samplerate error : src_simple err =" << err << std::endl;
        sampleSize = src_data.output_frames_gen;
    }


    if ( fromUser ) {
        programNumber = 0;
        createSlicesRaw();
        for ( int p=1; p <16; p++ ) {
            Programs[p]=Programs[0];
        }
    }
    return 0;
}

void NinjasPlugin::setProgram ( int oldProgram, int newProgram )
{
    Programs[newProgram]=Programs[oldProgram];
}
void NinjasPlugin::initPrograms()
{
    for ( int p=0; p < 16; p++ ) {
        Programs[p].program_isEmpty = true;
        Programs[p].slices = 1;
        Programs[p].currentSlice = 0;
        std::fill_n ( Programs[p].a_slices, 128, Slice() );
        std::fill_n ( Programs[p].Attack, 128, 0.001f );
        std::fill_n ( Programs[p].Decay,128, 0.001f );
        std::fill_n ( Programs[p].Sustain, 128, 1.0f );
        std::fill_n ( Programs[p].Release, 128, 0.001f );
        // std::fill_n ( Programs[p].OneShotFwd, 128, 1.0f );
        // std::fill_n ( Programs[p].OneShotRev,128, 0.0f );
        // std::fill_n ( Programs[p].LoopFwd, 128, 0.0f );
        // std::fill_n ( Programs[p].LoopRev, 128, 0.0f );
    }
    programNumber = 0;
}

std::string NinjasPlugin::serializeProgram ( int program ) const
{
    std::string programsString = "";
    programsString.append ( std::to_string ( Programs[program].slices ) );
    programsString.append ( " " );
    programsString.append ( std::to_string ( Programs[program].currentSlice ) );
    programsString.append ( " " );
    //  loop over slices
    for ( int i=0; i < 128 ; i++ ) {
        programsString.append ( std::to_string ( Programs[program].a_slices[i].sliceStart ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].a_slices[i].sliceEnd ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].a_slices[i].playmode ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].Attack[i] ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].Decay[i] ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].Sustain[i] ) );
        programsString.append ( " " );
        programsString.append ( std::to_string ( Programs[program].Release[i] ) );
        programsString.append ( " " );
    }
    return programsString;
}

void NinjasPlugin::deserializeProgram ( const int program, const char* string )
{
    if ( strcmp ( string, "empty" ) == 0 ) {
        //     std::cout << "programsString is empty" << std::endl;
        return;
    }
    const char *p = string;
    char *end;
    float fValue;
    for ( int iValue = std::strtol ( p, &end,10 ) ; p != end; iValue = std::strtol ( p, &end, 10 ) ) {
        p = end;
        if ( errno == ERANGE ) {
            std::cout << "deserializeProgram range error, got " << p << std::endl;
            errno = 0;
        }
        // common
        Programs[program].slices = iValue;

        iValue = std::strtol ( p, &end,10 );
        p = end;
        Programs[program].currentSlice = iValue;

        // loop over slices
        for ( int i=0; i < 128 ; i++ ) {
            // slices
            iValue = std::strtol ( p, &end,10 );
            p = end;
            Programs[program].a_slices[i].sliceStart = iValue;

            iValue = std::strtol ( p, &end,10 );
            p = end;
            Programs[program].a_slices[i].sliceEnd = iValue;

            iValue = std::strtol ( p, &end,10 );
            p = end;
            Programs[program].a_slices[i].playmode = static_cast<slicePlayMode> ( iValue );

            // ADSR
            fValue = std::strtof ( p, &end );
            p = end;
            Programs[program].Attack[i]= fValue;

            fValue = std::strtof ( p, &end );
            p = end;
            Programs[program].Decay[i]= fValue;

            fValue = std::strtof ( p, &end );
            p = end;
            Programs[program].Sustain[i]= fValue;

            fValue = std::strtof ( p, &end );
            p = end;
            Programs[program].Release[i]= fValue;
        }
    }
}
/* ------------------------------------------------------------------------------------------------------------
* Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new NinjasPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO


