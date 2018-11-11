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

#ifndef NINJAS_HPP_INCLUDED
#define NINJAS_HPP_INCLUDED

#include "DistrhoPlugin.hpp"
#include <sndfile.hh>
#include <vector>
#include <iostream>
#include "aubio.h"
#include <algorithm>
#include <string>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class NinjasPlugin : public Plugin
{
public:

  NinjasPlugin();
  
protected:
  // -------------------------------------------------------------------
  // Information

  const char* getLabel() const noexcept override
  {
    return "Ninjas 2";
  }

  const char* getDescription() const override
  {
    return "Ninjas Is Not Just Another Slicer";
  }

  const char* getMaker() const noexcept override
  {
    return "Clearly Broken Software";
  }

  const char* getHomePage() const override
  {
    return "https://github.com/rghvdberg/ninjas2";
  }

  const char* getLicense() const noexcept override
  {
    return "ISC";
  }

  uint32_t getVersion() const noexcept override
  {
    return d_version ( 2, 0, 0 );
  }

  int64_t getUniqueId() const noexcept override
  {
    return d_cconst ( 'N', 'i', 'N', '2' );
  }

  // -------------------------------------------------------------------
  // Init

  void initParameter ( uint32_t index, Parameter& parameter ) override;
  void initState ( uint32_t index, String& stateKey, String& defaultStateValue );

  // -------------------------------------------------------------------
  // Internal data

  float getParameterValue ( uint32_t index ) const override;
  void  setParameterValue ( uint32_t index, float value ) override;
  String getState ( const char* key ) const;
  void setState ( const char* key, const char* value ) override;


  // -------------------------------------------------------------------
  // Process

  //void activate() override;
  void run ( const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount ) override;
  

  // -------------------------------------------------------------------

private:

  void createSlicesRaw ();
  void getOnsets ();
  void createSlicesOnsets ();
  int loadSample(std::string fp);
  int64_t find_nearest ( std::vector<uint_t> & haystack, uint_t needle );

  // Paramaters for 128 slices
  float p_Attack[128], p_Decay[128], p_Sustain[128], p_Release[128];
  float p_OneShotFwd[128], p_OneShotRev[128], p_LoopFwd[128], p_LoopRev[128];
  float p_Grid[16];
  
  bool isPlaying = false;
  bool bypass = true;
  int playbackIndex = 0;
  // empty sample object
  std::vector<float> sampleVector; // this holds the sample data
  int sampleChannels;
  sf_count_t sampleSize; // in frames !!

  std::string filepath; // = "";
  std::vector<uint_t>onsets;
  double samplerate;
   
  /*
   * Set our plugin class as non-copyable and add a leak detector just in case.
  */


  DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( NinjasPlugin )
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // NINJAS_HPP_INCLUDED
