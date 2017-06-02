# Yaha

Yaha is a Windows C++ API for interacting with [USB Human Interface Devices](http://en.wikipedia.org/wiki/USB_human_interface_device_class).
It allows to generate notifications when a device is added or removed and when an IO operation is complete.

## Usage

Devices can be found using an HidApi object which enumerates the connected HID devices. Device paths are mapped to device object pointers in a std::map container. After creating an HidApi object, devices can be iterated through or a specific device object matching certain product and vendor IDs can be requested from the API.

```C++
HidApi m_hid;
for(auto &x : m_hid.m_devices)
	std::wcout << (x.second)->getProduct();

HidDevice *m_device;
m_device = m_hid.getHidDevice(0x1000, 0x2000);
```

HidApi generates a notification when a device is added or removed. Callbacks must be set for the application to catch these notifications. Callbacks are stored in std::function wrappers and can be set using lambda functions.

```C++
#include "hidapi.h"

void init() {
	HidApi m_hid;
    m_hid.setCallbackArrival([this](HidDevice* d){return arrivalCallback(d);});
    m_hid.setCallbackRemoval([this](HidDevice* d){return removalCallback(d);});
}

void arrivalCallback(HidDevice *d) {}
void removalCallback(HidDevice *d) {}
```

Devices can be written to and read from either asynchronously (non-blocking) or isosynchronously (default). Also asynchronous read can be performed continuously.

```C++
void arrivalCallback(HidDevice *d)
{
	d->setCallbackReadComplete([this](HidDevice* d){return readCallback(d);});
	d->setWriteBlocking(true);
	d->setReadBlocking(false);
	d->setReadContinuous(true);
	d->write(buf);
}

void readCallback(HidDevice *d)
{
	std::cout << d->m_readBuf << std::endl;
}
```

## Building

### Qt
Include yaha.pri in your projects .pro file as is done in the example.

### Visual Studio
XXX
