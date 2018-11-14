/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es> (Based on nr-helper.h)
 */

#ifndef NR_SIMPLE_HELPER_H
#define NR_SIMPLE_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/simple-channel.h"
#include "ns3/node-container.h"
#include "ns3/nr-radio-bearer-stats-calculator.h"

#include "ns3/nr-pdcp.h"
#include "ns3/nr-rlc.h"
#include "ns3/nr-rlc-um.h"
#include "ns3/nr-rlc-am.h"

namespace ns3 {

class NrTestRrc;
class NrTestMac;

/**
 * Creation and configuration of NR entities
 *
 */
class NrSimpleHelper : public Object
{
public:
  NrSimpleHelper (void);
  virtual ~NrSimpleHelper (void);

  static TypeId GetTypeId (void);
  virtual void DoDispose (void);


  /**
   * create a set of eNB devices
   *
   * \param c the node container where the devices are to be installed
   *
   * \return the NetDeviceContainer with the newly created devices
   */
  NetDeviceContainer InstallEnbDevice (NodeContainer c);

  /**
   * create a set of UE devices
   *
   * \param c the node container where the devices are to be installed
   *
   * \return the NetDeviceContainer with the newly created devices
   */
  NetDeviceContainer InstallUeDevice (NodeContainer c);


  /**
   * Enables logging for all components of the LENA architecture
   *
   */
  void EnableLogComponents (void);

  /**
   * Enables trace sinks for MAC, RLC and PDCP
   */
  void EnableTraces (void);


  /**
   * Enable trace sinks for RLC layer
   */
  void EnableRlcTraces (void);

  /**
   * Enable trace sinks for DL RLC layer
   */
  void EnableDlRlcTraces (void);

  /**
   * Enable trace sinks for UL RLC layer
   */
  void EnableUlRlcTraces (void);


  /**
   * Enable trace sinks for PDCP layer
   */
  void EnablePdcpTraces (void);

  /**
   * Enable trace sinks for DL PDCP layer
   */
  void EnableDlPdcpTraces (void);

  /**
   * Enable trace sinks for UL PDCP layer
   */
  void EnableUlPdcpTraces (void);


protected:
  // inherited from Object
  virtual void DoInitialize (void);

private:
  Ptr<NetDevice> InstallSingleEnbDevice (Ptr<Node> n);
  Ptr<NetDevice> InstallSingleUeDevice (Ptr<Node> n);

  Ptr<SimpleChannel> m_phyChannel;

public:

  Ptr<NrTestRrc> m_enbRrc;
  Ptr<NrTestRrc> m_ueRrc;

  Ptr<NrTestMac> m_enbMac;
  Ptr<NrTestMac> m_ueMac;

private:

  Ptr<NrPdcp>    m_enbPdcp;
  Ptr<NrRlc>     m_enbRlc;

  Ptr<NrPdcp>    m_uePdcp;
  Ptr<NrRlc>     m_ueRlc;

  ObjectFactory   m_enbDeviceFactory;
  ObjectFactory   m_ueDeviceFactory;

  enum NrRlcEntityType_t {RLC_UM = 1,
                           RLC_AM = 2 } m_nrRlcEntityType;

};


} // namespace ns3


#endif // NR_SIMPLE_HELPER_H