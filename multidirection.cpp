#include <PcapLiveDeviceList.h>
#include <IPv4Layer.h>

namespace multidir
{
    enum Mode { INCOMING, OUTGOING };

    struct SettingsWrapper
    {
        pcpp::PcapLiveDevice* vpnDevice;
        pcpp::PcapLiveDevice* mainDevice;
        Mode mode;
    };

    bool packetIsSomeCast(pcpp::Packet* packet)
    {
        if(!packet->isPacketOfType(pcpp::UDP)) return false;

        pcpp::IPv4Layer* ipLayer = packet->getLayerOfType<pcpp::IPv4Layer>();

        if(ipLayer == 0) return false;

        std::string destIp = ipLayer->getDstIpAddress().toString();
        std::string destPrefixString = destIp.substr(0, destIp.find('.'));
        int destPrefix = std::stoul(destPrefixString);

        bool isBroadcast = (std::stoul(destIp) == UINT32_MAX);
        if(isBroadcast) return true;

        bool isMulticast = ((destPrefix > 223) && (destPrefix < 240));
        if(isMulticast) return true;

        return false;
    }

    void onPacketReceive(pcpp::RawPacket* pkt, pcpp::PcapLiveDevice* dev, void* cookie)
    {
        SettingsWrapper* settings = (SettingsWrapper* ) cookie;

        pcpp::PcapLiveDevice* forwardDevice;
        if(dev == settings->mainDevice)
        {
            forwardDevice = settings->vpnDevice;
        }
        else if (dev == settings->vpnDevice)
        {
            forwardDevice = settings->mainDevice;
        }
        else
        {
            return;
        }

        pcpp::Packet packet(pkt);
        if(!packetIsSomeCast(&packet)) return; //Also tests for UDP

        pcpp::IPv4Layer* ipLayer = packet.getLayerOfType<pcpp::IPv4Layer>();

        if(settings->mode == INCOMING)
        {
            if(ipLayer->getSrcIpAddress() == settings->mainDevice->getIPv4Address()) return;
        }
        else if(settings->mode == OUTGOING)
        {
            if(ipLayer->getSrcIpAddress() != settings->mainDevice->getIPv4Address()) return;
            ipLayer->setSrcIpAddress(settings->vpnDevice->getIPv4Address());
        }
        else
        {
            return;
        }

        if(!forwardDevice->sendPacket(*pkt))
        {
            std::cout << "Paket konnte nicht gesendet werden";
        }
    }
}


