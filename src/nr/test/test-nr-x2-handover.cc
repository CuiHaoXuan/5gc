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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */


#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/nr-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/point-to-point-module.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NrX2HandoverTest");

struct HandoverEvent
{
  Time startTime;
  uint32_t ueDeviceIndex;
  uint32_t sourceEnbDeviceIndex;
  uint32_t targetEnbDeviceIndex;
};


class NrX2HandoverTestCase : public TestCase
{
public:

  /** 
   * 
   * 
   * \param nUes number of UEs in the test
   * \param nDedicatedBearers number of bearers to be activated per UE
   * \param handoverEventList 
   * \param handoverEventListName 
   * \param useUdp true if UDP is to be used, false if TCP is to be used
   * 
   * \return 
   */
  NrX2HandoverTestCase (uint32_t nUes, uint32_t nDedicatedBearers, std::list<HandoverEvent> handoverEventList, std::string handoverEventListName, bool useUdp, std::string schedulerType, bool admitHo, bool useIdealRrc);
  
private:
  static std::string BuildNameString (uint32_t nUes, uint32_t nDedicatedBearers, std::string handoverEventListName, bool useUdp, std::string schedulerType, bool admitHo, bool useIdealRrc);
  virtual void DoRun (void);
  void CheckConnected (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);

  uint32_t m_nUes; // number of UEs in the test
  uint32_t m_nDedicatedBearers; // number of UEs in the test
  std::list<HandoverEvent> m_handoverEventList;
  std::string m_handoverEventListName;
  bool m_ngc;
  bool m_useUdp;
  std::string m_schedulerType;
  bool m_admitHo;
  bool     m_useIdealRrc;
  Ptr<NrHelper> m_nrHelper;
  Ptr<PointToPointNgcHelper> m_ngcHelper;
  
  struct BearerData
  {
    uint32_t bid;
    Ptr<PacketSink> dlSink;
    Ptr<PacketSink> ulSink;
    uint32_t dlOldTotalRx;
    uint32_t ulOldTotalRx;
  };

  struct UeData
  {
    uint32_t id;
    std::list<BearerData> bearerDataList;
  };

  void SaveStatsAfterHandover (uint32_t ueIndex);
  void CheckStatsAWhileAfterHandover (uint32_t ueIndex);

  std::vector<UeData> m_ueDataVector;

  const Time m_maxHoDuration; 
  const Time m_statsDuration;
  const Time m_udpClientInterval;
  const uint32_t m_udpClientPktSize;

};


std::string NrX2HandoverTestCase::BuildNameString (uint32_t nUes, uint32_t nDedicatedBearers, std::string handoverEventListName, bool useUdp, std::string schedulerType, bool admitHo, bool useIdealRrc)
{
  std::ostringstream oss;
  oss << " nUes=" << nUes 
      << " nDedicatedBearers=" << nDedicatedBearers 
      << " udp=" << useUdp
      << " " << schedulerType
      << " admitHo=" << admitHo
      << " hoList: " << handoverEventListName;
  if (useIdealRrc)
    {
      oss << ", ideal RRC";
    }
  else
    {
      oss << ", real RRC";
    }  
  return oss.str ();
}

NrX2HandoverTestCase::NrX2HandoverTestCase (uint32_t nUes, uint32_t nDedicatedBearers, std::list<HandoverEvent> handoverEventList, std::string handoverEventListName, bool useUdp, std::string schedulerType, bool admitHo, bool useIdealRrc)
  : TestCase (BuildNameString (nUes, nDedicatedBearers, handoverEventListName, useUdp, schedulerType, admitHo, useIdealRrc)),
    m_nUes (nUes),
    m_nDedicatedBearers (nDedicatedBearers),
    m_handoverEventList (handoverEventList),
    m_handoverEventListName (handoverEventListName),
    m_ngc (true),
    m_useUdp (useUdp),
    m_schedulerType (schedulerType),
    m_admitHo (admitHo),
    m_useIdealRrc (useIdealRrc),
    m_maxHoDuration (Seconds (0.1)),
    m_statsDuration (Seconds (0.1)),
    m_udpClientInterval (Seconds (0.01)),
    m_udpClientPktSize (100)
    
{
}

void
NrX2HandoverTestCase::DoRun ()
{
  NS_LOG_FUNCTION (this << BuildNameString (m_nUes, m_nDedicatedBearers, m_handoverEventListName, m_useUdp, m_schedulerType, m_admitHo, m_useIdealRrc));

  Config::Reset ();
  Config::SetDefault ("ns3::UdpClient::Interval",  TimeValue (m_udpClientInterval));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));  
  Config::SetDefault ("ns3::UdpClient::PacketSize", UintegerValue (m_udpClientPktSize));  

  //Disable Uplink Power Control
  Config::SetDefault ("ns3::NrUePhy::EnableUplinkPowerControl", BooleanValue (false));

  int64_t stream = 1;
  
  m_nrHelper = CreateObject<NrHelper> ();
  m_nrHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));
  m_nrHelper->SetSchedulerType (m_schedulerType);
  m_nrHelper->SetHandoverAlgorithmType ("ns3::NrNoOpHandoverAlgorithm"); // disable automatic handover
  m_nrHelper->SetAttribute ("UseIdealRrc", BooleanValue (m_useIdealRrc));
  

  NodeContainer enbNodes;
  enbNodes.Create (2);
  NodeContainer ueNodes;
  ueNodes.Create (m_nUes);

  if (m_ngc)
    {
      m_ngcHelper = CreateObject<PointToPointNgcHelper> ();
      m_nrHelper->SetNgcHelper (m_ngcHelper);      
    }

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-3000, 0, 0)); // enb0
  positionAlloc->Add (Vector ( 3000, 0, 0)); // enb1
  for (uint16_t i = 0; i < m_nUes; i++)
    {
      positionAlloc->Add (Vector (0, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);  
  mobility.Install (ueNodes);

  NetDeviceContainer enbDevices;
  enbDevices = m_nrHelper->InstallEnbDevice (enbNodes);
  stream += m_nrHelper->AssignStreams (enbDevices, stream);
  for (NetDeviceContainer::Iterator it = enbDevices.Begin ();
       it != enbDevices.End ();
       ++it)
    {
      Ptr<NrEnbRrc> enbRrc = (*it)->GetObject<NrEnbNetDevice> ()->GetRrc ();
      enbRrc->SetAttribute ("AdmitHandoverRequest", BooleanValue (m_admitHo));
    }

  NetDeviceContainer ueDevices;
  ueDevices = m_nrHelper->InstallUeDevice (ueNodes);
  stream += m_nrHelper->AssignStreams (ueDevices, stream);

  Ipv4Address remoteHostAddr;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ipv4InterfaceContainer ueIpIfaces;
  Ptr<Node> remoteHost;
  if (m_ngc)
    {
      // Create a single RemoteHost
      NodeContainer remoteHostContainer;
      remoteHostContainer.Create (1);
      remoteHost = remoteHostContainer.Get (0);
      InternetStackHelper internet;
      internet.Install (remoteHostContainer);

      // Create the Internet
      PointToPointHelper p2ph;
      p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
      p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
      p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
      Ptr<Node> upf = m_ngcHelper->GetUpfNode ();
      NetDeviceContainer internetDevices = p2ph.Install (upf, remoteHost);
      Ipv4AddressHelper ipv4h;
      ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
      Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
      // in this container, interface 0 is the upf, 1 is the remoteHost
      remoteHostAddr = internetIpIfaces.GetAddress (1);

      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
      remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

      // Install the IP stack on the UEs      
      internet.Install (ueNodes);
      ueIpIfaces = m_ngcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevices));
    }

  // attachment (needs to be done after IP stack configuration)
  // all UEs attached to eNB 0 at the beginning
  m_nrHelper->Attach (ueDevices, enbDevices.Get (0));
   
  if (m_ngc)
    {
      // always true: bool ngcDl = true;
      // always true: bool ngcUl = true;
      // the rest of this block is copied from lena-dual-stripe

    
      // Install and start applications on UEs and remote host
      uint16_t dlPort = 10000;
      uint16_t ulPort = 20000;

      // randomize a bit start times to avoid simulation artifacts
      // (e.g., buffer overflows due to packet transmissions happening
      // exactly at the same time) 
      Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
      startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
      startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));
      startTimeSeconds->SetStream (stream++);      

      for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
          Ptr<Node> ue = ueNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (m_ngcHelper->GetUeDefaultGatewayAddress (), 1);

          UeData ueData;
     
          for (uint32_t b = 0; b < m_nDedicatedBearers; ++b)
            {
              ++dlPort;
              ++ulPort;

              ApplicationContainer clientApps;
              ApplicationContainer serverApps;
              BearerData bearerData;

              if (m_useUdp)
                {              
                  // always true: if (ngcDl)
                    {
                      UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
                      clientApps.Add (dlClientHelper.Install (remoteHost));
                      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                      ApplicationContainer sinkContainer = dlPacketSinkHelper.Install (ue);
                      bearerData.dlSink = sinkContainer.Get (0)->GetObject<PacketSink> ();
                      serverApps.Add (sinkContainer);
                      
                    }
                  // always true: if (ngcUl)
                    {      
                      UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
                      clientApps.Add (ulClientHelper.Install (ue));
                      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                      ApplicationContainer sinkContainer = ulPacketSinkHelper.Install (remoteHost);
                      bearerData.ulSink = sinkContainer.Get (0)->GetObject<PacketSink> ();
                      serverApps.Add (sinkContainer);  
                    }            
                }                    
              else // use TCP
                {
                  // always true: if (ngcDl)
                    {
                      BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
                      dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));
                      clientApps.Add (dlClientHelper.Install (remoteHost));
                      PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                      ApplicationContainer sinkContainer = dlPacketSinkHelper.Install (ue);
                      bearerData.dlSink = sinkContainer.Get (0)->GetObject<PacketSink> ();
                      serverApps.Add (sinkContainer);
                    }
                  // always true: if (ngcUl)
                    {     
                      BulkSendHelper ulClientHelper ("ns3::TcpSocketFactory",
                                                     InetSocketAddress (remoteHostAddr, ulPort));
                      ulClientHelper.SetAttribute ("MaxBytes", UintegerValue (0));                  
                      clientApps.Add (ulClientHelper.Install (ue));
                      PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory", 
                                                           InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                      ApplicationContainer sinkContainer = ulPacketSinkHelper.Install (remoteHost);
                      bearerData.ulSink = sinkContainer.Get (0)->GetObject<PacketSink> ();
                      serverApps.Add (sinkContainer);  
                    }
                } // end if (useUdp)

              Ptr<NgcTft> tft = Create<NgcTft> ();
              // always true: if (ngcDl)
                {
                  NgcTft::PacketFilter dlpf;
                  dlpf.localPortStart = dlPort;
                  dlpf.localPortEnd = dlPort;
                  tft->Add (dlpf); 
                }
              // always true: if (ngcUl)
                {
                  NgcTft::PacketFilter ulpf;
                  ulpf.remotePortStart = ulPort;
                  ulpf.remotePortEnd = ulPort;
                  tft->Add (ulpf);
                }

              // always true: if (ngcDl || ngcUl)
                {
                  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
                  m_nrHelper->ActivateDedicatedEpsBearer (ueDevices.Get (u), bearer, tft);
                }
              Time startTime = Seconds (startTimeSeconds->GetValue ());
              serverApps.Start (startTime);
              clientApps.Start (startTime);

              ueData.bearerDataList.push_back (bearerData);

            } // end for b

          m_ueDataVector.push_back (ueData);
        }

    } 
  else // (ngc == false)
    {
      // for radio bearer activation purposes, consider together home UEs and macro UEs
      for (uint32_t u = 0; u < ueDevices.GetN (); ++u)
        {
          Ptr<NetDevice> ueDev = ueDevices.Get (u);
          for (uint32_t b = 0; b < m_nDedicatedBearers; ++b)
            {
              enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
              EpsBearer bearer (q);
              m_nrHelper->ActivateDataRadioBearer (ueDev, bearer);
            }
        }
    }


  m_nrHelper->AddX2Interface (enbNodes);

  // check initial RRC connection
  const Time maxRrcConnectionEstablishmentDuration = Seconds (0.080);
  for (NetDeviceContainer::Iterator it = ueDevices.Begin (); it != ueDevices.End (); ++it)
    {
      Simulator::Schedule (maxRrcConnectionEstablishmentDuration, 
                           &NrX2HandoverTestCase::CheckConnected, 
                           this, *it, enbDevices.Get (0));
    }
  
  // schedule handover events and corresponding checks

  Time stopTime = Seconds (0);  
  for (std::list<HandoverEvent>::iterator hoEventIt = m_handoverEventList.begin ();
       hoEventIt != m_handoverEventList.end ();
       ++hoEventIt)
    {
      Simulator::Schedule (hoEventIt->startTime, 
                           &NrX2HandoverTestCase::CheckConnected, 
                           this, 
                           ueDevices.Get (hoEventIt->ueDeviceIndex), 
                           enbDevices.Get (hoEventIt->sourceEnbDeviceIndex));
      m_nrHelper->HandoverRequest (hoEventIt->startTime, 
                                    ueDevices.Get (hoEventIt->ueDeviceIndex),
                                    enbDevices.Get (hoEventIt->sourceEnbDeviceIndex),
                                    enbDevices.Get (hoEventIt->targetEnbDeviceIndex));
      Time hoEndTime = hoEventIt->startTime + m_maxHoDuration;
      Simulator::Schedule (hoEndTime, 
                           &NrX2HandoverTestCase::CheckConnected, 
                           this, 
                           ueDevices.Get (hoEventIt->ueDeviceIndex), 
                           enbDevices.Get (m_admitHo ? hoEventIt->targetEnbDeviceIndex : hoEventIt->sourceEnbDeviceIndex));
      Simulator::Schedule (hoEndTime, &NrX2HandoverTestCase::SaveStatsAfterHandover,
                           this, hoEventIt->ueDeviceIndex);

      Time checkStatsAfterHoTime = hoEndTime + m_statsDuration;
      Simulator::Schedule (checkStatsAfterHoTime, &NrX2HandoverTestCase::CheckStatsAWhileAfterHandover, 
                           this, hoEventIt->ueDeviceIndex);      
      if (stopTime <= checkStatsAfterHoTime)
        {
          stopTime = checkStatsAfterHoTime + MilliSeconds (1);
        }
    }
  
  // m_nrHelper->EnableRlcTraces ();
  // m_nrHelper->EnablePdcpTraces();

 
  Simulator::Stop (stopTime);

  Simulator::Run ();

  Simulator::Destroy ();

}

void 
NrX2HandoverTestCase::CheckConnected (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice)
{
  Ptr<NrUeNetDevice> ueNrDevice = ueDevice->GetObject<NrUeNetDevice> ();
  Ptr<NrUeRrc> ueRrc = ueNrDevice->GetRrc ();
  NS_TEST_ASSERT_MSG_EQ (ueRrc->GetState (), NrUeRrc::CONNECTED_NORMALLY, "Wrong NrUeRrc state!");


  Ptr<NrEnbNetDevice> enbNrDevice = enbDevice->GetObject<NrEnbNetDevice> ();
  Ptr<NrEnbRrc> enbRrc = enbNrDevice->GetRrc ();
  uint16_t rnti = ueRrc->GetRnti ();
  Ptr<UeManager> ueManager = enbRrc->GetUeManager (rnti);  
  NS_TEST_ASSERT_MSG_NE (ueManager, 0, "RNTI " << rnti << " not found in eNB");

  UeManager::State ueManagerState = ueManager->GetState ();
  NS_TEST_ASSERT_MSG_EQ (ueManagerState, UeManager::CONNECTED_NORMALLY, "Wrong UeManager state!");
  NS_ASSERT_MSG (ueManagerState == UeManager::CONNECTED_NORMALLY, "Wrong UeManager state!");

  uint16_t ueCellId = ueRrc->GetCellId ();
  uint16_t enbCellId = enbNrDevice->GetCellId ();
  uint8_t ueDlBandwidth = ueRrc->GetDlBandwidth ();
  uint8_t enbDlBandwidth = enbNrDevice->GetDlBandwidth ();
  uint8_t ueUlBandwidth = ueRrc->GetUlBandwidth ();
  uint8_t enbUlBandwidth = enbNrDevice->GetUlBandwidth ();
  uint8_t ueDlEarfcn = ueRrc->GetDlEarfcn ();
  uint8_t enbDlEarfcn = enbNrDevice->GetDlEarfcn ();
  uint8_t ueUlEarfcn = ueRrc->GetUlEarfcn ();
  uint8_t enbUlEarfcn = enbNrDevice->GetUlEarfcn ();
  uint64_t ueImsi = ueNrDevice->GetImsi ();
  uint64_t enbImsi = ueManager->GetImsi ();

  NS_TEST_ASSERT_MSG_EQ (ueImsi, enbImsi, "inconsistent IMSI");
  NS_TEST_ASSERT_MSG_EQ (ueCellId, enbCellId, "inconsistent CellId");
  NS_TEST_ASSERT_MSG_EQ (ueDlBandwidth, enbDlBandwidth, "inconsistent DlBandwidth");
  NS_TEST_ASSERT_MSG_EQ (ueUlBandwidth, enbUlBandwidth, "inconsistent UlBandwidth");
  NS_TEST_ASSERT_MSG_EQ (ueDlEarfcn, enbDlEarfcn, "inconsistent DlEarfcn");
  NS_TEST_ASSERT_MSG_EQ (ueUlEarfcn, enbUlEarfcn, "inconsistent UlEarfcn");

  ObjectMapValue enbDataRadioBearerMapValue;
  ueManager->GetAttribute ("DataRadioBearerMap", enbDataRadioBearerMapValue);
  NS_TEST_ASSERT_MSG_EQ (enbDataRadioBearerMapValue.GetN (), m_nDedicatedBearers + 1, "wrong num bearers at eNB");  

  ObjectMapValue ueDataRadioBearerMapValue;
  ueRrc->GetAttribute ("DataRadioBearerMap", ueDataRadioBearerMapValue);
  NS_TEST_ASSERT_MSG_EQ (ueDataRadioBearerMapValue.GetN (), m_nDedicatedBearers + 1, "wrong num bearers at UE"); 

  ObjectMapValue::Iterator enbBearerIt = enbDataRadioBearerMapValue.Begin ();
  ObjectMapValue::Iterator ueBearerIt = ueDataRadioBearerMapValue.Begin ();
  while (enbBearerIt != enbDataRadioBearerMapValue.End () &&
         ueBearerIt != ueDataRadioBearerMapValue.End ())
    {
      Ptr<NrDataRadioBearerInfo> enbDrbInfo = enbBearerIt->second->GetObject<NrDataRadioBearerInfo> ();
      Ptr<NrDataRadioBearerInfo> ueDrbInfo = ueBearerIt->second->GetObject<NrDataRadioBearerInfo> ();
      //NS_TEST_ASSERT_MSG_EQ (enbDrbInfo->m_epsBearer, ueDrbInfo->m_epsBearer, "epsBearer differs");
      NS_TEST_ASSERT_MSG_EQ ((uint32_t) enbDrbInfo->m_epsBearerIdentity, (uint32_t) ueDrbInfo->m_epsBearerIdentity, "epsBearerIdentity differs");
      NS_TEST_ASSERT_MSG_EQ ((uint32_t) enbDrbInfo->m_drbIdentity, (uint32_t) ueDrbInfo->m_drbIdentity, "drbIdentity differs");
      //NS_TEST_ASSERT_MSG_EQ (enbDrbInfo->m_rlcConfig, ueDrbInfo->m_rlcConfig, "rlcConfig differs");
      NS_TEST_ASSERT_MSG_EQ ((uint32_t) enbDrbInfo->m_logicalChannelIdentity, (uint32_t) ueDrbInfo->m_logicalChannelIdentity, "logicalChannelIdentity differs");
      //NS_TEST_ASSERT_MSG_EQ (enbDrbInfo->m_logicalChannelConfig, ueDrbInfo->m_logicalChannelConfig, "logicalChannelConfig differs");
 
      ++enbBearerIt;
      ++ueBearerIt;
    }
  NS_ASSERT_MSG (enbBearerIt == enbDataRadioBearerMapValue.End (), "too many bearers at eNB");
  NS_ASSERT_MSG (ueBearerIt == ueDataRadioBearerMapValue.End (), "too many bearers at UE");  
}

void 
NrX2HandoverTestCase::SaveStatsAfterHandover (uint32_t ueIndex)
{
  for (std::list<BearerData>::iterator it = m_ueDataVector.at (ueIndex).bearerDataList.begin ();
       it != m_ueDataVector.at (ueIndex).bearerDataList.end ();
       ++it)
    {
      it->dlOldTotalRx = it->dlSink->GetTotalRx ();
      it->ulOldTotalRx = it->ulSink->GetTotalRx ();
    }
}

void 
NrX2HandoverTestCase::CheckStatsAWhileAfterHandover (uint32_t ueIndex)
{      
  uint32_t b = 1;
  for (std::list<BearerData>::iterator it = m_ueDataVector.at (ueIndex).bearerDataList.begin ();
       it != m_ueDataVector.at (ueIndex).bearerDataList.end ();
       ++it)
    {
      uint32_t dlRx = it->dlSink->GetTotalRx () - it->dlOldTotalRx;
      uint32_t ulRx = it->ulSink->GetTotalRx () - it->ulOldTotalRx;                       
      uint32_t expectedBytes = m_udpClientPktSize * (m_statsDuration.GetSeconds () / m_udpClientInterval.GetSeconds ());
      //                           tolerance
      NS_TEST_ASSERT_MSG_GT (dlRx,   0.500 * expectedBytes, "too few RX bytes in DL, ue=" << ueIndex << ", b=" << b);
      NS_TEST_ASSERT_MSG_GT (ulRx,   0.500 * expectedBytes, "too few RX bytes in UL, ue=" << ueIndex << ", b=" << b);
      ++b;
    }
}


class NrX2HandoverTestSuite : public TestSuite
{
public:
  NrX2HandoverTestSuite ();
};


NrX2HandoverTestSuite::NrX2HandoverTestSuite ()
  : TestSuite ("nr-x2-handover", SYSTEM)
{
  // in the following:
  // fwd means handover from enb 0 to enb 1
  // bwd means handover from enb 1 to enb 0

  HandoverEvent ue1fwd;
  ue1fwd.startTime = MilliSeconds (100); 
  ue1fwd.ueDeviceIndex = 0;
  ue1fwd.sourceEnbDeviceIndex = 0;
  ue1fwd.targetEnbDeviceIndex = 1;

  HandoverEvent ue1bwd;
  ue1bwd.startTime = MilliSeconds (300); 
  ue1bwd.ueDeviceIndex = 0;
  ue1bwd.sourceEnbDeviceIndex = 1;
  ue1bwd.targetEnbDeviceIndex = 0;

  HandoverEvent ue1fwdagain;
  ue1fwdagain.startTime = MilliSeconds (500); 
  ue1fwdagain.ueDeviceIndex = 0;
  ue1fwdagain.sourceEnbDeviceIndex = 0;
  ue1fwdagain.targetEnbDeviceIndex = 1;

  HandoverEvent ue2fwd;
  ue2fwd.startTime = MilliSeconds (110); 
  ue2fwd.ueDeviceIndex = 1;
  ue2fwd.sourceEnbDeviceIndex = 0;
  ue2fwd.targetEnbDeviceIndex = 1;

  HandoverEvent ue2bwd;
  ue2bwd.startTime = MilliSeconds (250); 
  ue2bwd.ueDeviceIndex = 1;
  ue2bwd.sourceEnbDeviceIndex = 1;
  ue2bwd.targetEnbDeviceIndex = 0;

  std::string hel0name ("none");
  std::list<HandoverEvent> hel0;

  std::string hel1name ("1 fwd");
  std::list<HandoverEvent> hel1;
  hel1.push_back (ue1fwd);

  std::string hel2name ("1 fwd & bwd");
  std::list<HandoverEvent> hel2;
  hel2.push_back (ue1fwd);
  hel2.push_back (ue1bwd);

  std::string hel3name ("1 fwd & bwd & fwd");
  std::list<HandoverEvent> hel3;
  hel3.push_back (ue1fwd);
  hel3.push_back (ue1bwd);
  hel3.push_back (ue1fwdagain);

  std::string hel4name ("1+2 fwd");
  std::list<HandoverEvent> hel4;
  hel4.push_back (ue1fwd);
  hel4.push_back (ue2fwd);

  std::string hel5name ("1+2 fwd & bwd");
  std::list<HandoverEvent> hel5;
  hel5.push_back (ue1fwd);
  hel5.push_back (ue1bwd);
  hel5.push_back (ue2fwd);
  hel5.push_back (ue2bwd);

  std::string hel6name ("2 fwd");
  std::list<HandoverEvent> hel6;
  hel6.push_back (ue2fwd);

  std::string hel7name ("2 fwd & bwd");
  std::list<HandoverEvent> hel7;
  hel7.push_back (ue2fwd);
  hel7.push_back (ue2bwd);

  std::vector<std::string> schedulers;
  schedulers.push_back ("ns3::NrRrFfMacScheduler");
  schedulers.push_back ("ns3::NrPfFfMacScheduler");
  for (std::vector<std::string>::iterator schedIt = schedulers.begin (); schedIt != schedulers.end (); ++schedIt)
    {
      for (int32_t useIdealRrc = 1; useIdealRrc >= 0; --useIdealRrc)
        {
          //                                     nUes, nDBearers, helist, name, useUdp, sched, admitHo, idealRrc
          AddTestCase (new NrX2HandoverTestCase (  1,    0,    hel0, hel0name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel0, hel0name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    5,    hel0, hel0name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    5,    hel0, hel0name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    0,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    1,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    2,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    0,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    1,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    2,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    1,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    2,    hel1, hel1name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    1,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    2,    hel1, hel1name, true, *schedIt, false, useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    0,    hel2, hel2name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    1,    hel2, hel2name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    2,    hel2, hel2name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    0,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    1,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  1,    2,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    1,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    2,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::QUICK);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    1,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    2,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    0,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    1,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  2,    2,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    0,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    1,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    2,    hel3, hel3name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    0,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    1,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    2,    hel4, hel4name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    0,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    1,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::EXTENSIVE);
          AddTestCase (new NrX2HandoverTestCase (  3,    2,    hel5, hel5name, true, *schedIt, true,  useIdealRrc), TestCase::QUICK);

        }
    }
}

static NrX2HandoverTestSuite g_nrX2HandoverTestSuiteInstance;
