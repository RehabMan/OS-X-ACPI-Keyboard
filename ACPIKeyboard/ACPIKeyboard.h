/*
 * Copyright (c) 1998-2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef _ACPIKEYBOARD_H
#define _ACPIKEYBOARD_H

#define ACPIKeyboard org_rehabman_ACPIKeyboard

#include <libkern/c++/OSBoolean.h>
#include <IOKit/hidsystem/IOHIKeyboard.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#ifdef DEBUG
#define DEBUG_LOG(args...)  do { IOLog(args); } while (0)
#else
#define DEBUG_LOG(args...)  do { } while (0)
#endif

#define EXPORT __attribute__((visibility("default")))

#define kPacketLength (2+6+8) // 2 bytes for key data, 6-bytes not used, 8 bytes for timestamp
#define kPacketKeyOffset 0
#define kPacketTimeOffset 8
#define kPacketKeyDataLength 2

class EXPORT ACPIKeyboard : public IOHIKeyboard
{
    typedef IOHIKeyboard super;
    OSDeclareDefaultStructors(org_rehabman_ACPIKeyboard);
    
private:
    bool dispatchKeyboardEventWithPacket(const UInt8* packet);
    IOHIKeyboard *findKeyboardDevice();
    IOHIKeyboard* _delegateKeyboard;

protected:
    virtual const unsigned char * defaultKeymapOfLength(UInt32 * length);
    virtual UInt32 maxKeyCodes();
    inline void dispatchKeyboardEventX(unsigned int keyCode, bool goingDown, uint64_t time)
        { dispatchKeyboardEvent(keyCode, goingDown, *(AbsoluteTime*)&time); }
  

public:
    virtual bool init(OSDictionary * dict);
    virtual void free();
    virtual ACPIKeyboard * probe(IOService * provider, SInt32 * score);

    virtual bool start(IOService * provider);
    virtual void stop(IOService * provider);
    
    virtual UInt32 deviceType();
    virtual UInt32 interfaceID();
    
    virtual IOReturn message(UInt32 type, IOService* provider, void* argument);
};

#endif /* _ACPIKEYBOARD_H */
