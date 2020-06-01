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

#include "soundstream.h"
#include <kobold/log.h>

using namespace Kosound;

/***********************************************************************
 *                           Constructor                               *
 ***********************************************************************/
SoundStream::SoundStream(const SoundStreamType& t, unsigned long bufSize)
{
   type = t;

   bufferSize = bufSize;
   bufferData = new char[bufferSize];
   
   sampleRate = 44100;
   format = AL_FORMAT_STEREO16;

   opened = false;
   ended = false;
}

/***********************************************************************
 *                            Destructor                               *
 ***********************************************************************/
SoundStream::~SoundStream()
{
   release();
   delete []bufferData;
}

/*************************************************************************
 *                                 open                                  *
 *************************************************************************/
bool SoundStream::open(const Kobold::String& fName)
{
   if(opened)
   {
      /* Must avoid double opens */
      return false;
   }
   
   fileName = fName;

   /* Really open the file */
   if(_open(fName, &format, &sampleRate))
   {
      opened = true;

      /* Create the OpenAL Buffers */
      alGenBuffers(2, buffers);
      check("::open() -> alGenBuffers");
      alGenSources(1, &source);
      check("::open() -> alGenSouces");

      return true;
   }

   return false;
}

/*************************************************************************
 *                             defineAsMusic                             *
 *************************************************************************/
void SoundStream::defineAsMusic()
{
   if(opened)
   {
      alSource3f(source, AL_POSITION, 0.0, 0.0, 0.0);
      alSource3f(source, AL_VELOCITY, 0.0, 0.0, 0.0);
      alSource3f(source, AL_DIRECTION, 0.0, 0.0, 0.0);
      alSourcef(source, AL_ROLLOFF_FACTOR, 0.0);
      alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
   }
}

/*************************************************************************
 *                              getSource                                *
 *************************************************************************/
ALuint SoundStream::getSource()
{
   return source;
}

/*************************************************************************
 *                                release                                *
 *************************************************************************/
void SoundStream::release()
{
   if(opened)
   {
      /* Stop Source (if already not stoped) */
      if(!ended)
      {
         alSourceStop(source);
         check("::release() alSourceStop");
      }
      
      /* Empty the remaining buffers */
      empty();
      
      /* Delete Sources And Buffers */
      alDeleteSources(1, &source);
      check("::release() alDeleteSources");
      alDeleteBuffers(2, &buffers[0]);
      check("::release() alDeleteBuffers");
  
      /* Release internal elements */
      _release();

      opened = false;
   }
}

/*************************************************************************
 *                              playBack                                 *
 *************************************************************************/
bool SoundStream::playback(bool rw)
{
   int numBuffers = 2;
   if(opened)
   {
      if( (isPlaying()) && (!rw) )
      {
         return true;
      }
      
      if( (isPlaying()) && (rw) )
      {
         /* Must stop Buffer */
         alSourceStop(source);
         check("::playBack() alSourceStop");
         empty();
      }
      
      if(!stream(buffers[0], rw))
      {
         return false;
      }
      
      if(!stream(buffers[1]))
      {
         /* Only needed a single buffer. */
         numBuffers = 1;
      }
      
      alSourceQueueBuffers(source, numBuffers, buffers);
      alSourcePlay(source);
      
      return true;
   }
   return false;
}

/*************************************************************************
 *                               playing                                 *
 *************************************************************************/
bool SoundStream::isPlaying()
{
   ALenum state;

   if(opened)
   {
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      return state == AL_PLAYING;
   }
   return false;
}

/*************************************************************************
 *                               rewind                                  *
 *************************************************************************/
bool SoundStream::rewind()
{
   if(opened)
   {
      return playback(true);
   }
   return false;
}

/*************************************************************************
 *                               update                                  *
 *************************************************************************/
bool SoundStream::update()
{
   int processed;
   bool active = true;
   
   if(opened)
   {
      alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
      
      /* Must verify if sound is pending for loop */
      if( (processed == 0) && (ended) )
      {
         /* Verify Waiting to loop again time */
         if((loopInterval > 0) && (ended))
         {
            if( (int) (loopTimer.getMilliseconds() / 1000) >= loopInterval)
            {
               /* Reinit the play, rewinding the file */
               active = playback(true);
            }
         }
      }
      
      /* Process the processed buffers */
      while(processed--)
      {
         ALuint buffer;
         
         alSourceUnqueueBuffers(source, 1, &buffer);
         check("::update() alSourceUnqueueBuffers");
         
         /* Only stream if active (sometimes the previous buffer already
          * inactive the stream). */
         if(active)
         {
            active = stream(buffer);
         
            if( (active) && (!ended))
            {
               /* Only Queue if stream is active, and not waiting */
               alSourceQueueBuffers(source, 1, &buffer);
               check("::update() alSourceQueueBuffers");
            }
         }
      }
      
      return active;
      
   }
   return false;
}

/*************************************************************************
 *                               stream                                  *
 *************************************************************************/
bool SoundStream::stream(ALuint buffer, bool rw)
{
   bool gotEof = false;
   unsigned long bytesReaded=0;
   unsigned long totalBytesReaded=0;
   unsigned long readBytes;

   if(rw)
   {
      /* Must restart the stream */
      ended = false;

      /* Rewind the file */
      if(!_rewind())
      {
         return false;
      }
   }
   else if(ended)
   {
      if(isPlaying())
      {
         alSourceStop(source);
         check("::playBack() alSourceStop");
      }
      /* Must only wait. Done if no more plays */
      return loopInterval >= 0;
   }

   /* Get the buffer */
   readBytes = bufferSize;

   while( (totalBytesReaded < bufferSize) && (!ended) )
   {
      if(!_getBuffer(totalBytesReaded, readBytes, &bytesReaded, &gotEof))
      {
         Kobold::Log::add(Kobold::String("SoundStream::stream(): ") +
               Kobold::String("Couldn't _getBuffer()."));
         return false;
      }

      totalBytesReaded += bytesReaded;
      readBytes = bufferSize - totalBytesReaded;
      if(gotEof)
      {
         if(loopInterval == 0)
         {
            /* Auto Rewind file */
            if(!_rewind())
            {
               return false;
            }
         }
         else if(loopInterval > 0)
         {
            /* Start timer before reload */
            ended = true;
            loopTimer.reset();
         }
         else
         {
            /* Never loop */
            ended = true;
         }
      }
   }
  
   if(totalBytesReaded > 0)
   {
      alBufferData(buffer, format, bufferData, totalBytesReaded, sampleRate);
      check("::stream() alBufferData");
   }
   else if(ended)
   {
      alSourceStop(source);
      check("::playBack() alSourceStop");
   }
   
   return true;
}

/*************************************************************************
 *                            changeVolume                               *
 *************************************************************************/
void SoundStream::changeVolume(int volume)
{
   alSourcef(source, AL_GAIN, (float)(volume / 128.0));
}

/*************************************************************************
 *                               setLoop                                 *
 *************************************************************************/
void SoundStream::setLoop(int lp)
{
   loopInterval = lp;
}


/*************************************************************************
 *                                 empty                                 *
 *************************************************************************/
void SoundStream::empty()
{
   if(opened)
   {
      int queued;
      
      alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
      check("::empty() AL_BUFFERS_QUEUED");
      
      while(queued--)
      {
         ALuint buffer;
         
         alSourceUnqueueBuffers(source, 1, &buffer);
         check("::empty() alSourceUnqueueBuffers");
      }
   }
}

/*************************************************************************
 *                                 check                                 *
 *************************************************************************/
void SoundStream::check(const Kobold::String& where)
{
    int error = alGetError();
 
    if(error != AL_NO_ERROR)
    {
        Kobold::Log::add(Kobold::LOG_LEVEL_ERROR, 
              "SoundStream: OpenAL error was raised!");
        switch(error)
        {
           case AL_INVALID_NAME: 
              Kobold::Log::add("Invalid name parameter");
           break;
           case AL_INVALID_ENUM: 
              Kobold::Log::add("Invalid parameter");
           break;
           case AL_INVALID_VALUE: 
              Kobold::Log::add("Invalid enum parameter value");
           break;
           case AL_INVALID_OPERATION: 
              Kobold::Log::add("Illegal call");
           break;
           case AL_OUT_OF_MEMORY: 
              Kobold::Log::add("Unable to allocate memory");
           break;
        }
       Kobold::Log::add(Kobold::String("At: ")+where);
    }
}



