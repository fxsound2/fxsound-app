/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

    const wchar_t* fxvad_id = L"Root\\FXVAD";

    if (_wcsicmp(argv[1], L"install") == 0)
    {
        if (argc < 3)
        {
            std::cout << "Syntax: fxdevcon install <inf path>\r\n";
            return 0;
        }

        if (cmdInstall(NULL, NULL, 0, argv[2], argc < 4 ? fxvad_id : argv[3]) == EXIT_OK)
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
