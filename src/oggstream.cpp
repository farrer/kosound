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

#include "oggstream.h"
#include <kobold/log.h>
#include <SDL2/SDL.h>

#define KOSOUND_OGG_BUFFER_SIZE (4096 * 16) /**< Size of the Ogg Buffer */

namespace Kosound
{

/*************************************************************************
 *                         kosound_stream_read_func                         *
 *************************************************************************/
static size_t kosound_stream_read_func(void* ptr, size_t size, size_t nmemb, 
      void* datasource)
{
   Kobold::FileReader* fileReader = 
      static_cast<Kobold::FileReader*>(datasource);
   if(!fileReader->eof())
   {
      return fileReader->read(static_cast<char*>(ptr), size * nmemb);
   }

   return 0;
}

/*************************************************************************
 *                         kosound_stream_seek_func                         *
 *************************************************************************/
static int kosound_stream_seek_func(void* datasource, ogg_int64_t offset, 
      int whence)
{
   Kobold::FileReader* fileReader = 
      static_cast<Kobold::FileReader*>(datasource);
   
   if(whence == SEEK_SET)
   {
      /* We are seeking from the start position */
      fileReader->seek(offset);
      return 0;
   }
   else if((whence == SEEK_CUR) && (offset == 0))
   {
      /* Should be a rewind. */
      fileReader->seek(offset);
      return 0;
   }
   
   return -1;
}

/*************************************************************************
 *                       kosound_stream_close_func                          *
 *************************************************************************/
static int kosound_stream_close_func(void* datasource)
{
   Kobold::FileReader* fileReader = 
      static_cast<Kobold::FileReader*>(datasource);
   fileReader->close();

   return 0;
}

/*************************************************************************
 *                         kosound_stream_tell_func                         *
 *************************************************************************/
static long int kosound_stream_tell_func(void* datasource)
{
   Kobold::FileReader* fileReader = 
      static_cast<Kobold::FileReader*>(datasource);
   return fileReader->tell();
}

/*************************************************************************
 *                         kosound_stream_callbacks                         *
 *************************************************************************/
static ov_callbacks KOSOUND_STREAM_CALLBACK = {
   kosound_stream_read_func,
   kosound_stream_seek_func,
   kosound_stream_close_func,
   kosound_stream_tell_func
};

/*************************************************************************
 *                             OggStream                                 *
 *************************************************************************/
OggStream::OggStream(Kobold::FileReader* fileReader)
          :SoundStream(SoundStream::TYPE_OGG, KOSOUND_OGG_BUFFER_SIZE)
{
   this->fileReader = fileReader;
}

/*************************************************************************
 *                             ~OggStream                                *
 *************************************************************************/
OggStream::~OggStream()
{
   if(this->fileReader != NULL)
   {
      delete fileReader;
   }
}

/*************************************************************************
 *                                 open                                  *
 *************************************************************************/
bool OggStream::_open(const Kobold::String& path, ALenum* f, ALuint* sr)
{
   int result;

   if(!fileReader->open(path))
   {
      Kobold::Log::add(Kobold::LOG_LEVEL_ERROR,
         "OggStream: Couldn't open ogg file from resources: '%s'", 
         path.c_str());
      return false;
   }

   result = ov_open_callbacks((void*)fileReader, &oggStr, NULL, 0, 
         KOSOUND_STREAM_CALLBACK);

   if(result < 0)
   {
      fileReader->close();
      Kobold::Log::add(Kobold::String("OggStream::_open(): ") +
            Kobold::String("Could not open Ogg stream: '") +
            errorString(result));
      return false;
   }

   vorbisInfo = ov_info(&oggStr, -1);
   vorbisComment = ov_comment(&oggStr, -1);

   /* Set format */
   if(vorbisInfo->channels == 1)
   {
       *f = AL_FORMAT_MONO16;
   }
   else
   {
       *f = AL_FORMAT_STEREO16;
   }

   /* Set sampleRate */
   *sr = vorbisInfo->rate;

   return true;
}

/*************************************************************************
 *                                release                                *
 *************************************************************************/
void OggStream::_release()
{
   /* Clear the ogg stream */
   ov_clear(&oggStr);
}

/*************************************************************************
 *                                rewind                                 *
 *************************************************************************/
bool OggStream::_rewind()
{
   /* Rewind the file */
   if(ov_raw_seek(&oggStr,0) != 0)
   {
      Kobold::Log::add(Kobold::String("OggStream::stream(): ")+
            Kobold::String("Ogg Rewind Error!"));
      return false;
   }

   return true;
}


/*************************************************************************
 *                                stream                                 *
 *************************************************************************/
bool OggStream::_getBuffer(unsigned long index, unsigned long readBytes, 
      unsigned long* bytesReaded, bool* gotEof)
{
   int  section;
   int  result = -1;

   *gotEof = false;
   *bytesReaded = 0;
   
   /* Try to read from ogg file */
#if KOBOLD_PLATFORM != KOBOLD_PLATFORM_IOS && \
    KOBOLD_PLATFORM != KOBOLD_PLATFORM_ANDROID
   result = ov_read(&oggStr, bufferData+index, readBytes,
         SDL_BYTEORDER == SDL_BIG_ENDIAN, 2, 1, &section); 
#else
   result = ov_read(&oggStr, (static_cast<char*>(&bufferData[index]), 
            readBytes, &section);
#endif
   if(result < 0)
   {
      /* Error */
      Kobold::Log::add(Kobold::String("Ogg Buffer Error: ") +
            errorString(result));
      return false;
   }
   else if(result == 0)
   {
      /* Got EOF */
      *gotEof = true;
   }

   *bytesReaded = result;
   return true;
}

/*************************************************************************
 *                                errorString                            *
 *************************************************************************/
Kobold::String OggStream::errorString(int code)
{
    switch(code)
    {
        case OV_EREAD:
            return Kobold::String("Read from media.");
        case OV_ENOTVORBIS:
            return Kobold::String("Not Vorbis data.");
        case OV_EVERSION:
            return Kobold::String("Vorbis version mismatch.");
        case OV_EBADHEADER:
            return Kobold::String("Invalid Vorbis header.");
        case OV_EFAULT:
            return Kobold::String("Internal logic fault.");
        default:
            return Kobold::String("Unknown Ogg error.");
    }
}


}

