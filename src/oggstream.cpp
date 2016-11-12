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

#if KOSOUND_HAS_OGRE == 1
   #include <OGRE/OgreResourceGroupManager.h>
#endif

using namespace Kosound;

#ifdef __APPLE__
   #define KOSOUND_OGG_BUFFER_SIZE (4096 * 16) /**< Size of the Ogg Buffer */
#else
   #define KOSOUND_OGG_BUFFER_SIZE (4096 * 16) /**< Size of the Ogg Buffer */
#endif

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS && KOSOUND_HAS_OGRE == 1
   #include <OGRE/iOS/macUtils.h>
#endif


#if KOSOUND_HAS_OGRE == 1
/*************************************************************************
 *                         ogre_stream_read_func                         *
 *************************************************************************/
static size_t ogre_stream_read_func(void* ptr, size_t size, size_t nmemb, 
      void* datasource)
{
   Ogre::DataStreamPtr* dataStream = (Ogre::DataStreamPtr*) datasource;
   if(!(*dataStream)->eof())
   {
      return (*dataStream)->read(ptr, size * nmemb);
   }

   return 0;
}

/*************************************************************************
 *                         ogre_stream_seek_func                         *
 *************************************************************************/
static int ogre_stream_seek_func(void* datasource, ogg_int64_t offset, 
      int whence)
{
   Ogre::DataStreamPtr* dataStream = (Ogre::DataStreamPtr*) datasource;
   
   if(whence == SEEK_SET)
   {
      /* We are seeking from the start position */
      (*dataStream)->seek(offset);
      return 0;
   }
   else if((whence == SEEK_CUR) && (offset == 0))
   {
      /* Should be a rewind. */
      (*dataStream)->seek(offset);
      return 0;
   }
   /*else
   {
      Ogre::LogManager::getSingleton().stream(Ogre::LML_CRITICAL)
         << "seek: offset: '" << offset << "' whence: '" 
         << whence << "': SEEK_SET: " << SEEK_SET 
         << " SEEK_CUR: " << SEEK_CUR << " SEEK_END: " << SEEK_END; 
   }*/
   
   //TODO
   return -1;
}

/*************************************************************************
 *                       ogre_stream_close_func                          *
 *************************************************************************/
static int ogre_stream_close_func(void* datasource)
{
   Ogre::DataStreamPtr* dataStream = (Ogre::DataStreamPtr*) datasource;
   (*dataStream)->close();
   return 0;
}

/*************************************************************************
 *                         ogre_stream_tell_func                         *
 *************************************************************************/
static long int ogre_stream_tell_func(void* datasource)
{
   Ogre::DataStreamPtr* dataStream = (Ogre::DataStreamPtr*) datasource;
   return (*dataStream)->tell();
}

/*************************************************************************
 *                         ogre_stream_callbacks                         *
 *************************************************************************/
static ov_callbacks OGRE_STREAM_CALLBACK = {
   ogre_stream_read_func,
   ogre_stream_seek_func,
   ogre_stream_close_func,
   ogre_stream_tell_func
};
#endif

/*************************************************************************
 *                             OggStream                                 *
 *************************************************************************/
OggStream::OggStream()
          :SoundStream(SoundStream::TYPE_OGG, KOSOUND_OGG_BUFFER_SIZE)
{
}

/*************************************************************************
 *                             ~OggStream                                *
 *************************************************************************/
OggStream::~OggStream()
{
}

/*************************************************************************
 *                                 open                                  *
 *************************************************************************/
bool OggStream::_open(Kobold::String path, ALenum* f, ALuint* sr)
{
   int result;

#if KOSOUND_HAS_OGRE == 1
   try
   {
      stream = Ogre::ResourceGroupManager::getSingleton().openResource(path);
   }
   catch(Ogre::FileNotFoundException)
   {
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR,
         "OggStream: Couldn't open ogg file from resources: '%s'", 
         path.c_str());
      return false;
   }

   result = ov_open_callbacks((void*)&stream, &oggStr, NULL, 0, 
         OGRE_STREAM_CALLBACK);
#else
   //if(!(oggFile = fopen(dir.getRealFile(path).c_str(), "rb")))
   if(!(oggFile = fopen(path.c_str(), "rb")))
   {
      //cerr << "Could not open Ogg file: " <<  dir.getRealFile(path) << endl;
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR, 
	  "Could not open Ogg file: %s", path.c_str());
      return false;
   }

   #if defined(_MSC_VER)
      result = ov_open_callbacks(oggFile, &oggStr, 
            NULL, 0, OV_CALLBACKS_DEFAULT);
   #else
      result = ov_open(oggFile, &oggStr, NULL, 0);
   #endif

#endif

   if(result < 0)
   {
#if KOSOUND_HAS_OGRE == 1
      stream->close();
#else
      fclose(oggFile);
#endif
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

   #if KOSOUND_HAS_OGRE == 1
      result = ov_read(&oggStr, bufferData+index, readBytes,
            (OGRE_ENDIAN == OGRE_ENDIAN_BIG),2,1,&section); 
   #else
      //FIXME: assuming little-endian.
      result = ov_read(&oggStr, bufferData+index, readBytes,
            0, 2, 1,&section); 
   #endif
#else
   result = ov_read(&oggStr, (char*)&bufferData[index], readBytes,
                    &section);
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


