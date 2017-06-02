#ifndef HIDAPI_H
#define HIDAPI_H

#include <windows.h>
#include <dbt.h>
extern "C"
{
#include <hidsdi.h>
}

#include <algorithm>
#include <functional>
#include <map>
#include <string>

#include "hiddevice.h"

//! HidApi class
/*!
 * Enumerates available HID devices (HidDevice) and provides callback interface for removed and added devices
 */

class HidApi
{
	public:
		//! Initializes the API and enumerates HID devices
		HidApi();
		//! Cleans up, removes all device objects
		~HidApi();

		//! Returns pointer to the device with specified vendor and product id
		/*!
		 * \param vid	Vendor ID
		 * \param pid	Product ID
		 */
		HidDevice *getHidDevice(unsigned short vid, unsigned short pid);
		//! Returns pointer to the device with specified path
		/*!
		 * \param path	Device path
		 */
		HidDevice *getHidDevice(std::wstring path);

		//! Enumerates all HID devices present in the system
		bool enumerate();
		//! Sets the function to be called when a new HID device is added
		/*!
		 * \param cb	The function to call when a device arrives
		 */
        void setCallbackArrival(std::function<void(HidDevice*)> cb) {m_callbackArrival = cb;};
		//! Sets the function to be called when a HID device is removed
		/*!
		 * \param cb	The function to call when a device is removed
		 */
        void setCallbackRemoval(std::function<void(HidDevice*)> cb) {m_callbackRemoval = cb;};

        //! Container for maping device paths to device objects
        std::map<std::wstring, HidDevice*> m_devices;

	protected:
		//! Window handle
		HWND m_hwnd;
		//! Device notification handle
		HDEVNOTIFY m_hDeviceNotify;

	private:
		/*!
		 * Creates a window which can receive device notifications.
		 * Use GetLastError() to obtain more specific error information.
		 * \return		Return value of CreateWindow
		 */
		bool createWindow();
		/*!
		 * Registers a window class for subsequent use in call to the CreateWindow.
		 * Use GetLastError() to obtain more specific error information.
		 * \return		Return value of RegisterClass
		 */
		bool registerWindow();

		/*!
		 * Receives notification and passes it to the class member.
		 * \param hwnd	Window handle
		 * \param uMsg	The message
		 * \param wParam	Additional message information
		 * \param lParam	Additional message information
		 */
		static LRESULT CALLBACK s_wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		/*!
		 * Passes device notification to devAdded or devRemoved.
		 * \param hwnd	Window handle
		 * \param uMsg	The message
		 * \param wParam	Additional message information
		 * \param lParam	Additional message information
		 */
		LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/*!
		 * Inserts the added device into m_devices and calls user-defined callback.
		 * \param d		Identifies the device which generated the notification
		 */
		void devAdded(DEV_BROADCAST_DEVICEINTERFACE &d);
		/*!
		 * Closes the device and calls user-defined callback.
		 * \param d		Identifies the device which generated the notification
		 */
		void devRemoved(DEV_BROADCAST_DEVICEINTERFACE &d);

		//! User-defined callback for device arrivals
		std::function<void(HidDevice*)> m_callbackArrival = nullptr;
		//! User-defined callback for device removals
		std::function<void(HidDevice*)> m_callbackRemoval = nullptr;

};

#endif // HIDAPI_H
