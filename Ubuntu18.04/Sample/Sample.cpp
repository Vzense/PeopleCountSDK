#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <stdio.h>
#include "VzPeopleCountAPI.h"

using namespace std;
using namespace cv;

bool InitDevice();
void UpgradeStateCallback(int status, int params);
void DeviceStateCallback(int params);
void ShowMenu(void);

enum DeviceState
{
	HotPlugIn = -2,
	HotPlugOut = -1,
	None = 0,
	Opened = 1,
	Upgraded = 2,
};

bool g_isRunning = true;
DeviceState g_deviceState = None;
bool g_showPeopleInfo = false;
VzDeviceHandler g_deviceHandle = 0;
bool g_bopenDoor = false;
bool g_blowPower = false;
bool g_bShowImg = true;

int main(int argc, char *argv[])
{
	Vz_PCInitialize();
	cout << "Vz_Initialize" << endl;

OPEN:
	g_isRunning = InitDevice();
	if (false == g_isRunning)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		goto OPEN;
	}

	VzPeopleInfoCount peopleInfoCount = {0};
	cv::Mat imageMat;
	while (g_isRunning)
	{

		if (HotPlugOut == g_deviceState)
		{
			Vz_PCCloseDevice(&g_deviceHandle);
			g_deviceState = None;
		}
		else if (Opened == g_deviceState)
		{
			peopleInfoCount = {0};

			VzReturnStatus result = Vz_PCGetPeopleInfoCount(g_deviceHandle, &peopleInfoCount);
			if (VzReturnStatus::VzRetOK == result 
				&& 0 != peopleInfoCount.frame.pFrameData)
			{
				imageMat = cv::Mat(peopleInfoCount.frame.height, peopleInfoCount.frame.width, CV_8UC1, peopleInfoCount.frame.pFrameData);
				cv::imshow("ShowImg", imageMat);
			}
			else if ((true == g_bopenDoor && VzReturnStatus::VzRetDoorWasOpend == result) 
					|| (VzReturnStatus::VzRetOK == result) 
					|| (true == g_blowPower))
			{
				//do nothing
			}
			else
			{
				cout << "Vz_PCGetPeopleInfoCount error:" << result << endl;
			}
		}
		else if (HotPlugIn == g_deviceState)
		{
			InitDevice();
		}

		unsigned char key = waitKey(1);
		switch (key)
		{
		case 'U':
		case 'u':
		{
			//Test camera firmware upgrade
			//[Warnning]:Only Firmware_DCAMBOE_20201110_nand_b03 and later firmware support the upgrade. Older firmware versions do not support the upgrade.
			Vz_PCRegUpgradeStateCallbackFunc(g_deviceHandle, UpgradeStateCallback);
			char path[256] = "./Firmware_CSI100_20210116_nand_B12_15.img";
			cout << path << endl;
			if (VzReturnStatus::VzRetOK == Vz_PCStartUpgradeFirmWare(g_deviceHandle, path))
			{
				g_deviceState = Upgraded;
				cout << "start upgrade ok" << endl;
			}
			else
			{
				cout << "start upgrade ng" << endl;
			}
		}
		break;
		case 'S':
		case 's':
		{
			//Turn image display on and off
			g_bShowImg = !g_bShowImg;
			Vz_PCSetShowImg(g_bShowImg);
			cout << ((true == g_bShowImg) ? "start" : "stop") << "show img." << endl;
		}
		break;
		case 'L':
		case 'l':
		{
			//Entering and exiting low-power mode
			g_blowPower = !g_blowPower;
			Vz_PCSetLowpowerModeEnable(g_deviceHandle, g_blowPower);
			cout << ((true == g_blowPower) ? "enter" : "exit") << " low power." << endl;
		}
		break;
		case 'H':
		case 'h':
		{
			//Setting the mounting height of the camera
			cout << "Input camera height:";
			uint16_t cameraHeight = 2000;
			cin >> cameraHeight;
			VzReturnStatus result = Vz_PCSetCameraHeight(cameraHeight);
			cout << "SetCameraHeight: " << cameraHeight << (VzReturnStatus::VzRetOK == result ? " OK, and it takes effect the next time the application is started." : " NG") << endl;
		}
		break;
		case 'O':
		case 'o':
		{
			//Setting the opening and closing state of the refrigerator door
			g_bopenDoor = !g_bopenDoor;
			Vz_PCSetDoorOpenState(g_deviceHandle, g_bopenDoor);
			cout << ((true == g_bopenDoor) ? "open" : "close") << " the door." << endl;
		}
		break;
		case 'D':
		case 'd':
		{
			//Set dwell time threshold
			cout << "Input dwell time:";
			uint16_t threshold = 3;
			cin >> threshold;
			VzReturnStatus result = Vz_PCSetDwelltimeThreshold(threshold);
			cout << "SetDwelltime:" << (VzReturnStatus::VzRetOK == result ? "OK" : "NG") << endl;
		}
		break;
		case 'F':
		case 'f':
		{
			//Set the furthest detection distance
			cout << "Input the furthest detection distance:";
			int furthest = 3999;
			cin >> furthest;
			VzReturnStatus result = Vz_PCSetMaxDetectDistance(furthest);
			cout << "SetMaxDetectDistance:" << (VzReturnStatus::VzRetOK == result ? "OK, and it takes effect the next time the application is started." : "NG") << endl;
		}
		break;
		case 'P':
		case 'p':
		{
			//Save image once for debug
			char fileName[50] = {0};
			static uint16_t index = 0;
			sprintf(fileName, "%d.gray", index);
			FILE *fprb = fopen(fileName, "wb");
			if (fprb != 0) //> fname is path of file by const char* type
			{
				fwrite(imageMat.data, imageMat.elemSize() * imageMat.rows * imageMat.cols, 1, fprb);
				fclose(fprb);
				index++;
			}
		}
		break;
		case 'M':
		case 'm':
		{
			//Show menu
			ShowMenu();
		}
		break;
		case 27: //ESC
			g_isRunning = false;
			break;
		default:
			break;
		}
	}

	cv::destroyAllWindows();

	Vz_PCCloseDevice(&g_deviceHandle);
	cout << "Vz_PCCloseDevice" << endl;
	Vz_PCShutdown();
	cout << "Vz_Shutdown" << endl;
	return 0;
}

void ShowMenu(void)
{
	cout << "Press following key to set corresponding feature:" << endl;
	cout << "U/u: Test camera firmware upgrade" << endl;
	cout << "S/s: Turn image display on and off" << endl;
	cout << "L/l: Entering and exiting low-power mode" << endl;
	cout << "F/f: Setting the furthest detection distance" << endl;
	cout << "H/h: Setting the mounting height of the camera" << endl;
	cout << "O/o: Setting the opening and closing state of the refrigerator door" << endl;
	cout << "D/d: Set dwell time threshold" << endl;
	cout << "P/p: Save image once" << endl;
	cout << "M/m: Show menu" << endl;
	return;
}

void UpgradeStateCallback(int status, int params)
{
	if (-1 == params)
	{
		cout << "status:" << status << ", upgrade failed,wait for the device to reboot" << endl;
		g_deviceState = None;
	}
	else
	{
		switch (status)
		{
		case VZDEVICE_UPGRADE_IMG_COPY:
		{
			cout << "StatusCallback: DEVICE_PRE_UPGRADE_IMG_COPY status:" << (-1 == params ? "NG" : "OK") << endl;
		}
		break;
		case VZDEVICE_UPGRADE_IMG_CHECK_DOING:
		{
			cout << "StatusCallback: DEVICE_UPGRADE_IMG_CHECK_DOING status:" << params << endl;
		}
		break;
		case VZDEVICE_UPGRADE_IMG_CHECK_DONE:
		{
			cout << "StatusCallback: DEVICE_PRE_UPGRADE_IMG_COPY status:" << (-1 == params ? "NG" : "OK") << endl;
		}
		break;
		case VZDEVICE_UPGRADE_DOING:
		{
			cout << "StatusCallback: DEVICE_UPGRADE_UPGRAD_DOING percent:" << params << "%" << endl;
		}
		break;
		case VZDEVICE_UPGRADE_RECHECK_DOING:
		{
			cout << "StatusCallback: DEVICE_UPGRADE_RECHECK_DOING" << (-1 == params ? "NG" : "OK") << endl;
		}
		break;
		case VZDEVICE_UPGRADE_RECHECK_DONE:
		{
			cout << "StatusCallback: DEVICE_UPGRADE_RECHECK_DONE:" << (-1 == params ? "NG" : "OK") << endl;
		}
		break;
		case VZDEVICE_UPGRADE_DONE:
		{
			cout << "StatusCallback: DEVICE_UPGRADE_UPGRAD_DONE:" << (-1 == params ? "NG" : "OK") << ",wait for the device to reboot" << endl;
			g_isRunning = false;
		}
		break;
		default:
			cout << "StatusCallback: other stage:" << status << endl;
			g_deviceState = None;
			break;
		}
	}
}

void DeviceStateCallback(int status)
{
	if (Upgraded == g_deviceState)
	{
		return;
	}

	//hot plug out
	if (VZDEVICE_HotPlugIN != status)
	{
		g_deviceState = HotPlugOut;
	}
	else //hot plug in
	{
		g_deviceState = HotPlugIn;
	}

	cout << "status: " << status << " g_deviceState:" << g_deviceState << endl;
}

bool InitDevice()
{
	VzReturnStatus status = Vz_PCOpenDevice(&g_deviceHandle);
	if (status != VzReturnStatus::VzRetOK)
	{
		if (VzReturnStatus::VzRetNoDeviceConnected == status)
		{
			cout << "Please connect the device first!" << endl;
		}
		else
		{
			cout << "Vz_PCOpenDevice failed: " << status << endl;
		}
		return false;
	}

	ShowMenu();
	g_deviceState = Opened;
	Vz_PCRegDeviceHotplugStateCallbackFunc(DeviceStateCallback);
	Vz_PCSetShowImg(g_bShowImg);

	return true;
}