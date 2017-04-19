#include <iostream>
#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>
#include <IPv4Layer.h>

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
    if(ipLayer != NULL) {
        if (dev->getIPv4Address() != ipLayer->getSrcIpAddress()) {
            return;
        }

        unsigned int prefix = ipLayer->getDstIpAddress().toInt();
        bool is_broadcast = (prefix == UINT32_MAX);
        prefix >>= 24;

        if (prefix < 224 || (prefix > 239 && !is_broadcast)) {
            return;
        }
        cout << "Paket wird gesendet" << endl;

        ipLayer->setSrcIpAddress(settings->vpn_device->getIPv4Address());
    } else {
        return;
    }

    if(!settings->vpn_device->sendPacket(*pkt)) {
        puts("Broadcast konnte nicht weitergeleitet werden.\n");
    }
}

int main() {
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


    VPNDeviceWrapper data;
    data.vpn_device = vpn_device;

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
    main_device->startCapture(onPacketReceive, &data);

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
