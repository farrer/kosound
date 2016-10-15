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

#include "sndfx.h"
#include <kobold/log.h>

using namespace Kosound;

/*************************************************************************
 *                             Constructor                               *
 *************************************************************************/
SndFx::SndFx()
{
   sndStream = NULL;
   removable = true;
}

/*************************************************************************
 *                             Constructor                               *
 *************************************************************************/
SndFx::SndFx(ALfloat centerX, ALfloat centerY, ALfloat centerZ, int lp,
             Kobold::String fileName)
{
   removable = true;
   /* Create the Ogg Stream */ 
   sndStream = createStream(fileName);
   if(!sndStream)
   {
      return;
   }

   /*! open and load things */
   if(sndStream->open(fileName))
   {
      /* Define Position and OpenAL things */
      alSourcei(sndStream->getSource(), AL_SOURCE_RELATIVE, AL_FALSE);
      alSource3f(sndStream->getSource(), AL_POSITION, centerX,centerY,centerZ);
      alSourcef(sndStream->getSource(), AL_REFERENCE_DISTANCE, 160);
      alSource3f(sndStream->getSource(), AL_VELOCITY, 0.0, 0.0, 0.0);
      alSource3f(sndStream->getSource(), AL_DIRECTION, 0.0, 0.0, 0.0);
      alSourcef(sndStream->getSource(), AL_ROLLOFF_FACTOR, 1.0);
      alSourcef(sndStream->getSource(), AL_PITCH, 1.0f);
      alSourcef(sndStream->getSource(), AL_GAIN, 1.0f);
      setLoop(lp);

      if(!sndStream->playback())
      {
         Kobold::Log::add(Kobold::String("Couldn't play sound effect: ") +
               fileName);
      }
   }
   else
   {
      deleteStream();
   }
   next = NULL;
   previous = NULL;
}

/*************************************************************************
 *                             Constructor                               *
 *************************************************************************/
SndFx::SndFx(int lp, Kobold::String fileName)
{
   removable = true;
   /* Create the Ogg Stream */ 
   sndStream = createStream(fileName);
   if(!sndStream)
   {
      return;
   }
   
   if(sndStream->open(fileName))
   {
      /* Define Position */
      sndStream->defineAsMusic();
      setLoop(lp);

      if(!sndStream->playback())
      {
         //cerr << "Couldn't play sound effect: " << fileName << endl;
      }
   }
   else
   {
      deleteStream();
   }
   next = NULL;
   previous = NULL;
}

/*************************************************************************
 *                             createStream                              *
 *************************************************************************/
SoundStream* SndFx::createStream(Kobold::String fileName)
{
   if(fileName.find(Kobold::String(".ogg")) !=  Kobold::String::npos )
   {
      /* Create Ogg */
      return(new OggStream());
   }
   else
   {
#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS
      /* Create the CAF */
      return(new CafStream());
#endif
   }

   Kobold::Log::add(Kobold::String("Unsupported file format for: ") +
         fileName);
   return(NULL);
}

/*************************************************************************
 *                            deleteStream                               *
 *************************************************************************/
void SndFx::deleteStream()
{
   if(sndStream)
   {
      switch(sndStream->getType())
      {
#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS
         case SoundStream::TYPE_CAF:
         {
            CafStream* cs = (CafStream*)sndStream;
            delete(cs);
         }
         break;
#endif
         case SoundStream::TYPE_OGG:
         {
            OggStream* os = (OggStream*)sndStream;
            delete(os);
         }
         break;
         default:
         {
            delete(sndStream);
         }
         break;
      }
      sndStream = NULL;
   }
}

/*************************************************************************
 *                              Destructor                               *
 *************************************************************************/
SndFx::~SndFx()
{
   if(sndStream)
   {
      sndStream->release();
      deleteStream();
   }
}

/*************************************************************************
 *                                setLoop                                *
 *************************************************************************/
void SndFx::setLoop(int lp)
{
   if(sndStream)
   {
      sndStream->setLoop(lp);
   }
}

/*************************************************************************
 *                            defineAsMusic                              *
 *************************************************************************/
void SndFx::defineAsMusic()
{
   if(sndStream)
   {
      sndStream->defineAsMusic();
   }
}

/*************************************************************************
 *                             redefinePosition                          *
 *************************************************************************/
void SndFx::redefinePosition(ALfloat centerX, ALfloat centerY, ALfloat centerZ)
{
   if(sndStream != NULL)
   {
      alSource3f(sndStream->getSource(), AL_POSITION,centerX,centerY,centerZ);
   }
}

/*************************************************************************
 *                             setVelocity                               *
 *************************************************************************/
void SndFx::setVelocity(ALfloat velX, ALfloat velY, ALfloat velZ)
{
   if(sndStream != NULL)
   {
      alSource3f(sndStream->getSource(), AL_VELOCITY, velX, velY, velZ);
   }
}

/*************************************************************************
 *                             setRelative                               *
 *************************************************************************/
void SndFx::setRelative(bool relative)
{
   if(sndStream != NULL)
   {
      if(relative)
      {
         alSourcei(sndStream->getSource(), AL_SOURCE_RELATIVE, AL_TRUE);
      }
      else
      {
         alSourcei(sndStream->getSource(), AL_SOURCE_RELATIVE, AL_FALSE);
      }
   }
}

/*************************************************************************
 *                          setDirectionCone                             *
 *************************************************************************/
void SndFx::setDirectionCone(ALfloat direcX, ALfloat direcY, ALfloat direcZ,
                             ALfloat innerAngle, ALfloat outerAngle)
{
   if(sndStream != NULL)
   {
      alSource3f(sndStream->getSource(), AL_DIRECTION, direcX, direcY, direcZ);
      alSourcef(sndStream->getSource(), AL_CONE_INNER_ANGLE, innerAngle);
      alSourcef(sndStream->getSource(), AL_CONE_OUTER_ANGLE, outerAngle);
   }
}

/*************************************************************************
 *                               rewind                                  *
 *************************************************************************/
bool SndFx::rewind()
{
   if(sndStream != NULL)
   {
      return(sndStream->rewind());
   }

   return(false);
}

/*************************************************************************
 *                               update                                  *
 *************************************************************************/
bool SndFx::update()
{
   if(sndStream != NULL)
   {
      return(sndStream->update()); 
   }
   return(false);
}

/*************************************************************************
 *                               isPlaying                               *
 *************************************************************************/
bool SndFx::isPlaying()
{
   if(sndStream != NULL)
   {
      return(sndStream->isPlaying());
   }

   return(false);
}

/*************************************************************************
 *                            changeVolume                               *
 *************************************************************************/
void SndFx::changeVolume(int volume)
{
   if(sndStream)
   {
      sndStream->changeVolume(volume);
   }
}


