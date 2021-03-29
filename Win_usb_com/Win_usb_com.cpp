/* influenced by: https://aticleworld.com/serial-port-programming-using-win32-api/*/

#include <iostream>
#include <Windows.h>
#include <string.h>

struct header {
    uint32_t msgno;
    uint32_t len;
};

#define BASE_ADDR 0x1000
#define COUNTER_REQ BASE_ADDR + 0x1

struct counter_req {
    struct header header;
};
#define COUNTER_RSP BASE_ADDR + 0x2

struct counter_rsp {
    struct header header;
    uint32_t value;
};

union usb_packets {
    struct header header;
    struct counter_req counter_req;
    struct counter_rsp counter_rsp;
};

struct data
{
    HANDLE hComm;  // Handle to the Serial port
    BOOL   Status; // Status
    DCB dcbSerialParams = { 0 };  // Initializing DCB structure
    COMMTIMEOUTS timeouts = { 0 };  //Initializing timeouts structure
    char SerialBuffer[64] = { 0 }; //Buffer to send and receive data
    DWORD BytesWritten = 0;          // No of bytes written to the port
    DWORD dwEventMask;     // Event mask to trigger
    char  ReadData;        //temperory Character
    DWORD NoBytesRead;     // Bytes read by ReadFile()
    unsigned char loop = 0;
    wchar_t pszPortName[10] = { 0 }; //com port id
    LPCSTR PortNo; //contain friendly name
};

static int init(data* data, LPCSTR PortNo)
{
    memset(data, 0, sizeof(*data));

    data->PortNo = PortNo;
	
    data->hComm = CreateFile(data->PortNo, //friendly name
        GENERIC_READ | GENERIC_WRITE,      // Read/Write Access
        0,                                 // No Sharing, ports cant be shared
        NULL,                              // No Security
        OPEN_EXISTING,                     // Open existing port only
        0,                                 // Non Overlapped I/O
        NULL);                             // Null for Comm Devices
    if (data->hComm == INVALID_HANDLE_VALUE)
    {
        printf_s("\n Port can't be opened\n\n");
        return 2;
        //goto Exit2;
    }
    //Setting the Parameters for the SerialPort
    data->dcbSerialParams.DCBlength = sizeof(data->dcbSerialParams);
    data->Status = GetCommState(data->hComm, &data->dcbSerialParams); //retreives  the current settings
    if (data->Status == FALSE)
    {
        printf_s("\nError to Get the Com state\n\n");
        return 1;
        //goto Exit1;
    }
    data->dcbSerialParams.BaudRate = CBR_9600;      //BaudRate = 9600
    data->dcbSerialParams.ByteSize = 8;             //ByteSize = 8
    data->dcbSerialParams.StopBits = ONESTOPBIT;    //StopBits = 1
    data->dcbSerialParams.Parity = NOPARITY;      //Parity = None
    data->Status = SetCommState(data->hComm, &data->dcbSerialParams);
    if (data->Status == FALSE)
    {
        printf_s("\nError to Setting DCB Structure\n\n");
        //goto Exit1;
        return 1;
    }
    //Setting Timeouts
    data->timeouts.ReadIntervalTimeout = 50;
    data->timeouts.ReadTotalTimeoutConstant = 50;
    data->timeouts.ReadTotalTimeoutMultiplier = 10;
    data->timeouts.WriteTotalTimeoutConstant = 50;
    data->timeouts.WriteTotalTimeoutMultiplier = 10;
    if (SetCommTimeouts(data->hComm, &data->timeouts) == FALSE)
    {
        printf_s("\nError to Setting Time outs");
        //goto Exit1;
        return 1;
    }
	
    return 0;

}

static void deinit(const int val, data* data)
{
	if(val == 1) CloseHandle(data->hComm);//Closing the Serial Port
}

static int read_data(data *data)
{
    data->loop = 0;

    //Setting WaitComm() Event
    data->Status = WaitCommEvent(data->hComm, &data->dwEventMask, NULL); //Wait for the character to be received
    if (data->Status == FALSE)
    {
        printf_s("\nError! in Setting WaitCommEvent()\n\n");
        return 1;
//        status = 1;
//        goto Exit;
    }

    //Read data and store in a buffer
    counter_rsp rsp;
    void *data_void = static_cast<void*>(&rsp);
    char *data_char = static_cast<char*>(data_void);

    //Read data and store in a buffer
    do
    {
        data->Status = ReadFile(data->hComm, &data->ReadData, sizeof(data->ReadData), &data->NoBytesRead, NULL);
        data->SerialBuffer[data->loop] = data->ReadData;
        ++data->loop;
    } while (data->NoBytesRead > 0);
    --data->loop; //Get Actual length of received data
    //printf_s("\nNumber of bytes received = %d\n\n", data->loop);
    if (data->loop != sizeof(counter_rsp)) return 1;

    return 0;
}

static int write_data(data* data)
{
    counter_req send;
    send.header.msgno = COUNTER_REQ;
    send.header.len = sizeof(send);
    DWORD dNoOfBytesWritten = 0;

    void* data_void = static_cast<void*>(&send);
    char* data_char = static_cast<char*>(data_void);
	
    //Writing data to Serial Port
    data->Status = WriteFile(data->hComm,// Handle to the Serialport
        data_char,            // Data to be written to the port
        sizeof(send),   // No of bytes to write into the port
        &data->BytesWritten,  // No of bytes written to the port
        NULL);
    if (data->Status == FALSE)
    {
        printf_s("\nFail to Written");
        return 1;
//        status = 1;
//        goto Exit;
    }
    //print numbers of byte written to the serial port
    //printf_s("\nNumber of bytes written to the serail port = %d\n\n", data->BytesWritten);

    return 0;
	

}

int main(void)
{
    int status;
    data mydata;
	
	status = init(&mydata, "\\\\.\\COM3");
	
    if (status > 0) goto Exit;

    for(int i = 0; i < 10 ; i++)
    {
        status = write_data(&mydata);
        if (status) {
            std::cout << "write failed..\n" << std::endl;
            goto Exit;
        }

        status = read_data(&mydata);
        if (status) {
            std::cout << "read failed..\n" << std::endl;
            goto Exit;
        }

        void* olle = static_cast<void*>(mydata.SerialBuffer);
        counter_rsp* resp = static_cast<counter_rsp*>(olle);
        std::cout << "value: " << resp->value << std::endl;

        Sleep(10);
    }
	
Exit:
    deinit(status, &mydata);
    return 0;
}

