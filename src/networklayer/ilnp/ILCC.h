
#include <algorithm>
#include "INETDefs.h"
#include "IPv6Address.h"
#include "ICMPv6.h"
#include <vector>
#include <map>

class InterfaceEntry;
class IPv6ControlInfo;
class IPv6Datagram;
class IPv6NeighbourDiscovery;
class RoutingTable6;
class NotificationBoard;


class INET_API ILCC : public cSimpleModule
{
  public:

    // 21.07.08 - CB
    enum ILCCFlags
    {
        ACTIVE,
        AGED,
        EXPIRED
    };
    
    class L64Entry
    {
    public:
        IPv6Address locatorAddress;
        ILCCFlags flag;
        int lifetime;
        // To maintain the info of the interface to which the Locator belongs to.
        // only used for source locators.
        InterfaceEntry *ie;
        // Timers and all the other stuff.
        // @kbteja.. I think the timers over here should be designed in such a way that
        // The freshness of Local Locator is ensured.
        // The timers need to ensure the freshness using the lifetime value of the
        // L64 entry.
    };
    
    // map the locator entry with the help of the locator.
    typedef std::map<IPv6Address,L64Entry> L64Info;

    // to maintain the currently valid list of local locators.
    std::map<InterfaceEntry*,L64Entry> localLocators;

    // saves time to fill in the Locator in packets coming from Transport Layer.
    IPv6Address currActiveLocalIdentifier;
    L64Entry currActiveLocalLocator;
    InterfaceEntry* currActiveInterface;

    class ILCCEntry
    {
      public:
        IPv6Address remoteIdentifier;

        // List of Remote Locators.
        L64Info remoteLocators;

        IPv6Address currActiveRemoteLocator;
        uint bindingLifetime;

        // This is the sequence number used while sending an LU to the CN.
        uint sentSequenceNumber;
        // This is the last sequence number sent by the MN to this CN.
        uint recvSequenceNumber;
        
        simtime_t bindingExpiry;
        simtime_t sentTime; 
        bool BAck;
        BindingUpdateTimer* buTimer; 
        virtual ~ILCCEntry() {};
    };

    // ?
    friend std::ostream& operator<<(std::ostream& os, const ILCCEntry& ilccEntry);

    // Maps the destination identifier to a particular ILCC entry.
    typedef std::map<IPv6Address,ILCCEntry> ILCache;
    // Main datastructure to maintain bindings.
    ILCache ilcc;

    class BindingUpdateTimer
    {
      public:
        cMessage* timer; // pointer to the scheduled timer message
        IPv6Address destIdentifier; // The ILNP Identifier of the node to which the msg is sent.
        IPv6Address destILNPAddress; // the ILNP address to which the message is sent.
        simtime_t ackTimeout; // timeout for the Ack
        simtime_t nextScheduledTime; // time when the corrsponding message is supposed to be sent
        InterfaceEntry* ifEntry;  // interface from which the message will be transmitted
        int buSequenceNumber;
        simtime_t presentSentTimeBU;
        simtime_t nextScheduledTime;
    };

  protected:
    ICMPv6 *icmpv6;
  public:
    ILCC();
    virtual ~ILCC();

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *);
  public:
    virtual void triggerHandoff(InterfaceEntry *ie, const IPv6Address& newLocator);
};
