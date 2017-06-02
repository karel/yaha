#include "hidapi.h"

#define WND_CLASS_NAME L"HidApi"
HINSTANCE g_hinst;

HidApi::HidApi()
{
    /* XXX failure handling. */
    bool res;
    res = registerWindow();
    if (res == false) {
        return;
    }
    res = createWindow();
    if (res == false) {
        return;
    }

	GUID InterfaceGuid;
	HidD_GetHidGuid(&InterfaceGuid);

	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceGuid;

	m_hDeviceNotify = RegisterDeviceNotification(
	        m_hwnd,
	        &NotificationFilter,
	        DEVICE_NOTIFY_ALL_INTERFACE_CLASSES
	        );

     if (m_hDeviceNotify == NULL)
         return;

	enumerate();
}

HidApi::~HidApi()
{
	for(auto &x : m_devices)
		delete (x.second);
	m_devices.clear();
}

bool HidApi::registerWindow()
{
	WNDCLASS wc;
	wc.style = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WND_CLASS_NAME;
	wc.hInstance = g_hinst; // GetModuleHandle(0)
	wc.lpfnWndProc = HidApi::s_wndProc;

	if(!RegisterClass(&wc))
		return false;
	else
		return true;
}

bool HidApi::createWindow()
{
	m_hwnd = CreateWindow(
	        WND_CLASS_NAME,
	        NULL, 0,
	        CW_USEDEFAULT, 0,
	        CW_USEDEFAULT, 0,
	        NULL, NULL,
	        g_hinst,
	        this);

	if (m_hwnd == NULL)
		return false;
	else
		return true;
}

bool HidApi::enumerate()
{
	GUID InterfaceClassGuid;
	HidD_GetHidGuid(&InterfaceClassGuid);
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	SP_DEVICE_INTERFACE_DETAIL_DATA *DeviceInterfaceDetailData = NULL;
	HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
	DWORD DeviceIndex;

	ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	DeviceIndex = 0;

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DeviceInfoSet = SetupDiGetClassDevs(
	        &InterfaceClassGuid,
	        NULL,
	        NULL,
	        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if(DeviceInfoSet == INVALID_HANDLE_VALUE)
		return false;

	while (SetupDiEnumDeviceInterfaces(
	               DeviceInfoSet,
	               NULL,
	               &InterfaceClassGuid,
	               DeviceIndex++,
	               &DeviceInterfaceData))
	{
		DWORD RequiredSize = 0;

		SetupDiGetDeviceInterfaceDetail(
		        DeviceInfoSet,
		        &DeviceInterfaceData,
		        NULL,
		        0,
		        &RequiredSize,
		        NULL);

		DeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(RequiredSize);
		DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		SetupDiGetDeviceInterfaceDetail(
		        DeviceInfoSet,
		        &DeviceInterfaceData,
		        DeviceInterfaceDetailData,
		        RequiredSize,
		        NULL,
		        NULL);

		std::wstring path = DeviceInterfaceDetailData->DevicePath;

		HidDevice *CurrentDevice =  new HidDevice(path);
        if(!CurrentDevice->open()) {
			delete CurrentDevice;
			continue;
		}

		m_devices[CurrentDevice->getPath()] = CurrentDevice;

		free(DeviceInterfaceDetailData);
        CurrentDevice->close();
	}

	if (DeviceInfoSet)
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);

	return true;
}

LRESULT CALLBACK HidApi::s_wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HidApi *pThis;

	if (uMsg == WM_NCCREATE) {
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = static_cast<HidApi*>(lpcs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else
		pThis = reinterpret_cast<HidApi*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (pThis)
        return pThis->wndProc(hwnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT HidApi::wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DEVICECHANGE && lParam != 0) {
		DEV_BROADCAST_DEVICEINTERFACE *const d = (DEV_BROADCAST_DEVICEINTERFACE*)lParam;
		switch(wParam) {
		case DBT_DEVICEARRIVAL:
			devAdded(*d);
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			devRemoved(*d);
			break;
		}
		return true;
	} else
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void HidApi::devAdded(DEV_BROADCAST_DEVICEINTERFACE &d)
{
	GUID InterfaceGuid;
	HidD_GetHidGuid(&InterfaceGuid);

	if(d.dbcc_classguid != InterfaceGuid)
		return;

	/* dbcc_name is mixed case, DevicePath in DevInterfaceDetailData
     * is lower case */
    std::wstring path = d.dbcc_name;
    std::transform (path.begin(), path.end(), path.begin(), tolower);

    /* If an object for this device already exists, set its state to connected,
     * otherwise create new object and add to container. */
    if (m_devices[path] != nullptr) {
        m_devices[path]->connected();
        if(m_callbackArrival)
            m_callbackArrival(m_devices[path]);
    } else {
        HidDevice *Device = new HidDevice(path);
        if(!Device->open()) {
            delete Device;
        } else {
            m_devices[path] = Device;
            Device->close();
        }

        if(m_callbackArrival)
            m_callbackArrival(Device);
    }
    return;
}

void HidApi::devRemoved(DEV_BROADCAST_DEVICEINTERFACE &d)
{
	GUID InterfaceGuid;
	HidD_GetHidGuid(&InterfaceGuid);

	if(d.dbcc_classguid != InterfaceGuid)
		return;

	std::wstring path = d.dbcc_name;
	std::transform (path.begin(), path.end(), path.begin(), tolower);

	HidDevice *Device = m_devices[path];
	if(!Device)
		return;

    Device->removed();

	std::function<void(HidDevice*)> cb = Device->getCallbackRemoval();
	if(cb != nullptr)
        cb(Device);

	if(m_callbackRemoval)
		m_callbackRemoval(Device);

	return;
}

HidDevice* HidApi::getHidDevice(unsigned short vid, unsigned short pid)
{
	for (auto &x : m_devices) {
		if ((x.second)->getVid() == vid && (x.second)->getPid() == pid)
			return x.second;
	}

    return nullptr;
}

HidDevice* HidApi::getHidDevice(std::wstring path)
{
	for (auto &x : m_devices) {
		if ((x.second)->getPath() == path)
			return x.second;
	}

    return nullptr;
}
