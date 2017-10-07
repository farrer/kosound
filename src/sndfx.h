/*
 Kosound - A simple sound library
 Copyright (C) DNTeam <kosound@dnteam.org>
 
 This file is part of Kosound.
 
 Kosound is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Kosound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with Kosound.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _kosound_sndfx_h
#define _kosound_sndfx_h

#include <kobold/platform.h>
#include <kobold/kstring.h>

#ifdef __APPLE__
   #include <OpenAL/al.h>
#else
   #include <AL/al.h>
#endif

#include "soundstream.h"

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS
   /* CAF sound files for iOS and MacOS */
   #include "cafstream.h"
#endif

/* OGG sound files for all platforms */
#include "oggstream.h"

#include "kosoundconfig.h"
#include <kobold/list.h>

namespace Kosound
{

/*! Sound Effect Manipulation and Definitions */
class SndFx: public Kobold::ListElement
{
   public:
      /*! Constructor of NULL class (for head node) */
      SndFx();

      /*! Constructor of Non positional Source 
       * \param lp -> loop interval (<0 won't loop, =0 loop 
       *              just after the end, >0 wait lp seconds to loop)
       * \param fileName -> name of the Ogg File to Open */
      SndFx(int lp, const Kobold::String& fileName);
      
      /*! Constructor of the Class.
       * \param centerX -> X position of the source
       * \param centerY -> Y position of the source
       * \param centerZ -> Z position of the source
       * \param lp -> loop interval (see setLoop)
       * \param fileName -> name of the Ogg File to Open */
      SndFx(ALfloat centerX, ALfloat centerY, ALfloat centerZ, int lp,
            const Kobold::String& fileName);
      /*! Destructor */
      ~SndFx();

      /*! Redefine Position of the Source
       * \param centerX -> X position of the source
       * \param centerY -> Y position of the source
       * \param centerZ -> Z position of the source */
      void redefinePosition(ALfloat centerX, ALfloat centerY, ALfloat centerZ);

      /*! Sets if the Source will Loop at end or not
       * \param lp -> loop interval (<0 won't loop, =0 loop 
       *              just after the end, >0 wait lp seconds to loop) */
      void setLoop(int lp);

      /*! Set the Velocity of the Source (if have one)
       * \param velX -> X velocity value
       * \param velY -> Y velocity value
       * \param velZ -> Z velocity value */
      void setVelocity(ALfloat velX, ALfloat velY, ALfloat velZ);

      /*! Set if the source Position is relative to the listener or not.
       *  Relative position is usually used to set a SndFx source to
       *  something that is IN the listener (like a flute, clock, etc) 
       *  \param relative -> true to set SourceRelative, false otherwise */
      void setRelative(bool relative);

      /*! Set the source to be directional.
       * \param direcX -> X direction value 
       * \param direcY -> Y direction value
       * \param direcZ -> Z direction value
       * \param innerAngle -> Cone Inner Angle
       * \param outerAngle -> Cone Outer Angle */
      void setDirectionCone(ALfloat direcX, ALfloat direcY, ALfloat direcZ,
                            ALfloat innerAngle, ALfloat outerAngle);

      /*! Update the Execution of the Source
       * \return false when execution is over */
      bool update();

      /*! Verify if the file still is playing */
      bool isPlaying();

      /*! Change the stream overall volume 
       * \param volume -> volume value [0 - 128]*/
      void changeVolume(int volume);
   
      /*! Define the SndFx as a music */
      void defineAsMusic();

      /*! Rewind the sound effect
       * \return true if successfull */
      bool rewind();

      /*! Set removal flag (true will remove when ended all plays) */
      void setRemoval(bool r){removable=r;};

      /*! Get if the sound effect is removable when finished 
       * (repeats included). */
      const bool getRemoval() const { return removable; };

   private:
      /*! Create specific sound stream (related with file type) */
      SoundStream* createStream(const Kobold::String& fileName);

      SoundStream* sndStream; /**< Sound stream used */
      bool removable; /**< if is automatically removable or not */
};

}

#endif

