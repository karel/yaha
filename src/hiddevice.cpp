#include "hiddevice.h"

HidDevice::HidDevice()
{
    m_overlapped.Internal = 0;
    m_overlapped.InternalHigh = 0;
    m_overlapped.Pointer = 0;
    m_overlapped.hEvent = 0;
    m_attributes.Size = sizeof(HIDD_ATTRIBUTES);
}

HidDevice::HidDevice(std::wstring path)
{
    m_overlapped.Internal = 0;
    m_overlapped.InternalHigh = 0;
    m_overlapped.Pointer = 0;
    m_overlapped.hEvent = 0;
    m_attributes.Size = sizeof(HIDD_ATTRIBUTES);
    m_path = path;
}

HidDevice::~HidDevice()
{
    m_connected = false;
    if(m_readThread.joinable())
        m_readThread.join();
    if(m_writeThread.joinable())
        m_writeThread.join();
    if(m_overlapped.hEvent)
        CloseHandle(m_overlapped.hEvent);
    if(isOpen())
        CloseHandle(m_handle);
    if(m_readBuf != nullptr) {
        delete(m_readBuf);
        m_readBuf = nullptr;
    }
}

bool HidDevice::open()
{
    HIDP_CAPS caps;
    PHIDP_PREPARSED_DATA pp_data = NULL;
    BOOL res;
    NTSTATUS nt_res;
    DWORD DesiredAccess = GENERIC_WRITE | GENERIC_READ;
    DWORD SharedMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    BYTE sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = &sd;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

    m_handle = CreateFileW(
                m_path.c_str(),
                DesiredAccess,
                SharedMode,
                &sa,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                0);

    if (m_handle == INVALID_HANDLE_VALUE)
        return false;

    m_overlapped.hEvent = CreateEventW(
                NULL,   // A pointer to a SECURITY_ATTRIBUTES structure, optional.
                TRUE,   // Manual reset (true). Requires ResetEvent to set state to nonsignaled.
                FALSE,  // Initial state is nonsignaled (false).
                NULL);  // Name of the event object, optional.

    if (m_overlapped.hEvent == NULL)
        return false;

    /* Set the maximum number of input reports that the HID class driver ring buffer can hold for a specified top-level collection. */
    res = HidD_SetNumInputBuffers(m_handle, 64);
    if (!res)
        return false;

    /* Preparsed data is report descriptor data associated with a top-level collection. User-mode applications or kernel-mode drivers
     * use preparsed data to extract information about specific HID controls without having to obtain and interpret a device's entire
     * report descriptor. */
    res = HidD_GetPreparsedData(m_handle, &pp_data);
    if (!res)
        return false;

    /* Returns top-level collection's HIDP_CAPS structure. */
    nt_res = HidP_GetCaps(pp_data, &caps);
    if (nt_res != HIDP_STATUS_SUCCESS)
        return false;

    m_outputReportLength = caps.OutputReportByteLength;
    m_inputReportLength = caps.InputReportByteLength;
    m_usagePage = caps.UsagePage;
    m_usage = caps.Usage;
    /* Release the resources that the HID class driver allocated to hold a top-level collection's preparsed data. */
    res = HidD_FreePreparsedData(pp_data);
    if (!res)
        return false;

    if(m_readBuf != nullptr)
        delete m_readBuf;
    m_readBuf = new unsigned char[m_inputReportLength];
    if(m_readBuf == nullptr)
        return false;

    HidD_GetAttributes(m_handle, &m_attributes);

    if (m_attributes.VendorID != 0x00 && m_attributes.ProductID != 0x00) {

#define WSTR_LEN 512
        wchar_t wstr[WSTR_LEN]; /* XXX Determine Size */

        res = HidD_GetSerialNumberString(m_handle, wstr, sizeof(wstr));
        wstr[WSTR_LEN-1] = 0x0000;
        if (res)
            m_serialNumber = wstr;

        res = HidD_GetManufacturerString(m_handle, wstr, sizeof(wstr));
        wstr[WSTR_LEN-1] = 0x0000;
        if (res)
            m_manufacturer = wstr;

        res = HidD_GetProductString(m_handle, wstr, sizeof(wstr));
        wstr[WSTR_LEN-1] = 0x0000;
        if (res)
            m_product = wstr;

        return true;
    } else
        return false;
}

bool HidDevice::close()
{
    m_closing = true;
    if(m_readThread.joinable())
        m_readThread.join();
    if(m_writeThread.joinable())
        m_writeThread.join();
    m_closing = false;

    BOOL res;
    CloseHandle(m_overlapped.hEvent); /* XXX is this correct? */
    res = CloseHandle(m_handle);
    if(!res)
        return false;
    else
        m_handle = INVALID_HANDLE_VALUE;

    if(m_readBuf != nullptr) {
        delete(m_readBuf);
        m_readBuf = nullptr;
    }
    return true;
}

void HidDevice::removed()
{
    m_connected = false;

    if(isOpen())
        close();
}

void HidDevice::readThread()
{
    do {
        DWORD res;
        ReadFileEx(m_handle, m_readBuf, m_inputReportLength, &m_overlapped,
                   fileReadIOComplete);

        do {
            res = WaitForSingleObjectEx(
                        m_overlapped.hEvent,
                        TIMEOUT,     // Time-out interval, in milliseconds.
                        TRUE);
        } while (res != WAIT_IO_COMPLETION && m_connected && !m_closing);
        if (!m_connected || m_closing)
            return;

        DWORD bytesTransferred = 0;
        BOOL overlappedResult = GetOverlappedResult(m_handle,
                                       &m_overlapped, &bytesTransferred, false);

        if (!bytesTransferred || !overlappedResult) {
            ResetEvent(m_overlapped.hEvent);
            continue;
        } else {
            ResetEvent(m_overlapped.hEvent);
            if(m_callbackReadComplete)
                m_callbackReadComplete(this);
        }
    } while (m_readContinuous && m_connected && !m_closing);
    return;
}

bool HidDevice::read()
{
    if(m_readBlocking) {
        DWORD res;
        ReadFileEx(m_handle, m_readBuf, m_inputReportLength, &m_overlapped,
                   fileReadIOComplete);

        do {
            res = WaitForSingleObjectEx(
                        m_overlapped.hEvent,
                        TIMEOUT,
                        TRUE);
        } while (res != WAIT_IO_COMPLETION && m_connected && !m_closing);
        if (!m_connected || m_closing)
            return false;

        DWORD bytesTransferred = 0;
        BOOL overlappedResult = GetOverlappedResult(m_handle,
                                       &m_overlapped, &bytesTransferred, false);

        if (!bytesTransferred || !overlappedResult) {
            ResetEvent(m_overlapped.hEvent);
            return false;
        } else {
            ResetEvent(m_overlapped.hEvent);
        }
    } else {
        if(m_readThread.joinable())
            m_readThread.join();
        m_readThread = std::move(std::thread ([this](){this->readThread();}));
    }
    return true;
}

void HidDevice::writeThread(LPVOID b)
{
    DWORD res;
    WriteFileEx(m_handle, b, m_outputReportLength, &m_overlapped,
                fileWriteIOComplete);

    res = WaitForSingleObjectEx(
                m_overlapped.hEvent,    // event object to wait for
                TIMEOUT,                // wait timeout
                TRUE);                  // alertable wait enabled

    switch (res) {

    // The wait was ended by one or more user-mode
    // asynchronous procedure calls (APC) queued to the thread.
    case WAIT_IO_COMPLETION:
        break;

    // An error occurred in the wait function.
    default:
        break;
    }

#if 0
    do {
        res = WaitForSingleObjectEx(
                    m_overlapped.hEvent,
                    TIMEOUT,
                    TRUE);
    } while (res != WAIT_IO_COMPLETION && m_connected && !m_closing);
    if (!m_connected || m_closing)
        return;
#endif
    ResetEvent(m_overlapped.hEvent);

    if(m_callbackWriteComplete)
        m_callbackWriteComplete(this);

    return;
}

bool HidDevice::write(LPVOID b)
{
    if(b == nullptr)
        return false;

    if(m_writeBlocking) {
        DWORD res;
        WriteFileEx(m_handle, b, m_outputReportLength, &m_overlapped,
                    fileWriteIOComplete);

        res = WaitForSingleObjectEx(
                    m_overlapped.hEvent,    // event object to wait for
                    TIMEOUT,                // wait timeout
                    TRUE);                  // alertable wait enabled

        switch (res) {

        // The wait was ended by one or more user-mode
        // asynchronous procedure calls (APC) queued to the thread.
        case WAIT_IO_COMPLETION:
            break;

        // An error occurred in the wait function.
        default:
            break;
        }

#if 0
        do {
            res = WaitForSingleObjectEx(
                        m_overlapped.hEvent,
                        TIMEOUT,
                        TRUE);
        } while (res == WAIT_TIMEOUT && m_connected);
#endif
        if (!m_connected || m_closing)
            return false;
        ResetEvent(m_overlapped.hEvent);
    } else {
        if(m_writeThread.joinable())
            m_writeThread.join();
        m_writeThread = std::move(std::thread ([this, &b](){this->writeThread(b);}));
    }
    return true;
}

/* XXX Completion routines are rather useless at the moment
 * as it does not seem to be possible to pass the device to
 * them. Should change to ReadFile/WriteFile? */

VOID WINAPI HidDevice::fileReadIOComplete(DWORD dwErrorCode,
                                          DWORD dwNumberOfBytesTransfered,
                                          LPOVERLAPPED lpOverlapped)
{
    (void)dwErrorCode;
    (void)dwNumberOfBytesTransfered;
    (void)lpOverlapped;
#if 0
    if(m_callbackReadComplete)
        m_callbackReadComplete();
#endif
}

VOID WINAPI HidDevice::fileWriteIOComplete(DWORD dwErrorCode,
                                           DWORD dwNumberOfBytesTransfered,
                                           LPOVERLAPPED lpOverlapped)
{
    (void)dwErrorCode;
    (void)dwNumberOfBytesTransfered;
    (void)lpOverlapped;
#if 0
    if(m_callbackWriteComplete)
        m_callbackWriteComplete();
#endif
}
