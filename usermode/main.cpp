#include <iostream>
#include <future>

#include "main.h"
#include "keinterface.h"

int main()
{
	SetConsoleTitle("notepad");
	SetConsoleCtrlHandler(control_handler, TRUE);

	load_driver();

	KeInterface driver("\\\\.\\driver");

	DWORD pid = driver.get_pid();

	DWORD client_address = driver.get_module();

	std::cout << xorstr_("[*]: pid: ") << pid << std::endl;
	std::cout << xorstr_("[*]: client_address: ") << std::uppercase << std::hex << client_address << std::endl;

	DWORD local_player = driver.Read<DWORD>(pid, client_address + 0xCF9A4C, sizeof(ULONG));

	std::cout << xorstr_("[*]: press end to exit") << std::endl << std::endl;

	while (!GetAsyncKeyState(VK_END))
	{
		DWORD glowObjectManager = driver.Read<DWORD>(pid, client_address + 0x524C460, sizeof(ULONG));
		int glowObjectCount = driver.Read<int>(pid, client_address + 0x524C460 + 0x4, sizeof(int));

		for (int i = 0; i < glowObjectCount; i++)
		{
			int currentPlayer = driver.Read<int>(pid, client_address + 0x4D0C004 + (i * 16), sizeof(int));
			int glowIndex = driver.Read<int>(pid, currentPlayer + 0xA40C, sizeof(int));
			int team_num = driver.Read<int>(pid, currentPlayer + 0xF4, sizeof(int));

			int glowAdress = glowObjectManager + (glowIndex * 38);
			float color_white = 1.0f;
			ULONG white = *((unsigned long*)&color_white);
			if (team_num == 2 || team_num == 3)
			{
				driver.Write(pid, glowAdress + 0x4, white, sizeof(float));
				driver.Write(pid, glowAdress + 0x8, white, sizeof(ULONG));
				driver.Write(pid, glowAdress + 0xC, white, sizeof(float));
				driver.Write(pid, glowAdress + 0x10, white, sizeof(float));
				driver.Write(pid, glowAdress + 0x24, true, sizeof(BOOL));
				driver.Write(pid, glowAdress + 0x25, false, sizeof(BOOL));

			}
		}

		DWORD in_ground = driver.Read<DWORD>(pid, local_player + 0x104, sizeof(ULONG));
		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && (in_ground))
		{
			driver.Write(pid, client_address + 0x51AF6B8, 0x5, 8);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			driver.Write(pid, client_address + 0x51AF6B8, 0x4, 8);

		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool load_driver()
{
	std::error_code error;

	driver_name = random_string(6);
	driver_path = std::filesystem::absolute("driver/" + driver_name + ".sys");

	if (!std::filesystem::copy_file(std::filesystem::absolute(o_driver_path), driver_path, error))
	{
		std::cout << xorstr_("[*]: the driver can't be copied: %s\n") << error.message().c_str() << std::endl;
		return false;
	}

	if (!start_service(driver_name, driver_path.string()))
	{
		std::cout << xorstr_("[*]: start service failed..") << std::endl;
		return false;
	}

	std::cout << xorstr_("[*]: driver loaded..") << std::endl;

	return true;
}

void unload_driver()
{
	if (!stop_service(driver_name))
		std::cout << xorstr_("[*]: stop_service failed..") << std::endl;

	std::error_code error;

	if (!std::filesystem::remove(driver_path, error))
		std::cout << xorstr_("[*]: remove failed %s\n") << error.message().c_str() << std::endl;
}

BOOL WINAPI control_handler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		unload_driver();
		break;
	}

	return TRUE;
}
