#ifndef HIDDEVICE_H
#define HIDDEVICE_H

#define _UNICODE
#define TIMEOUT 50

#include <windows.h>
#include <winioctl.h>
#include <dbt.h>
#include <setupapi.h>
extern "C"
{
#include <hidsdi.h>
}

#include <functional>
#include <string>
#include <thread>

//! HidDevice class
/*!
 * Represents a HID device
 */

class HidDevice
{
	public:
		//! Initializes the OVERLAPPED structure
		HidDevice();
		//! Initializes the OVERLAPPED structure and sets device path
		HidDevice(std::wstring path);
		//! Closes device handle, deletes buffer and joins separate threads
		~HidDevice();

		//! Open the device for I/O
		/*!
		 * \return		True if valid handle
		 */
		bool open();
		//! Close the device for I/O
		/*!
		 * \return		True if closed properly
		 */
		bool close();
		//! Check if device ready for I/O and has a valid handle
		/*!
         * \return		False if device not ready for I/O
		 */
        bool isOpen() {return INVALID_HANDLE_VALUE != m_handle;}
        //! Signal the device object that the device has been removed
        /*!
         * Closes the device and marks as removed.
         */
        void removed();
        //! Signal the device object that the device has been reconnected
        /*!
         * Simply marks the device as connected.
         */
        void connected() {m_connected = true;};
        //! Check if the device is connected
        /*!
         * \return  true if connected, false otherwise
         */
        bool isConnected() {return m_connected;};

		//! Function
		/*!
         * \return Product ID
		 */
        unsigned short getPid() {return m_attributes.ProductID;};
		//! Function
		/*!
         * \return Vendor ID
		 */
        unsigned short getVid() {return m_attributes.VendorID;};
		//! Function
		/*!
         * \return Version number
		 */
        unsigned short getVersionNumber() {return m_attributes.VersionNumber;};
        //! Get the device path
        std::wstring getPath() {return m_path;};
        //! Get the device manufacturer string
        std::wstring getManufacturer() {return m_manufacturer;};
        //! Get the device product string
        std::wstring getProduct() {return m_product;};
        //! Get the device serial number string
        std::wstring getSerialNumber() {return m_serialNumber;};

		//! Set the function to be called when device is removed
		/*!
		 * \param cb	Callback
		 */
        void setCallbackRemoval(std::function<void(HidDevice*)> cb) {m_callbackRemoval = cb;};
		//! Get the function to be called when device is removed
		/*!
		 * \return		Callback or nullptr
		 */
		std::function<void(HidDevice*)> getCallbackRemoval() {return m_callbackRemoval;};
        //! Set the function to be called when non-blocking read is completed
        /*!
         * \param cb	Callback
         */
        void setCallbackReadComplete(std::function<void(HidDevice*)> cb) {m_callbackReadComplete = cb;};
		//! Set the function to be called when non-blocking write is completed
		/*!
		 * \param cb	Callback
		 */
		void setCallbackWriteComplete(std::function<void(HidDevice*)> cb) {m_callbackWriteComplete = cb;};

		//! Set read to be blocking or non-blocking
		/*!
		 * \param a		true - blocking, false - non-blocking
		 */
		void setReadBlocking(bool a) {m_readBlocking = a;};
        //! Set the non-blocking variant of read to read continuously or only once
        /*!
         * \param a     true - continuous read, false - single read
         */
        void setReadContinuous(bool a) {m_readContinuous = a;};
		//! Read from the device
		/*!
         * Read from the device (blocking by default)
		 */
		bool read();
		//! Set write to be blocking or non-blocking
		/*!
		 * \param a		true - blocking, false - non-blocking
		 */
		void setWriteBlocking(bool a) {m_writeBlocking = a;};
		//! Write data to the device
		/*!
         * \param b     Pointer to the data to write
         * Write to the device (blocking by default). User is
         * responsible for providing a buffer of exact length
         * (m_outputReportLength).
		 */
        bool write(LPVOID b);
        //! Run in different thread to provide asynchronous reading
		/*!
         * Waits in alertable state for asynchronous read to complete.
		 */
		void readThread();
        //! Run in different thread to provide asynchronous writing
        /*!
         * Waits in alertable state for asynchronous write to complete.
         * \param b     Pointer to the data to write
         */
        void writeThread(LPVOID b);

		/* XXX as it seems difficult? to relate fileReadIOComplete to a object
		 * and call it's callbacks then maybe invoke callbacks from readThread()
		 * and write() */

		//! Function
		/*!
		 * Called when packet read from device
		 */
		static VOID WINAPI fileReadIOComplete(DWORD, DWORD, LPOVERLAPPED);
		//! Function
		/*!
		 * Called when done writing to device
		 */
		static VOID WINAPI fileWriteIOComplete(DWORD, DWORD, LPOVERLAPPED);

        //! Read buffer
        unsigned char *m_readBuf = nullptr;

	private:
		//! Device handle
        HANDLE m_handle = INVALID_HANDLE_VALUE;
		//! The HIDD_ATTRIBUTES structure contains vendor information about a HIDClass device
		HIDD_ATTRIBUTES m_attributes;
		//! Contains information used in asynchronous (or overlapped) input and output (I/O)
        OVERLAPPED m_overlapped;
		//! Specifies the maximum size, in bytes, of all the input reports (including the report ID, if report IDs are used, which is prepended to the report data)
        size_t m_inputReportLength = 0;
		//! Specifies the maximum size, in bytes, of all the output reports (including the report ID, if report IDs are used, which is prepended to the report data)
        size_t m_outputReportLength = 0;
        //! Device path
        std::wstring m_path;
        //! Serial number
        std::wstring m_serialNumber;
        //! Manufacturer
        std::wstring m_manufacturer;
        //! Product
        std::wstring m_product;
        //! Specifies the top-level collection's usage page
		/*!
		 * HID usages are organized into usage pages of related controls. A specific control usage is defined by its usage page, a usage ID, a name, and a description.
		 * Examples of usage pages include Generic Desktop Controls, Game Controls, LEDs, Button, and so on. Examples of controls that are listed on the Generic Desktop
		 * Controls usage page include pointers, mouse and keyboard devices, joysticks, and so on. A usage page value is a 16-bit unsigned value.
		 */
        unsigned short m_usagePage = 0;
		//! Specifies a top-level collection's usage ID
		/*!
		 * In the context of a usage page, a valid usage identifier, or usage ID, is a positive integer greater than zero that indicates a usage in a usage page.
		 * A usage ID of zero is reserved. A usage ID value is an unsigned 16-bit value.
		 */
		unsigned short m_usage = 0;

		//! Non-blocking read thread
		std::thread m_readThread;
		//! Non-blocking write thread
		std::thread m_writeThread;
		//! Determines if read is blocking
		bool m_readBlocking = true;
        //! Determines if read is continuous
        bool m_readContinuous = false;
		//! Determines if write is blocking
		bool m_writeBlocking = true;

        //! Marks if the device is connected or has been removed
        bool m_connected = true;
        //! Set to true when closing to notify threads
        bool m_closing = false;

		//! User-defined callback for device removal
		std::function<void(HidDevice*)> m_callbackRemoval = nullptr;
        //! User-defined callback for read complete
        std::function<void(HidDevice*)> m_callbackReadComplete = nullptr;
        //! User-defined callback for write complete
        std::function<void(HidDevice*)> m_callbackWriteComplete = nullptr;
};

#endif // HIDDEVICE_H
