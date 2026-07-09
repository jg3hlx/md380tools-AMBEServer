//
//  cusb3000emuinterface.h
//  ambed
//
// ----------------------------------------------------------------------------
//    This file is part of ambed.
//
//    xlxd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    xlxd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef cusb3000emuinterface_h
#define cusb3000emuinterface_h


#include "cudpsocket.h"
#include "cbuffer.h"
#include "cusb3000interface.h"

////////////////////////////////////////////////////////////////////////////////////////
// define


////////////////////////////////////////////////////////////////////////////////////////
// class

class CUsb3000emuInterface : public CUsb3000Interface
{
public:
    // constructors
    CUsb3000emuInterface(uint32, uint32, const char *, const char *);
    
    // destructor
    virtual ~CUsb3000emuInterface() {}
    
protected:
    // low level
    bool OpenDevice(void);
    bool CheckIfDeviceNeedsReOpen(void);
    bool ReadBuffer(CBuffer *);
    int  FTDI_read_packet(FT_HANDLE, char *, int);
    bool FTDI_write_packet(FT_HANDLE, const char *, int);

protected:
    CUdpSocket      m_Socket;
    CIp             m_Ip;
    uint16          m_uiPort;
};

////////////////////////////////////////////////////////////////////////////////////////
#endif /* cusb3000emuinterface_h */
