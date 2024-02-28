// DfxInstallTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include "..\DfxInstall\DfxInstall.h"

int wmain(int argc, wchar_t *argv[])
{
	if (argc < 2)
	{
		return 0;
	}

	DfxInstall dfx_install(argv[1], L"12.0.0.0");

	std::string log;    

	dfx_install.CreateUpdateTask(log);
	std::cout << log << std::endl;

	dfx_install.UninstallDFXDriver(log);
	std::cout << log << std::endl;

    dfx_install.UninstallFxSoundDriver(log);
    std::cout << log << std::endl;

	std::cin.get();

	dfx_install.InstallDFXDriver(log);
	std::cout << log << std::endl;

	std::cin.get();

	dfx_install.UninstallFxSoundDriver(log);
	std::cout << log << std::endl;

	dfx_install.DeleteUpdateTask(log);
	std::cout << log << std::endl;
}