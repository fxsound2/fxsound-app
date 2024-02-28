// fxdevcon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "..\DfxInstall\DfxInstall.h"

int wmain(int argc, wchar_t *argv[])
{
    if (argc < 2)
    {
        std::cout << "Please specify a paramter : install or remove\r\n";
        return 0;
    }

    if (_wcsicmp(argv[1], L"install") == 0)
    {
        if (argc < 3)
        {
            std::cout << "Syntax: fxdevcon install <inf path>\r\n";
            return 0;
        }

        if (cmdInstall(NULL, NULL, 0, argv[2], L"Root\\FXVAD") == EXIT_OK)
        {
            std::cout << "Success\r\n";
            return 0;
        }
        else
        {
            std::cout << "FxSound driver installation failed\r\n";
            return -1;
        }
    }

    const wchar_t* fxvad_id = L"Root\\FXVAD";
    if (_wcsicmp(argv[1], L"remove") == 0)
    {
        if (cmdRemove(NULL, NULL, argc < 3 ? fxvad_id : argv[2]) == EXIT_OK)
        {
            std::cout << "Success\r\n";
            return 0;
        }
        else
        {
            std::cout << "Driver removal failed\r\n";
            return -1;
        }
    }

    return 0;
}
