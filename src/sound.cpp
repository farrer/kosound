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

#include "sound.h"
#include <kobold/log.h>

#if KOSOUND_HAS_OGRE == 1
   #include <OGRE/OgreMath.h>
   #if OGRE_VERSION_MAJOR == 1
      #include <OGRE/Ogre.h>
   #else
      #include <OGRE/OgreVector3.h>
   #endif
#else
   #include <math.h>

   #define PID180 M_PI / 180.0 /**< PI / 180 definition */
   inline double deg2Rad(double x){ return PID180 * x; }
#endif

using namespace Kosound;

#define KOBOLD_SOUND_UPDATE_RATE   100


/*************************************************************************
 *                                Init                                   *
 *************************************************************************/
void Sound::init()
{
   enabled = true;
   
   /* None current Opened Music */
   backMusic = NULL;

   musicVolume = DEFAULT_VOLUME;
   sndfxVolume = DEFAULT_VOLUME;

   if(!initOpenAL())
   {
      return;
   }
}

/*************************************************************************
 *                             initOpenAL                                *
 *************************************************************************/
bool Sound::initOpenAL()
{
   /* Initialize Open AL */
   device = alcOpenDevice(NULL); 
   
   if (device != NULL) 
   {
      context=alcCreateContext(device,NULL); 
      if (context != NULL) 
      {
         alcMakeContextCurrent(context);
         enabled = true;
         /* set attenuation model */
         alDistanceModel(AL_EXPONENT_DISTANCE);
         return true;
      }
      else
      {
         Kobold::Log::add("Sound::initOpenAL() Couldn't create context!");
      }

   }
   else
   {
      Kobold::Log::add("Sound::initOpenAL() No OpenAL device available!");
   }
   enabled = false;
   timer.reset();
   return false;
}

/*************************************************************************
 *                               Finish                                  *
 *************************************************************************/
void Sound::finish()
{
   if(enabled)
   {
      finishOpenAL();
   }
}

/*************************************************************************
 *                            finishOpenAL                               *
 *************************************************************************/
void Sound::finishOpenAL()
{
   /* Clear the Opened Music */
   if(backMusic)
   {
      delete(backMusic);
      backMusic = NULL;
   }

   /* Clear all opened Sound Effects */
   removeAllSoundEffects();

   /* Clear OpenAL Context and Device */
   alcDestroyContext(context);
   alcCloseDevice(device);
}

/*************************************************************************
 *                          setListenerPosition                          *
 *************************************************************************/
void Sound::setListenerPosition(ALfloat centerX, ALfloat centerY, 
                                ALfloat centerZ, ALfloat phi, ALfloat theta,
                                ALfloat d)
{
   if(enabled)
   {
      float posX, posY, posZ;   /* Listener Position */
      ALfloat directionvect[6]; /* Direction Vector of Listener */
      
      	
      alListener3f(AL_POSITION, centerX, centerY, centerZ);

#if KOSOUND_HAS_OGRE == 1
      Ogre::Radian thetaR = Ogre::Radian(Ogre::Degree(theta));
      Ogre::Radian phiR = Ogre::Radian(Ogre::Degree(phi));
      
      Ogre::Real cosTheta = Ogre::Math::Cos(thetaR);
      Ogre::Real sinTheta = Ogre::Math::Sin(thetaR);
      
      Ogre::Real cosPhi = Ogre::Math::Cos(phiR);
      Ogre::Real sinPhi = Ogre::Math::Sin(phiR);
#else
      float thetaR = deg2Rad(theta);
      float phiR = deg2Rad(phi);

      float cosTheta = cos(thetaR);
      float sinTheta = sin(thetaR);
      
      float cosPhi = cos(phiR);
      float sinPhi = sin(phiR);

#endif
      
      posX = centerX + d * cosTheta * sinPhi;
      posY = centerY + d * sinTheta;
      posZ	= centerZ + d * cosTheta * cosPhi;
      
      directionvect[0] = centerX - posX;
      directionvect[1] = centerY - posY;
      directionvect[2] = centerZ - posZ;
      directionvect[3] = 0;
      directionvect[4] = 1;
      directionvect[5] = 0;
      alListenerfv(AL_ORIENTATION, directionvect);
   }
}

/*************************************************************************
 *                              loadMusic                                *
 *************************************************************************/
bool Sound::loadMusic(const Kobold::String& fileName)
{
   if(!enabled)
   {
      return false;
   }
      
   if(backMusic)
   {
      /* Delete Active Music, if one is */
      delete(backMusic);
      backMusic = NULL;
   }

   /* Load The File and Set The active Music */
   backMusic = new SndFx(0, fileName);
   backMusic->changeVolume(musicVolume);

   backMusic->setLoop(SOUND_AUTO_LOOP);
   backMusic->defineAsMusic();
   
   return true;
}

/*************************************************************************
 *                                flush                                  *
 *************************************************************************/
void Sound::flush()
{
   SndFx* snd, *tmp;
   int i, total;

   if(!enabled)
   {
      return;
   }
   
   if(timer.getMilliseconds() < KOBOLD_SOUND_UPDATE_RATE)
   {
      return;
   }
   
   /* Music Update */
   if(backMusic)
   {
      if(!backMusic->update()) 
      {
         Kobold::Log::add("Sound::flush: Error while playing music");
         delete backMusic;
         backMusic = NULL;
      }
   }

   /* Sound Effects Update */
   total = sndList.getTotal();
   snd = (SndFx*)sndList.getFirst();
   for(i=0; i < total; i++)
   {
      if( (!snd->update()) && (snd->getRemoval()) )
      {
         /* Remove Sound */
         tmp = snd;
         snd = (SndFx*)snd->getNext();
         removeSoundEffect(tmp);
      }
      else
      {
         snd = (SndFx*)snd->getNext();
      }
   }
}

/*************************************************************************
 *                            addSoundEffect                             *
 *************************************************************************/
SndFx* Sound::addSoundEffect(ALfloat x, ALfloat y, ALfloat z, int loop,
      const Kobold::String& fileName)
{
   SndFx* snd = NULL;

   if(enabled)
   {
      /* Create it */
      snd = new SndFx(x,y,z,loop, fileName);
      snd->changeVolume(sndfxVolume);

      /* Insert on the list */
      sndList.insert(snd);
   }

   return snd;
}

/*************************************************************************
 *                            addSoundEffect                             *
 *************************************************************************/
SndFx* Sound::addSoundEffect(int loop, const Kobold::String& fileName)
{
   SndFx* snd = NULL;

   if(enabled)
   {
      /* Create it */
      snd = new SndFx(loop, fileName);
      snd->changeVolume(sndfxVolume);

      /* Insert on the list */
      sndList.insert(snd);
   }

   return snd;
}


/*************************************************************************
 *                          removeSoundEffect                            *
 *************************************************************************/
void Sound::removeSoundEffect(SndFx* snd)
{
   if( (enabled) && (snd != NULL) )
   {
      sndList.remove(snd);
   }
}

/*************************************************************************
 *                        removeAllSoundEffects                          *
 *************************************************************************/
void Sound::removeAllSoundEffects()
{
   /* Clear all opened Sound Effects */
   sndList.clearList();
}


/*************************************************************************
 *                              changeVolume                             *
 *************************************************************************/
void Sound::changeVolume(int music, int sndV)
{
   SndFx* snd;
   int i;
   
   if(enabled)
   {
      /* Updata values */
      musicVolume = music;
      sndfxVolume = sndV;
      
      /* Update backmusic */
      if(backMusic)
      {
         backMusic->changeVolume(musicVolume);
      }
      
      /* Update all current Sounds */
      snd = (SndFx*)sndList.getFirst();
      for(i=0; i < sndList.getTotal(); i++)
      {
         snd->changeVolume(sndfxVolume);
         snd = (SndFx*)snd->getNext();
      }
   }
}

/*************************************************************************
 *                             static members                            *
 *************************************************************************/
ALCdevice* Sound::device;         /**< Active AL device */
ALCcontext* Sound::context;       /**< Active AL context */
SndFx* Sound::backMusic;         /**< Active BackGround Music */

bool Sound::enabled;              /**< If Sound is Enabled or Not */

Kobold::List Sound::sndList;         /**< sndFx List */

int Sound::musicVolume;           /**< The Music volume */
int Sound::sndfxVolume;           /**< The SndFxVolume */
Kobold::Timer Sound::timer;

