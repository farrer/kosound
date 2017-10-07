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

#ifndef _kosound_sound_stream_h
#define _kosound_sound_stream_h

#include "kosoundconfig.h"
#include <kobold/platform.h>

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS ||\
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS
   #include <OpenAL/al.h>
#else
   #include <AL/al.h>
#endif

#include <kobold/kstring.h>

#include <kobold/timer.h>

namespace Kosound
{

/*! The SoundStream class is a generic implementation of a sound stream.
 * All specific sound formats must derive from this one. */
class SoundStream
{
   public:
      enum SoundStreamType
      {
         TYPE_CAF=0,
         TYPE_OGG
      };

      /*! Constructor
       * \param t -> SoundStream type constant
       * \param bufSize -> size of the read buffer */ 
      SoundStream(const SoundStreamType& t, unsigned long bufSize);
      /*! Destructor
       * \note -> the destructor will call _release() */
      virtual ~SoundStream();

      /*! Open the Stream to use
       * \param fName -> name of sound file to load
       * \return true if successfully loaded, false otherwise */
      bool open(const Kobold::String& fName);

      /*! Define the stream as Music (no position and no atenuation) */
      void defineAsMusic();

      /*! Release all buffers and sources relative to the file */
      void release();         
      
      /*! playback the caf stream
       * \param rw -> if true, rewind the caf file
       * \return false if is not playing */
      bool playback(bool rw=false); 
      
      /*! Verify if the source is playing
       * \return false if is not playing */
      bool isPlaying();         
      
      /*! Update the stream to OpenAL buffers
       * \return false if stream is over */
      bool update();

      /*! Rewind the sound to play again */
      bool rewind();
      
      /*! Get The Source
       * \return AL source of the caf */
      ALuint getSource();

      /*! Change the stream overall volume 
       * \param volume -> volume value [0,128]*/
      void changeVolume(int volume);

      /*! Set if will Loop at EOF or not
       * \param lp -> loop interval (< 0 won't loop, =0 loop just at 
       *              the EOF, >0 wait lp seconds before loop) */
      void setLoop(int lp);

      /*! Get the stream type */
      const SoundStreamType& getType(){ return type; };

   protected:
      /*! Stream the file to the OpenAL buffer
       * \param buffer -> buffer to reload 
       * \param rw -> if true, force a file rewind
       * \return false if stream is over, or error happened */
      bool stream(ALuint buffer, bool rw=false);

      /*! Implementation of the open on the specific sound file
       * \note: Implementation must set the OpenAL format (f)
       *        to either AL_FORMAT_STEREO16 or AL_FORMAT_MONO16
       * \note: Must set the sampleRate (sr) too. */
      virtual bool _open(const Kobold::String& fName, ALenum* f, ALuint* sr)=0;

      /*! Release all specific implementation allocs */
      virtual void _release()=0;

      /*! Rewind the stream
       * \return true on success */
      virtual bool _rewind()=0;

      /*! Read data to the internal buffer
       * \param index -> first position on the buffer to fill
       * \param readBytes -> total bytes to read (could read less)
       * \param bytesReaded -> return total bytes really readed on this call
       * \param gotEof -> return true if got EOF
       * \return false if some error occurred */
      virtual bool _getBuffer(unsigned long index, unsigned long readBytes, 
            unsigned long* bytesReaded, bool* gotEof)=0;

      /*! Empty the queue */
      void empty();      

      /*! Check OpenAl errors
       * \param where -> string with information about 
       *                 where the check occurs */
      void check(const Kobold::String& where); 

      Kobold::String fileName; /**< Filename of the sound stream */

      char* bufferData;         /**< The buffer used to read */
      unsigned long bufferSize; /**< Size of the buffer */


   private:
      SoundStreamType type;  /**< Sound stream type */

      bool opened;        /**< If caf was opened or not */
      bool ended;         /**< If play ended or not */

      int loopInterval;   /**< Number of seconds before next loop */
      Kobold::Timer loopTimer;    /**< Timer to next loop */

      ALuint buffers[2]; /**< front and back buffers */
      ALuint source;     /**< audio source */
      ALenum format;     /**< internal format */
      ALuint sampleRate; /**< input/output sample rate */

      
};


}

#endif

