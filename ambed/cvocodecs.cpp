//
//  cvocodecs.cpp
//  ambed
//
//  Created by Jean-Luc Deltombe (LX3JL) on 23/04/2017.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
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
#include <string.h>
#include "cvocodecs.h"

////////////////////////////////////////////////////////////////////////////////////////
// global object

CVocodecs g_Vocodecs;

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CVocodecs::CVocodecs()
{
    m_Interfaces.reserve(5);
    m_Channels.reserve(20);
    m_FtdiDeviceDescrs.reserve(10);
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CVocodecs::~CVocodecs()
{
    // delete channels
    m_MutexChannels.lock();
    {
        for ( int i = 0; i < m_Channels.size(); i++ )
        {
            delete m_Channels[i];
        }
        m_Channels.clear();
    }
    m_MutexChannels.unlock();
    
    // delete interfaces
    m_MutexInterfaces.lock();
    {
        for ( int i = 0; i < m_Interfaces.size(); i++ )
        {
            delete m_Interfaces[i];
        }
        m_Interfaces.clear();
    }
    m_MutexInterfaces.unlock();
    
    // delete ftdi device descriptors
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        delete m_FtdiDeviceDescrs[i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CVocodecs::Init(void)
{
    bool ok = true;
    int iNbCh = 0;
    
    // discover and add vocodecs interfaces
    DiscoverFtdiDevices();

    // and create interfaces for the discovered devices
    // first handle all BaoFarm devices
    std::vector<CVocodecChannel *>  MultiBAOFarmDevicesChs;
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        CFtdiDeviceDescr *descr = m_FtdiDeviceDescrs[i];
        if ( !descr->IsUsed() && (descr->GetNbChannels() == 4) )
        {
            // create the object
            iNbCh += CFtdiDeviceDescr::CreateInterface(descr, &MultiBAOFarmDevicesChs);
            // and flag as used
            descr->SetUsed(true);
        }
    }
    // next handle all even number of channels devices
    std::vector<CVocodecChannel *>  Multi3003DevicesChs;
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        CFtdiDeviceDescr *descr = m_FtdiDeviceDescrs[i];
        if ( !descr->IsUsed() && IsEven(descr->GetNbChannels()) )
        {
            // create the object
            iNbCh += CFtdiDeviceDescr::CreateInterface(descr, &Multi3003DevicesChs);
            // and flag as used
            descr->SetUsed(true);
        }
    }
    // next handle all single channel devices.
    std::vector<CVocodecChannel *>  PairsOf3000DevicesChs;
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        CFtdiDeviceDescr *descr = m_FtdiDeviceDescrs[i];
        if ( !descr->IsUsed() && (descr->GetNbChannels() == 1) )
        {
            // create the object
            iNbCh += CFtdiDeviceDescr::CreateInterface(descr, &PairsOf3000DevicesChs);
            // and flag as used
            descr->SetUsed(true);
        }
    }
    // now we should have only remaining the 3 channels device(s)
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        CFtdiDeviceDescr *descr = m_FtdiDeviceDescrs[i];
        if ( !descr->IsUsed() && (descr->GetNbChannels() == 3) )
        {
            // create the object
            iNbCh += CFtdiDeviceDescr::CreateInterface(descr, &Multi3003DevicesChs);
            // and flag as used
            descr->SetUsed(true);
        }
    }


    // now agregate channels by order of priority
    // for proper load sharing
    // pairs of 3000 devices first
    {
        for ( int i = 0;  i < PairsOf3000DevicesChs.size(); i++ )
        {
            m_Channels.push_back(PairsOf3000DevicesChs.at(i));
        }
        PairsOf3000DevicesChs.clear();
    }
    // next BaoFarm devices
    {
        int n = (int)MultiBAOFarmDevicesChs.size() / 8;
        for ( int i = 0; (i < 8) && (n != 0); i++ )
        {
            for ( int j = 0; j < n; j++ )
            {
                m_Channels.push_back(MultiBAOFarmDevicesChs.at((j*8) + i));
            }
        }
        MultiBAOFarmDevicesChs.clear();
    }
    // finally interlace multi-3003 and pairs of 3003 devices which always
    // results to 6 channels per pair of 3003
    {
        int n = (int)Multi3003DevicesChs.size() / 6;
        for ( int i = 0; (i < 6) && (n != 0); i++ )
        {
            for ( int j = 0; j < n; j++ )
            {
                m_Channels.push_back(Multi3003DevicesChs.at((j*6) + i));
            }
        }
        Multi3003DevicesChs.clear();
    }

    
    // done
    if ( ok )
    {
        std::cout << "Codec interfaces initialized successfully : " << iNbCh << " channels available" << std::endl;
    }
    else
    {
        std::cout << "At least one codec interfaces failed to initialize : " << iNbCh << " channels availables" << std::endl;
    }
    // done
    return ok;
}

////////////////////////////////////////////////////////////////////////////////////////
// initialisation helpers

bool CVocodecs::DiscoverFtdiDevices(void)
{
    bool ok = false;
    int iNbDevices = 0;
    FT_DEVICE_LIST_INFO_NODE *list;
    
    // clear vector
    for ( int i = 0; i < m_FtdiDeviceDescrs.size(); i++ )
    {
        delete m_FtdiDeviceDescrs[i];
    }
    
    // and discover
    if ( FT_CreateDeviceInfoList((LPDWORD)&iNbDevices) == FT_OK )
    {
        std::cout << "Detected " << iNbDevices << " USB-FTDI devices" << std::endl << std::endl;
        ok = true;
        if ( iNbDevices > 0 )
        {
            // allocate the list
            list = new FT_DEVICE_LIST_INFO_NODE[iNbDevices];
            
            // fill
            if ( FT_GetDeviceInfoList(list, (LPDWORD)&iNbDevices) == FT_OK )
            {
                // process
                for ( int i = 0; i < iNbDevices; i++ )
                {
                    std::cout << "Description : " << list[i].Description << "\t Serial : " << list[i].SerialNumber << std::endl;
                    CFtdiDeviceDescr *descr = new CFtdiDeviceDescr(
                        LOWORD(list[i].ID), HIWORD(list[i].ID),
                        list[i].Description, list[i].SerialNumber);
                    m_FtdiDeviceDescrs.push_back(descr);
                }
                std::cout << std::endl;
            }
            else
            {
                ok = false;
            }

            // and delete
            delete list;
        }
    }
    
    // done
    return ok;
}

////////////////////////////////////////////////////////////////////////////////////////
// manage channels

CVocodecChannel *CVocodecs::OpenChannel(uint8 uiCodecIn, uint8 uiCodecOut)
{
    CVocodecChannel *Channel = NULL;
    bool done = false;
    
    // loop on all interface until suitable & available channel found
    m_MutexChannels.lock();
    for ( int i = 0; (i < m_Channels.size()) && !done; i++ )
    {
        if ( !m_Channels[i]->IsOpen() &&
             (m_Channels[i]->GetCodecIn() == uiCodecIn) &&
             (m_Channels[i]->GetCodecOut() == uiCodecOut) )
        {
            if ( m_Channels[i]->Open() )
            {
                Channel = m_Channels[i];
                done = true;
            }
        }
    }
    m_MutexChannels.unlock();
    
    // done
    return Channel;
}

void CVocodecs::CloseChannel(CVocodecChannel *Channel)
{
    Channel->Close();
}
