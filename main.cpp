#include <iostream>
#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>
#include <IPv4Layer.h>
#include <multidirection.cpp>

using namespace std;
using namespace pcpp;

struct VPNDeviceWrapper {
    PcapLiveDevice* vpn_device;
};

void onPacketReceive(RawPacket* pkt, PcapLiveDevice* dev, void* cookie) {
    Packet packet(pkt);

    VPNDeviceWrapper* settings = (VPNDeviceWrapper*) cookie;

    if (!packet.isPacketOfType(UDP))
        return;

    IPv4Layer* ipLayer = packet.getLayerOfType<IPv4Layer>();

    if(ipLayer != NULL)
    {
        if (dev->getIPv4Address() != ipLayer->getSrcIpAddress())
        {
            return;
        }

        std::string sDestinationAddress = ipLayer->getDstIpAddress().toString();
        std::string sPrefix = sDestinationAddress.substr(0, sDestinationAddress.find('.'));
        bool is_broadcast = (stoul(sDestinationAddress) == UINT32_MAX);
        int prefix = stoul(sPrefix);

        cout << prefix;

        if (prefix < 224 || (prefix > 239 && !is_broadcast))
        {
            return;
        }
        cout << ipLayer->getDstIpAddress().toString();
        cout << "Paket wird gesendet" << endl;

        ipLayer->setSrcIpAddress(settings->vpn_device->getIPv4Address());
    } else {
        return;
    }

    if(!settings->vpn_device->sendPacket(*pkt)) {
        puts("Broadcast konnte nicht weitergeleitet werden.\n");
    }
}



int main()
{
    vector<PcapLiveDevice*> device_list = (&pcpp::PcapLiveDeviceList::getInstance())->getPcapLiveDevicesList();

    if (device_list.size() < 2)
    {
        cout << "Wahrscheinlich wurde OpenVPN noch nicht gestartet. Bitte starte das Programm neu." << endl;
    }

    PcapLiveDevice* main_device = NULL;
    while(main_device == NULL)
    {
        for (unsigned int i = 0; i < device_list.size(); i++)
        {
#ifdef __linux__
            cout << "[" << i << "] " << device_list[i]->getName() << endl;
#elif defined(_WIN32) || defined(WIN32)
            cout << "[" << i << "] " << device_list[i]->getDesc() << endl;
#endif
        }

        cout << "Waehle das Haupt-Interface: ";
        unsigned int choice = UINT32_MAX;

        try
        {
            std::string input;
            std::cin >> input;
            choice = stoul(input);
        }
        catch (exception &e)
        {
            cout << e.what();
        }

        if (choice < device_list.size())
        {
            main_device = device_list[choice];
            device_list.erase(device_list.begin() + choice);
        } else {
            cout << "Ungueltige Auswahl." << endl;
        }
    }

    PcapLiveDevice* vpn_device = NULL;
    if (device_list.size() == 1)
    {
        vpn_device = device_list[0];
    }

    while(vpn_device == NULL)
    {
        for (unsigned int i = 0; i < device_list.size(); i++)
        {
#ifdef __linux__
            cout << "[" << i << "] " << device_list[i]->getName() << endl;
#elif defined(_WIN32) || defined(WIN32)
            cout << "[" << i << "] " << device_list[i]->getDesc() << endl;
#endif
        }

        cout << "Waehle das VPN-Interface: ";
        unsigned int choice = UINT32_MAX;

        try
        {
            std::string input;
            std::cin >> input;
            choice = stoul(input);
        }
        catch (exception& e)
        {
            cout << e.what();
        }

        if (choice < device_list.size())
        {
            vpn_device = device_list[choice];
        } else {
            cout << "Ungueltige Auswahl." << endl;
        }
    }

    puts("Oeffne Haupt-Device.");
    if (!main_device->open())
    {
        puts("Konnte das Haupt-Device nicht oeffnen.");
        return 1;
    }

    puts("Oeffne VPN-Device.");
    if (!vpn_device->open())
    {
        puts("Konnte das VPN-Device nicht oeffnen.");
        return 2;
    }

    puts("Yay. Alle relevanten Pakete werden jetzt umgeleitet.");
    puts("Schreibe stop, um das Programm sauber zu beenden.");

    multidir::SettingsWrapper vpn_settings;
    vpn_settings.mainDevice = main_device;
    vpn_settings.vpnDevice  = vpn_device;
    vpn_settings.mode       = multidir::INCOMING;

    multidir::SettingsWrapper main_settings;
    main_settings.mainDevice = main_device;
    main_settings.vpnDevice  = vpn_device;
    main_settings.mode       = multidir::OUTGOING;

    main_device->startCapture(multidir::onPacketReceive, &main_settings);
    vpn_device->startCapture(multidir::onPacketReceive, &vpn_settings);

    bool shouldStop = false;

    while (!shouldStop)
    {
        std::string s;
        std::cin >> s;

        if (strcmp(s.c_str(), "stop") == 0) shouldStop = true;
    }

    main_device->stopCapture();
    main_device->close();
    vpn_device->close();
    puts("Das Programm wurde sauber beendet.");
    return 0;
}
