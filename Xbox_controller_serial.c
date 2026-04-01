#include <windows.h>
#include <Xinput.h>
#include <stdio.h>

#pragma comment(lib, "XInput.lib")

HANDLE serial;

void send_packet(unsigned char *data, int size)
{
    DWORD bytes;
    if(!WriteFile(serial, data, size, &bytes, NULL))
    {
        printf("Error sending data\n");
    }
}

int main()
{

    serial = CreateFile(
        "COM8",             // Correspond to the port that the UART adapter is connected
        GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if(serial == INVALID_HANDLE_VALUE)
    {
        printf("Error opening COM\n");
        DWORD err = GetLastError();
        printf("Error opening port: %lu\n", err);
        return 1;
    }

    // Configure serial port
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);

    if(!GetCommState(serial, &dcb))
    {
        printf("Error GetCommState\n");
        return 1;
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if(!SetCommState(serial, &dcb))
    {
        printf("Error SetCommState\n");
        return 1;
    }

    // Clean buffers
    PurgeComm(serial, PURGE_TXCLEAR | PURGE_RXCLEAR);

    XINPUT_STATE state;

    while(1)
    {

        if(XInputGetState(0, &state) == ERROR_SUCCESS)
        {

            unsigned char packet[8];

            packet[0] = 0xAA;   // To synchronize the sending of data as first byte
            packet[1] = 0;

            // Each button pressed will correspond to one bit of the first byte
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_A) packet[1] |= 1<<0;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_B) packet[1] |= 1<<1;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_X) packet[1] |= 1<<2;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) packet[1] |= 1<<3;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) packet[1] |= 1<<4;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) packet[1] |= 1<<5;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_START) packet[1] |= 1<<6;
            if(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) packet[1] |= 1<<7;

            // Analog triggers have values between 0 to 255 (1 byte)
            packet[2] = state.Gamepad.bLeftTrigger;
            packet[3] = state.Gamepad.bRightTrigger;

            // Analog joysticks have postive and negative part, so they will be send in 2 bytes each one
            short LX = state.Gamepad.sThumbLX;
            short LY = state.Gamepad.sThumbLY;

            packet[4] = LX & 0xFF;
            packet[5] = (LX >> 8) & 0xFF;

            packet[6] = LY & 0xFF;
            packet[7] = (LY >> 8) & 0xFF;
            
            send_packet(packet,8);

            printf("LX: %d, LY: %d  botones:%d LT:%d RT:%d\n",
                   LX, LY, packet[1], packet[2], packet[3]);

            for (int i=1; i<4; i++){
                printf("%d ",packet[i]);
            }
            printf("\n");
            
        }

        Sleep(5);
    }

    return 0;
}