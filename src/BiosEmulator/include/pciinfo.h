#pragma once

/****************************************************************************
 *
 *                   SciTech OS Portability Manager Library
 *
 *  ========================================================================
 *
 *    The contents of this file are subject to the SciTech MGL Public
 *    License Version 1.0 (the "License"); you may not use this file
 *    except in compliance with the License. You may obtain a copy of
 *    the License at http://www.scitechsoft.com/mgl-license.txt
 *
 *    Software distributed under the License is distributed on an
 *    "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *    implied. See the License for the specific language governing
 *    rights and limitations under the License.
 *
 *    The Original Code is Copyright (C) 1991-1998 SciTech Software, Inc.
 *
 *    The Initial Developer of the Original Code is SciTech Software, Inc.
 *    All Rights Reserved.
 *
 *  ========================================================================
 *
 * Language:     ANSI C
 * Environment:  Any
 *
 * Description:  Header file for interface routines to the PCI bus.
 *
 ****************************************************************************/

#pragma pack(1)

#include "x86emu/types.h"

/* NOTE: We define all bitfield's as uint's, specifically so that the IBM
 *       Visual Age C++ compiler does not complain. We need them to be
 *       32-bits wide, and this is the width of an unsigned integer, but
 *       we can't use a ulong to make this explicit or we get errors.
 */

/* Structure defining a PCI slot identifier */

typedef union
{
    struct
    {
        uint Zero : 2;
        uint Register : 6;
        uint Function : 3;
        uint Device : 5;
        uint Bus : 8;
        uint Reserved : 7;
        uint Enable : 1;
    } p;
    uint32_t i;
} PCIslot;

/* Structure defining the regular (type 0) PCI configuration register
 * layout. We use this in a union below so we can describe all types of
 * PCI configuration spaces with a single structure.
 */

typedef struct
{
    ulong BaseAddress10;
    ulong BaseAddress14;
    ulong BaseAddress18;
    ulong BaseAddress1C;
    ulong BaseAddress20;
    ulong BaseAddress24;
    ulong CardbusCISPointer;
    ushort SubSystemVendorID;
    ushort SubSystemID;
    ulong ROMBaseAddress;
    uchar CapabilitiesPointer;
    uchar reserved1;
    uchar reserved2;
    uchar reserved3;
    ulong reserved4;
    uchar InterruptLine;
    uchar InterruptPin;
    uchar MinimumGrant;
    uchar MaximumLatency;

    /* These are not in the actual config space, but we enumerate them */
    ulong BaseAddress10Len;
    ulong BaseAddress14Len;
    ulong BaseAddress18Len;
    ulong BaseAddress1CLen;
    ulong BaseAddress20Len;
    ulong BaseAddress24Len;
    ulong ROMBaseAddressLen;
} PCIType0Info;

/* Structure defining PCI to PCI bridge (type 1) PCI configuration register
 * layout. We use this in a union below so we can describe all types of
 * PCI configuration spaces with a single structure.
 */

typedef struct
{
    ulong BaseAddress10;
    ulong BaseAddress14;
    uchar PrimaryBusNumber;
    uchar SecondayBusNumber;
    uchar SubordinateBusNumber;
    uchar SecondaryLatencyTimer;
    uchar IOBase;
    uchar IOLimit;
    ushort SecondaryStatus;
    ushort MemoryBase;
    ushort MemoryLimit;
    ushort PrefetchableMemoryBase;
    ushort PrefetchableMemoryLimit;
    ulong PrefetchableBaseHi;
    ulong PrefetchableLimitHi;
    ushort IOBaseHi;
    ushort IOLimitHi;
    uchar CapabilitiesPointer;
    uchar reserved1;
    uchar reserved2;
    uchar reserved3;
    ulong ROMBaseAddress;
    uchar InterruptLine;
    uchar InterruptPin;
    ushort BridgeControl;
} PCIType1Info;

/* PCI to CardBus bridge (type 2) configuration information */
typedef struct
{
    ulong SocketRegistersBaseAddress;
    uchar CapabilitiesPointer;
    uchar reserved1;
    ushort SecondaryStatus;
    uchar PrimaryBus;
    uchar SecondaryBus;
    uchar SubordinateBus;
    uchar SecondaryLatency;
    struct
    {
        ulong Base;
        ulong Limit;
    } Range[4];
    uchar InterruptLine;
    uchar InterruptPin;
    ushort BridgeControl;
} PCIType2Info;

/* Structure defining the PCI configuration space information for a
 * single PCI device on the PCI bus. We enumerate all this information
 * for all PCI devices on the bus.
 */

typedef struct
{
    ulong dwSize;
    PCIslot slot;
    ulong mech1;
    ushort VendorID;
    ushort DeviceID;
    ushort Command;
    ushort Status;
    uchar RevID;
    uchar Interface;
    uchar SubClass;
    uchar BaseClass;
    uchar CacheLineSize;
    uchar LatencyTimer;
    uchar HeaderType;
    uchar BIST;
    union
    {
        PCIType0Info type0;
        PCIType1Info type1;
        PCIType2Info type2;
    } u;
} PCIDeviceInfo;

#pragma pack()