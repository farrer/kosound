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

#ifndef _kosound_caf_stream_h
#define _kosound_caf_stream_h

#include "kosoundconfig.h"
#include <kobold/platform.h>

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS

#include <OpenAL/al.h>
#include <AudioToolbox/AudioToolbox.h>

#include <kobold/kstring.h>

#include "soundstream.h"

namespace Kosound
{

/*! The CAF Input Stream Class.
 * \note Only available at apple systems (MacOS & iOS) */
   class CafStream: public SoundStream
{
   public:
      /*! Constructor */
      CafStream();
      /*! Destructor */
      virtual ~CafStream();
   
   protected:
      /*! Implementation of the open on the specific sound file */
      bool _open(const Kobold::String& fName, ALenum* f, ALuint* sr);
   
      /*! Release all specific implementation allocs */
      void _release();
   
      /*! Rewind the stream
       * \return true on success */
      bool _rewind();
   
      /*! Read data to the internal buffer
       * \param index -> first position on the buffer to fill
       * \param readBytes -> total bytes to read (could read less)
       * \param bytesReaded -> return total bytes really readed on this call
       * \param gotEof -> return true if got EOF
       * \return false if some error occurred */
      bool _getBuffer(unsigned long index, unsigned long readBytes, 
                      unsigned long* bytesReaded, bool* gotEof);
   
      /*! Error code
       * \param code -> numer of error
       * \return string relative to the error */
      Kobold::String errorString(int code); 
      
   private:
      ExtAudioFileRef extAudioFile;  /**< The audio file used */

      AudioStreamBasicDescription outputFormat; /**< format of the output */
      UInt32 totalFrames;   /**< Total Frames of the input stream */
      UInt32 dataSize;      /**< The size of the input stream */
      SInt64 initialFrameOffset; /**< The offset to initial frame position */
};

}

#endif

#endif


