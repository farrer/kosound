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

#include "kosoundconfig.h"
#include "cafstream.h"

#include <kobold/platform.h>
#include <kobold/log.h>

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_MACOS || \
    KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS

#include <kobold/macutils.h>

using namespace Kosound;

#if KOBOLD_PLATFORM == KOBOLD_PLATFORM_IOS
   #define CAF_BUFFER_SIZE (4096 * 16)  /**< Size of the stream buffer */
#else
   #define CAF_BUFFER_SIZE (4096 * 16) /**< Size of the stream Buffer */
#endif

/*************************************************************************
 *                               Constructor                             *
 *************************************************************************/
CafStream::CafStream()
          :SoundStream(SoundStream::TYPE_CAF, CAF_BUFFER_SIZE)
{
   extAudioFile = NULL;
}

/*************************************************************************
 *                              Destructor                               *
 *************************************************************************/
CafStream::~CafStream()
{
}

/*************************************************************************
 *                                 open                                  *
 *************************************************************************/
bool CafStream::_open(const Kobold::String& fName, ALenum* f, ALuint* sr)
{
   AudioStreamBasicDescription inputFormat;
   SInt64 fileFrames = 0;
   UInt32 propSize = sizeof(inputFormat);
   
   initialFrameOffset = 0;
   
   /* First convert path (full with mucBundle) to CFStringRef */
   CFStringEncoding encoding = kCFStringEncodingMacRoman; // =  0;
	CFAllocatorRef alloc_default = kCFAllocatorDefault;  // = NULL;
   CFStringRef pathC = CFStringCreateWithCString(alloc_default, 
      ((Kobold::macBundlePath()+Kobold::String("/")+fName).c_str()), encoding);
   
   /* Now create the URL Ref (ouch, that's pretty ugly!) */
   CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
         pathC, kCFURLPOSIXPathStyle, false);
   
   /* Finally, open the audio file */
   if(ExtAudioFileOpenURL(fileURL, &extAudioFile) )
   {
      /* Couldn't open file */
      CFRelease(pathC);
      CFRelease(fileURL);
      
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR,
                       "CafStream::open() Couldn't open: '%s'",
                       (Kobold::macBundlePath() + fName).c_str());
      return false;
   }
   
   /* Clean up string things */
   CFRelease(pathC);
   CFRelease(fileURL);
   
   /* Get format from input file */
   ExtAudioFileGetProperty(extAudioFile, kExtAudioFileProperty_FileDataFormat, 
                           &propSize, &inputFormat);
   
   if( (inputFormat.mChannelsPerFrame > 2) || 
       (inputFormat.mChannelsPerFrame <= 0) )
   {
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR,
               "CafStream::open(): Too many channels for '%s'",
               fileName.c_str());
      return false;
   }
   
   /* Set OpenAL Format */
   if(outputFormat.mChannelsPerFrame > 1)
   {
      *f = AL_FORMAT_STEREO16;
   }
   else
   {
      *f = AL_FORMAT_MONO16;
   }
   *sr = inputFormat.mSampleRate;
   
   /* Set format for output (16 bit signed integer, native-endian) */
   outputFormat.mSampleRate = inputFormat.mSampleRate;
   outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame; 
   outputFormat.mFormatID = kAudioFormatLinearPCM;
   outputFormat.mBytesPerPacket = 2 * outputFormat.mChannelsPerFrame;
   outputFormat.mFramesPerPacket = 1;
   outputFormat.mBytesPerFrame = 2 * outputFormat.mChannelsPerFrame;
   outputFormat.mBitsPerChannel = 16;
   outputFormat.mFormatFlags = kAudioFormatFlagsNativeEndian |
                               kAudioFormatFlagIsSignedInteger |
                               kAudioFormatFlagIsPacked;
   ExtAudioFileSetProperty(extAudioFile, kExtAudioFileProperty_ClientDataFormat,
                           sizeof(outputFormat), &outputFormat);
   
   /* Calculate total data size */
   propSize = sizeof(fileFrames);
   ExtAudioFileGetProperty(extAudioFile, kExtAudioFileProperty_FileLengthFrames, 
                           &propSize, &fileFrames);
   
   totalFrames = (UInt32)fileFrames;     
   dataSize = totalFrames * outputFormat.mBytesPerFrame;
   
   /* Get the initial read position (to make possible rewinds latter) */
   ExtAudioFileTell(extAudioFile, &initialFrameOffset);

   return true;
}

/*************************************************************************
 *                              _release                                 *
 *************************************************************************/
void CafStream::_release()
{
   ExtAudioFileDispose(extAudioFile);
}

/*************************************************************************
 *                              _release                                 *
 *************************************************************************/
bool CafStream::_rewind()
{
   /* Rewind the file */
   if(ExtAudioFileSeek(extAudioFile, initialFrameOffset))
   {
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR, "CAF Rewind Error!");
      return false;
   }
   return true;
}

/*************************************************************************
 *                               _getBuffer                              *
 *************************************************************************/
bool CafStream::_getBuffer(unsigned long index, unsigned long readBytes, 
                unsigned long* bytesReaded, bool* gotEof)
{
   UInt32 maxFrames=0; /* Max frames could read this time */
   int err;
   
   AudioBufferList dataBuffer;
   dataBuffer.mNumberBuffers = 1;
   dataBuffer.mBuffers[0].mDataByteSize = readBytes;
   dataBuffer.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
   dataBuffer.mBuffers[0].mData = &bufferData[index];
   
   
   /* Calculate number of frames that fits the buffer */
   maxFrames = readBytes / outputFormat.mBytesPerFrame;
   *gotEof = false;
   *bytesReaded = 0;
   
   err = ExtAudioFileRead(extAudioFile, &maxFrames, &dataBuffer);
   if(err != noErr)
   {
      Kobold::Log::add(Kobold::Log::LOG_LEVEL_ERROR, "CAF buffer error: %s",
               errorString(err).c_str());
      return false;
   }
   
   /* Verify frames readed */
   if(maxFrames > 0)
   {
      /* Calculate number of bytes readed */
      *bytesReaded = maxFrames*outputFormat.mBytesPerFrame;
   }
   else if(maxFrames == 0)
   {
      /* Got EOF */
      *gotEof = true;
   }
   
   return true;
   
}

/*************************************************************************
 *                                errorString                            *
 *************************************************************************/
Kobold::String CafStream::errorString(int code)
{
   switch(code)
   {
      case kExtAudioFileError_InvalidProperty:
         return Kobold::String("Invalid Property.");
      case kExtAudioFileError_InvalidPropertySize:
         return Kobold::String("Invalid Property Size.");
      case kExtAudioFileError_NonPCMClientFormat:
         return Kobold::String("Non PCM Client Format.");
      case kExtAudioFileError_InvalidChannelMap:
         return Kobold::String("Invalid Channels Map.");
      case kExtAudioFileError_InvalidOperationOrder:
         return Kobold::String("Invalid Operation Order.");
      case kExtAudioFileError_InvalidDataFormat:
         return Kobold::String("Invalid Data Format.");
      case kExtAudioFileError_MaxPacketSizeUnknown:
         return Kobold::String("Max Packed Size Unknown.");
      case kExtAudioFileError_InvalidSeek:
         return Kobold::String("Invalid Seek.");
      default:
         return Kobold::String("Unknown CAF error.");
   }
}

#endif

