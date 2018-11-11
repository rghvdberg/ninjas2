/*
 * Ninjas 2
 *
 * Copyright (C) 2018 Clearly Broken Software
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
#include <cerrno>
// #include <sstream>

#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------


// constructor
NinjasPlugin::NinjasPlugin()
  : Plugin ( paramCount, 0, 2 ) //1 parameter, 0 programs (presets) , 1 states
{
  // init parameters

  // adsr
  std::fill_n ( p_Attack, 128, 0.05f );
  std::fill_n ( p_Decay, 128, 0.05f );
  std::fill_n ( p_Sustain, 128, 1.0f );
  std::fill_n ( p_Release, 128, 0.05f );
  // play modes
  std::fill_n ( p_OneShotFwd, 128, 1.0f );
  std::fill_n ( p_OneShotRev, 128, 0.0f );
  std::fill_n ( p_LoopFwd, 128, 0.0f );
  std::fill_n ( p_LoopRev, 128, 0.0f );


  filepath = "";
  isPlaying = false;

  samplerate = getSampleRate();
  sampleChannels = 1;
  sampleSize = 0;
  // grid
  std::fill_n ( p_Grid, 16, 0 );
  p_Grid[0] = 1;
}

// Init

void NinjasPlugin::initParameter ( uint32_t index, Parameter& parameter )
{
  switch ( index )
    {
    case paramNumberOfSlices:
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsInteger;
      parameter.ranges.def = 1.0f;
      parameter.ranges.min = 1.0f;
      parameter.ranges.max = 128.0f;
      parameter.name   = "Slices";
      parameter.symbol  = "number_of_slices";
      parameter.midiCC = 102;
      break;
    }
    case paramAttack:
    {
      parameter.hints      = kParameterIsAutomable ;
      parameter.ranges.def = 0.05f;
      parameter.ranges.min = 0.05f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Attack";
      parameter.symbol = "attack";
      parameter.midiCC = 103;
      break;
    }
    case paramDecay:
    {
      parameter.hints      = kParameterIsAutomable ;
      parameter.ranges.def = 0.05f;
      parameter.ranges.min = 0.05f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Decay";
      parameter.symbol =  "decay";
      parameter.midiCC = 104;
      break;
    }
    case paramSustain:
    {
      parameter.hints      = kParameterIsAutomable ;
      parameter.ranges.def = 1.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name = "Sustain";
      parameter.symbol = "sustain";
      // parameter.midiCC = 105;
      break;
    }
    case paramRelease:
    {
      parameter.hints      = kParameterIsAutomable ;
      parameter.ranges.def = 0.05f;
      parameter.ranges.min = 0.05f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Release";
      parameter.symbol = "release";
      parameter.midiCC = 106;
      break;
    }
    case paramOneShotFwd:
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsBoolean ;
      parameter.ranges.def = 1.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "One Shot Forward";
      parameter.symbol  = "one_shot_fwd";
      parameter.midiCC = 107;
      break;
    }
    case paramOneShotRev:
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsBoolean ;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "One Shot Reverse";
      parameter.symbol  = "one_shot_rev";
      parameter.midiCC = 108;
      break;
    }
    case paramLoopFwd:
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsBoolean ;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Looped Play Forward";
      parameter.symbol  = "loop_fwd";
      parameter.midiCC = 109;
      break;
    }
    case paramLoopRev:
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsBoolean ;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Looped Play Reverse";
      parameter.symbol  = "loop_rev";
      parameter.midiCC = 110;
      break;
    }
    case paramLoadSample:
    {
      parameter.hints = kParameterIsAutomable|kParameterIsBoolean;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Load";
      parameter.symbol  = "load";
      break;
    }

    case paramSliceMode:
    {
      parameter.hints = kParameterIsAutomable|kParameterIsInteger;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Slice Mode";
      parameter.symbol  = "slicemode";
      parameter.midiCC = 111;
      break;
    }

    }
  if ( index >= paramSwitch01 && index <= paramSwitch16 )
    {
      parameter.hints      = kParameterIsAutomable|kParameterIsBoolean ;
      parameter.ranges.def = 0.0f;
      parameter.ranges.min = 0.0f;
      parameter.ranges.max = 1.0f;
      parameter.name   = "Switch "+String ( index - paramSwitch01 );
      parameter.symbol  = "switch"+String ( index - paramSwitch01 );
//       parameter.midiCC = index - paramSwitch01 + 33;

    }

}

void NinjasPlugin::initState ( uint32_t index, String& stateKey, String& defaultStateValue )
{
  switch ( index )
    {
    case 0:
    {
      stateKey ="filepath";
      defaultStateValue = "empty";
      break;
    }
    case 1:
    {
      stateKey = "slice";
      defaultStateValue = "empty";
      break;
    }
    }
}

String NinjasPlugin::getState ( const char* key ) const
{
  if ( std::strcmp ( key, "filepath" ) )
    return String ( "filepath" );

  if ( std::strcmp ( key, "slices" ) )
    return String ( "slices" );

  return String ( "something went wrong" );

}

void NinjasPlugin::setState ( const char* key, const char* value )
{
  if ( strcmp ( key, "filepath" ) == 0 )
    {
      std::string fp = value;
      // load file in sample memory
      if ( !loadSample ( fp ) )
        {
          // sample loaded ok, slice it up and set bool
          bypass = false;
          setParameterValue ( paramLoadSample, 1.0f );
        }
      else
        {
          bypass = true;
          setParameterValue ( paramLoadSample, 0.0f );
        }
    }

  if ( strcmp ( key, "slice" ) == 0 )
    {
      return;
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
  return 0.0f;
}

/**
Change a parameter value.
The host may call this function from any context, including realtime processing.
When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
@note This function will only be called for parameter inputs.
*/
void NinjasPlugin::setParameterValue ( uint32_t index, float value )
{
  return;
}

/* --------------------------------------------------------------------------------------------------------
* Audio/MIDI Processing */

//------------------- inputs unused , outputs       , size of block we process, pointer to midi data       , number of midi events in current block
void NinjasPlugin::run ( const float**, float** outputs, uint32_t frames,          const MidiEvent* midiEvents, uint32_t midiEventCount )
{

  float* const outL = outputs[0]; // output ports , stereo
  float* const outR = outputs[1];
  uint32_t framesDone = 0;
  uint32_t curEventIndex = 0; // index for midi event to process
  while ( framesDone < frames )   // we have frames to process !!
    {
      if ( !bypass )
        {
          // process any ready midi events
          // we have midi data to proces, at precisly the current audio frame in the loop
          while ( curEventIndex < midiEventCount && framesDone == midiEvents[curEventIndex].frame )   // the .frame is the offset of the midi event in current block
            {
              if ( midiEvents[curEventIndex].size > MidiEvent::kDataSize ) // not excatly shure what's happening here. this is in both Nekobi and Kars sourcecode
                continue;

              int status = midiEvents[curEventIndex].data[0]; // midi status
              // int channel = status & 0x0F ; // get midi channel
              int message = status & 0xF0 ; // get midi message
              int data1 = midiEvents[curEventIndex].data[1];// note number
              int data2 = midiEvents[curEventIndex].data[2]; //

              switch ( message )
                {
                case 0x80 :   // note off
                {
                  isPlaying = false;
                  break; // note wasn't playing anyway .. ignore
                }

                case 0x90 :
                {
                  isPlaying = true;
                  playbackIndex = 0;
                  break;
                } // case 0x90
                } // switch

              curEventIndex++; // we've processed a midi event,increase index so we know which midi event to process next
            }

          // get the raw samples from the voice
          // float* pointer will allow any amount of samples to be pulled in
          if (isPlaying)
	  {
          int pos = playbackIndex * 2;
          float* sample = &sampleVector.at ( pos );
          float sampleL { *sample };
          float sampleR { * ( sample + 1 ) };
	  outL[framesDone] = sampleL;
	  outR[framesDone] = sampleR;

          // increase sample reading position
          playbackIndex++;
	   if ( playbackIndex >= sampleSize )
            {
              playbackIndex = 0;
              isPlaying = false;
            }
	  }
	}
        
      else
        {
          // no voices playing
          outL[framesDone] = 0; // output 0 == silence
          outR[framesDone] = 0;
        }
      ++framesDone;

    }

}// run()



int NinjasPlugin::loadSample ( std::string fp )
{
  SndfileHandle fileHandle ( fp , SFM_READ,  SF_FORMAT_WAV | SF_FORMAT_FLOAT , 2 , 44100 );

  // get the number of frames in the sample
  sampleSize = fileHandle.frames();

  if ( sampleSize == 0 )
    {
      //file doesn't exist or is of incompatible type, main handles the -1
      std::cout << "Something went wrong" << std::endl;
      return -1;
    }
  // get some more info of the sample

  sampleChannels = fileHandle.channels();
  int file_samplerate = fileHandle.samplerate();

  // resize vector
  sampleVector.resize ( sampleSize * sampleChannels );

  // load sample memory in samplevector
  fileHandle.read ( &sampleVector.at ( 0 ) , sampleSize * sampleChannels );

  // check if samplerate != host_samplerate
  if ( file_samplerate != samplerate )
    std::cout << "resampling from " << file_samplerate << " to " << samplerate << std::endl;
  {
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
    sampleSize = src_data.output_frames_gen;
  }

  return 0;
}

/* ------------------------------------------------------------------------------------------------------------
* Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
  return new NinjasPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
