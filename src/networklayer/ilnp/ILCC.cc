
#include <algorithm>
#include <vector>

#include "InterfaceTableAccess.h"
#include "IPv6ControlInfo.h"
#include "IPv6InterfaceData.h"
#include "IPv6NeighbourDiscoveryAccess.h"
#include "RoutingTable6Access.h"
#include "ICMPv6Access.h"

#define MK_SEND_PERIODIC_ILNP_LU            1

Define_Module(ILCC);

void ILCC::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == 0)
    {
        EV << "Initializing ILNP module" << endl;
    }
    else if (stage == 2)
    {
    }
    else if (stage == 3)
    {
      icmpv6 = ICMPv6Access().get();
    }
}

void ILCC::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "Self message received!\n";

        if (msg->getKind() == MK_SEND_PERIODIC_ILNP_LU)
        {
            EV << "Periodic BU Timeout Message Received\n";
            sendPeriodicLU(msg);
        }
        else
            error("Unrecognized Timer"); 
    else
        opp_error("Unknown message type received.");
}

 // called after the interface is configured with a new ipv6 address.
void ILCC::triggerHandoff(InterfaceEntry *ie, const IPv6Address& newLocator)
{
	// once the handoff is triggered, the MN should also update its local Locator values in the
	// ILCC. Now uodating the local locator values can be performed here or somewhere else??

    Enter_Method_Silent();

    ILCache::iterator it;

    // stores the currently active locator value for quick reference.
    currActiveLocalLocator = newLocator;

    L64Entry locatorEntry = new L64Entry();
    locatorEntry.locatorAddress = newLocator;
    locatorEntry.flag = ACTIVE;
    locatorEntry.ie = ie;

    // make the Locator Corresponding to the currently Active interface as active.
    localLocators[currActiveInterface].flag = VALID;
    
    currActiveInterface = ie;
    localLocators[currActiveInterface] = locatorEntry;

    // creating Location Update Message Timers For sending LU Messages to each of the
    // Correspondent Nodes.
    for(it = ilcc.begin(); it!=ilcc.end(); it++)
    {
    	ILCacheEntry ilccEntry = it->second;
      IPv6Address remoteLocator = ilccEntry.currActiveRemoteLocator;
      IPv6Address remoteIdentifier = ilccEntry.remoteIdentifier;
      // forming the ILNP Address of the correspondent Node.
      IPv6Address cNILNPAddress = remoteIdentifier.setPrefix(remoteLocator,64);
      // "@Kbtodo.." _getMaxCNBindingLifetime() to be defined in the interfaces IPv6 data.
      // In the InterfaceData.. the getMaxCnLuAckTimeout is set to 0. Change it accordingly.
      createLUTimer(remoteIdentifier,cNILNPAddress,ie,ie->ipv6Data()->_getMaxCNBindingLifetime())
    }
}

void ILCC::createLUTimer(const IPv6Address& cNIdentifier,
	                const IPv6Address& cNILNPAddress, InterfaceEntry* ie, const uint lifeTime)
{
    Enter_Method("createBUTimer()");

    EV << "Creating BU timer at sim time: " << simTime() << " seconds." << endl;

    cMessage *buTriggerMsg = new cMessage("sendPeriodicLU", MK_SEND_PERIODIC_ILNP_LU);

    // to write this method..
    // get the timer data structure for the CN from the ILCC.
    BindingUpdateTimer* buTimer = getBindingUpdateTimer(cNIdentifier);

    buTimer->destILNPAddress = cNILNPAddress;
    buTimer->ifEntry = ie;
    buTimer->timer = buTriggerMsg;

    // TO WRITE THIS METHOD.
    // get the  sequenceNumber corresponding to the CN using the CN's Identifier.
    //buTimer->buSequenceNumber = getSequenceNumber(cNIdentifier);
    // Increment the last Sent luSeq by 1.
    // and also increment the sentSequence number in the cache by 1.
    buTimer->buSequenceNumber = ilcc[cNIdentifier].sentSequenceNumber++;
    buTimer->lifeTime = lifeTime;
    //This "getInitialBindAckTimeout" was defined already for mipv6. Define seperately for ILNP
    buTimer->ackTimeout = ie->ipv6Data()->_getInitialLUAckTimeout();
    buTriggerMessage->setContextPointer(butimer);
    scheduleAt(simTime(), buTriggerMsg);
}


void ILCache::sendPeriodicLU(cMessage *msg)
{
	
	  EV << "Sending periodic BU message at time: " << simTime() << " seconds." << endl;

    BindingUpdateTimer *buTimer = (BindingUpdateTimer* )msg->getContextPointer();

    InterfaceEntry* ie = buTimer->ifEntry; 
    IPv6Address& cNILNPAddress = buTimer->destILNPAddress;
    IPv6Address& destIdentifier = buTimer->destIdentifier;
    buTimer->presentSentTimeBU = simTime();

    buTimer->nextScheduledTime = buTimer->presentSentTimeBU + buTimer->ackTimeout;
    buTimer->ackTimeout = (buTimer->ackTimeout)* 2;
    buTimer->buSequenceNumber = (buTimer->buSequenceNumber+1) % 65536;

    // update the sent sequence number in ILCC.
    ilcc[destIdentifier].sentSequenceNumber = buTimer->buSequenceNumber;
    createAndSendLUMessage(cNILNPAddress, ie, buTimer->buSequenceNumber, buTimer->lifeTime);

    // "getMaxBindAckTimeout" defined already  for mipv6.. must be redefined for ILNP. I think
    if (!(buTimer->ackTimeout < ie->ipv6Data()->_getMaxLUAckTimeout()))
    {
        EV << "Crossed maximum BINDACK timeout...resetting to predefined maximum." << endl; 
        buTimer->ackTimeout = ie->ipv6Data()->_getMaxLUAckTimeout();
    }

    EV << "Present Sent Time: " << buTimer->presentSentTimeBU << ", Present TimeOut: " << butimer->ackTimeout << endl;
    EV << "Next Sent Time: " << buTimer->nextScheduledTime << endl; 
    scheduleAt(buTimer->nextScheduledTime, msg);
}

void ILCache::createAndSendLUMessage(const IPv6Address& cNILNPAddress, InterfaceEntry* ie, const uint buSeq, const uint lifeTime)
{
    EV << "Creating and sending Binding Update" << endl;

    // to be written
    // or can be ie->ipv6Data()->getPrefferedAddress();
    // the above can be true if we use ILNP addresses rather than IP addresses for Interface.
    IPv6Address srcILNPAddress = ie->ipv6Data()->getILNPAddress();
    icmpv6->sendILNPLUMsg(cNILNPAddress,ie,buSeq,lifeTime);
}

void ILCache::ProcessReceivedLU(const IPv6Address& srcILNPAddress, const uint buSeq, 
  const uint lifeTime )
{
// we need to first seperate the Identifier and Locator from the source Address of the 
// received ILNP message( we dont explicitly store the Identifier and locator in the msg)
//@kbteja.. to be written
  IPv6Address srcIdentifier = srcILNPAddress->getIdentifier();
  IPv6Address srcLocator = srcILNPAddress->getLocator();

  ILCC::iterator it;
  // find the binding corresponding to the srcIdentifier.
  it = ilcc.find(srcIdentifier); 
  if(it==ilcc.end())
  {
    // handle error
    // The binding for the destination node not already registered.
  }
  else
  {
    ILCacheEntry cacheEntry = it->second;
    if(cacheEntry->recvSequenceNumber < buSeq)
    {
      L64Info locatorList = cacheEntry->remoteLocators;
      L64Info::iterator it;
      it = locatorList.find(srcLocator);
      if(it==locatorList.end())
      {
          L64Entry newLocator = L64Entry();
          locatorList[srcLocator] = newLocator;
          newLocator.locatorAddress = srcLocator;
          newLocator.flag = ILCCFlags.ACTIVE;
          newLocator.lifeTime = lifeTime;
      }
      else
      {
          L64Entry newLocator = locatorList[srcLocator];
          newLocator.flag = ILCCFlags.ACTIVE;
          newLocator.lifeTime = lifeTime;
      }

      // making the flag Valid from Active..@check.
      locatorList[cacheEntry.currActiveRemoteLocator].flag = ILCCFlags.valid;
      cacheEntry.currActiveRemoteLocator = srcLocator;
      createAndSendLUAckMsg(srcILNPAddress,ie,buSeq,lifeTime);
      // maintains the last receicved sequence number from the particular CN.
      cacheEntry.recvSequenceNumber = buSeq;
    }
  }
}

void ILCache::ProcessReceivedLUAck(const IPv6Address& srcILNPAddress, const uint buSeq, 
  const uint lifeTime )
{
  // we got an LU Acknowledgement from the Correspondent Node(CN). There was a timer
  // running to send periodic LU messages until an LU-ACK is received. This timer should
  // be cancelled.

  // Retrieve the timer object corresponding to the correspondent Node.
  // @kbtodo..
  //  need to write the methods getIdentifier() and getLocator().
  IPv6Address = srcILNPAddress->getIdentifier();
  IPv6Address = srcILNPAddress->getLocator();

  BindingUpdateTimer* luTimer = ilcc[srcIdentifier].buTimer;

  // cancel the timer message from the buTimer object.
  cancelAndDelete(luTimer->timer);

  // Now we need to update the buSequence Number in the ilcc to use it further 
  // when we send LU'sto the correspondent node.
  // doubtful..
  ilcc[srcIdentifier].sentSequenceNumber = buSeq; 
}

void ILCache::createAndSendLUAckMsg(const IPv6Address& destILNPAddress, 
  InterfaceEntry* ie, const uint buSeq, const uint lifeTime)
{
      EV << "Creating and sending Location Update  Acknowledgement" << endl;
      icmpv6->sendILNPLUAckMsg()
}

BindingUpdateTimer* ILCache::getBindingUpdateTimer(const IPv6Address& cNIdentifier)
{
  ILCacheEntry entry = ilcc[cNIdentifier];
  return entry.buTimer;
}




