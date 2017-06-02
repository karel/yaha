#ifndef DEVICE_H
#define DEVICE_H

#include <QTreeWidgetItem>

#include "hiddevice.h"

class Device : public QTreeWidgetItem
{
public:
    Device();
    Device(QString product, QString vid, QString pid, QString serial, HidDevice *device);
    HidDevice* getHidDevicePtr() {return m_device;};

signals:

public slots:

private:
    HidDevice *m_device = nullptr;
};

#endif // DEVICE_H
