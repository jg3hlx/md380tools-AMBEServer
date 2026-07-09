//
//  cusb3000emuinterface.cpp
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

#include "main.h"
#include "ctimepoint.h"
#include "cambeserver.h"
#include "cambepacket.h"
#include "cusb3000emuinterface.h"
#include "cvocodecs.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CUsb3000emuInterface::CUsb3000emuInterface(uint32 uiVid, uint32 uiPid, const char *szDeviceName, const char *szDeviceSerial)
: CUsb3000Interface(uiVid, uiPid, szDeviceName, szDeviceSerial)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// low level

bool CUsb3000emuInterface::OpenDevice(void)
{
    bool ok;

    m_Ip = g_AmbeServer.GetEmuIp();
    m_uiPort = g_AmbeServer.GetEmuPort();
    
    // create our socket
    ok = m_Socket.Open(g_AmbeServer.GetListenIp(), 10400+m_uiPid);
    if ( !ok )
    {
        std::cout << "Error opening socket on port UDP" << 10400+m_uiPid << " on ip " << g_AmbeServer.GetListenIp() << std::endl;
        return false;
    }
    
    // done
    return true;
}

bool CUsb3000emuInterface::CheckIfDeviceNeedsReOpen(void)
{
    return false;
}

bool CUsb3000emuInterface::ReadBuffer(CBuffer *buffer)
{
    CBuffer     Buffer;
    CIp         Ip;
    int         len;
    
    len = m_Socket.Receive(&Buffer, &Ip, 5);
    if ( len > 0 )
    {
        if (len > USB3XXX_MAXPACKETSIZE)
        {
            std::cout << "ReadBuffer received packet is larger than expected" << std::endl;
            return false;
        }
        buffer->clear();
        buffer->resize(len);
        ::memcpy(buffer->data(), Buffer.data(), len);
        return true;
    }
    return false;
}

int CUsb3000emuInterface::FTDI_read_packet(FT_HANDLE ftHandle, char *pkt, int maxlen)
{
    CBuffer     Buffer;
    CIp         Ip;
    int         len;
    
    len = m_Socket.Receive(&Buffer, &Ip, 500);
    if ( len > 0 )
    {
        if (len > maxlen)
        {
            std::cout << "FTDI_read_packet supplied buffer is not large enough for packet" << std::endl;
            return 0;
        }
        ::memcpy(pkt, Buffer.data(), len);
        return len;
    }
    return 0;
}

bool CUsb3000emuInterface::FTDI_write_packet(FT_HANDLE ft_handle, const char *pkt, int len)
{
    CBuffer     Buffer;
    
    if ( len > 0 )
    {
        Buffer.Set((uint8 *)pkt, len);
        m_Socket.Send(Buffer, m_Ip, m_uiPort);
        return true;
    }
    return false;
}
