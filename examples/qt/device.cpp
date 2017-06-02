#include "device.h"

Device::Device()
{
    setText(0, "No device");
}

Device::Device(QString product, QString vid, QString pid, QString serial, HidDevice *device) :
    m_device(device)
{
    setText(0, product);
    setText(1, vid);
    setText(2, pid);
    setText(3, serial);
}
