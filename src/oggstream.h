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

#ifndef _kosound_ogg_stream_h
#define _kosound_ogg_stream_h

#include "kosoundconfig.h"
#include <kobold/platform.h>
#include <kobold/kstring.h>

#if KOSOUND_HAS_OGRE == 1
   #include <OGRE/OgreDataStream.h>
#else
   #include <stdio.h>
#endif

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS
   #include <OpenAL/al.h>
#else
   #include <AL/al.h>
#endif

#include <ogg/ogg.h>

#if KOBOLD_PLATFORM != KOBOLD_PLATFORM_IOS && \
    KOBOLD_PLATFORM != KOBOLD_PLATFORM_ANDROID
   #include <vorbis/codec.h>
   #include <vorbis/vorbisenc.h>
   #include <vorbis/vorbisfile.h> 
#else
   #include <tremor/ivorbiscodec.h>
   #include <tremor/ivorbisfile.h>
#endif

#include "soundstream.h"

namespace Kosound
{

/*! The OGG Input Stream Class */
class OggStream : public SoundStream
{
   public:
      /*! Constructor */
      OggStream();
      /*! Destructor */
      ~OggStream();
      
   protected:
      /*! Open the Ogg file to use */
      bool _open(Kobold::String fName, ALenum* f, ALuint* sr);

      /*! Release specific Ogg structures */
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

      /*! Error code from ogg
       * \param code -> numer of error
       * \return string relative to the error */
      Kobold::String errorString(int code); 
      
   private:
     #if KOBOLD_HAS_OGRE == 1
        Ogre::DataStreamPtr stream;    /**< Data stream related to the file. */ 
     #else
        FILE* oggFile;
     #endif
        OggVorbis_File oggStr;         /**< stream handle */
        vorbis_info* vorbisInfo;       /**< some formatting data */
        vorbis_comment* vorbisComment; /**< user comments */
};

}

#endif


