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

#ifndef _kosound_sound_h
#define _kosound_sound_h

#include "kosoundconfig.h"
#include <kobold/platform.h>

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS
   #include <OpenAL/al.h>
   #include <OpenAL/alc.h>
#else
   #include <AL/al.h>
   #include <AL/alc.h>
#endif

#include <kobold/kstring.h>
#include <kobold/timer.h>

#include "sndfx.h"


namespace Kosound
{

#define SOUND_NO_LOOP   -1
#define SOUND_AUTO_LOOP  0

#define DEFAULT_VOLUME  128

/*! The Sound Class definitions */
class Sound
{
   public:
      /*! Init the Sound system to use (must be called at program's init) */
      static void init();

      /*! Finish the use of Sound system (must be called at program's end) */
      static void finish();

      /*! Define the Listener 3D Position (usually the Camera Position)
       *  \param centerX -> X position of the listener
       *  \param centerY -> Y position of the listener 
       *  \param centerZ -> Z position of the listener
       *  \param theta -> Theta Orientation Angle of the listener 
       *  \param phi -> Phi Orientation Angle of the listener 
       *  \param d -> D Param of the Listener */
      static void setListenerPosition(ALfloat centerX, ALfloat centerY, 
            ALfloat centerZ, ALfloat phi, ALfloat theta, ALfloat d);


      /*! Flush All Buffers to the Sound Device, updating the played Sounds
       *  and music (usually called every frame, near GLflush() */
      static void flush();

      /*! Load and Start to Play OGG music file.
       * \param fileName -> name of the ogg file with the desired music. */
      static bool loadMusic(const Kobold::String& fileName);
   
      /*! Add Sound effect to the list
       *  \param x -> X position
       *  \param y -> Y position
       *  \param z -> Z position
       *  \param loop -> Sound loop interval ( < 0 won't loop) 
       *  \param fileName -> name of the ogg file to open
       *  \return pointer to the added Sound */
      static SndFx* addSoundEffect(ALfloat x, ALfloat y, ALfloat z, int loop,
            const Kobold::String& fileName);
      
      /*! Add Sound effect without position to the list
       *  \param loop -> if Sound will loop at end or not (see sndFx and
       *                                                   ogg_stream)
       *  \param fileName -> name of the ogg file to open
       *  \return pointer to the added Sound */
      static SndFx* addSoundEffect(int loop, const Kobold::String& fileName);

      /*! Remove Sound effect from list
       *  \param snd -> pointer to Sound effect to remove */
      static void removeSoundEffect(SndFx* snd);

      /*! Remove All Sound Effects from list */
      static void removeAllSoundEffects();

      /*! Change Overall Volume.
       *  \param music -> volume of the music
       *  \param sndV -> Sound effects volume */
      static void changeVolume(int music, int sndV); 

      /*! init the OpenAL device
       * \return true if successfull */
      static bool initOpenAL();

      /*! finish the openAL device and related Sounds */
      static void finishOpenAL();

   private:
      /* Must not allow instances. */
      Sound(){};

      static ALCdevice* device;         /**< Active AL device */
      static ALCcontext* context;       /**< Active AL context */
      static SndFx* backMusic;          /**< Active BackGround Music */

      static bool enabled;              /**< If Sound is Enabled or Not */

      static Kobold::List sndList;      /**< Head Node of sndFx List */

      static int musicVolume;           /**< The Music volume */
      static int sndfxVolume;           /**< The SndFxVolume */
      static Kobold::Timer timer;       /**< Timer for sound update */
};
   
}

#endif

