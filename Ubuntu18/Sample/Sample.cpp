#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <unistd.h>
#include <stdio.h>
#include "VzPeopleCountAPI.h"

using namespace std;
using namespace cv;

bool InitDevice();
void UpgradeStateCallback(int status, int params);
void DeviceStateCallback(int params);
void ShowMenu(void);


bool g_isRunning = true;
int32_t g_deviceState = 0; //-2:plug in; -1:plug out 0:none; 1:open; 2:upgrade
bool g_showPeopleInfo = false;
VzDeviceHandler g_deviceHandle = 0;

int main(int argc, char *argv[])
{
	Vz_PCInitialize();
	cout<<"Vz_Initialize"<<endl;

	g_isRunning = InitDevice();

	ShowMenu();
	VzPeopleInfoCount peopleInfoCount = {0};
	cv::Mat imageMat ;
	while(g_isRunning)
	{
		
		if( -1 == g_deviceState)
		{
			Vz_PCCloseDevice(&g_deviceHandle);
			g_deviceState = 0;
		}
		else if(1 == g_deviceState)
		{
			peopleInfoCount = {0};

			VzReturnStatus result = Vz_PCGetPeopleInfoCount(g_deviceHandle, &peopleInfoCount);
			if(VzReturnStatus::VzRetOK == result
				&& 0 != peopleInfoCount.frame.pFrameData)
			{
				imageMat = cv::Mat(peopleInfoCount.frame.height,  peopleInfoCount.frame.width,  CV_8UC1,  peopleInfoCount.frame.pFrameData);
					cv::imshow("ShowImg", imageMat);
			}
			else
			{
					cout << "Vz_PCGetPeopleInfoCount error:" << result<<endl;
			}
		}
		else if(-2 == g_deviceState)
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
			cout << "Input Firmware path:" ;
            char path[256];
            cin.getline(path,256) ;
            cout<<path<<endl;
			if(VzReturnStatus::VzRetOK == Vz_PCStartUpgradeFirmWare(g_deviceHandle, path))
			{
				g_deviceState = 2;
				cout<< "start upgrade ok"<<endl;
			}
			else
			{
				cout<< "start upgrade ng"<<endl;
			}
		}
		break;
		case 'S':
		case 's':
		{
			//Turn image display on and off
			static bool bShowImg = false;
			Vz_PCSetShowImg(bShowImg);
			bShowImg = !bShowImg;
		}
		break;
		case 'L':
		case 'l':
		{
			//Entering and exiting low-power mode
			static bool blowPower = true;
			Vz_PCSetLowpowerModeEnable(g_deviceHandle, blowPower);
			blowPower = !blowPower;
		}
		break;
		case 'H':
		case 'h':
		{
			//Setting the mounting height of the camera
			cout << "Input camera height:" ;
            uint16_t  cameraHeight = 1900;
            cin >> cameraHeight;
            cout<<"camera height:"<<cameraHeight<<endl;
			cout<<"SetCameraHeight: "<< (VzReturnStatus::VzRetOK == Vz_PCSetCameraHeight(cameraHeight) ? "OK":"NG");
		}
		break;
		case 'O':
		case 'o':
		{	
			//Setting the opening and closing state of the refrigerator door
			static bool bopenDoor = true;
			Vz_PCSetDoorOpenState(bopenDoor);
			bopenDoor = !bopenDoor;
		}
		break;
		case 'D':
		case 'd':
		{
			//Set dwell time threshold
			cout << "Input dwell time:";
            uint16_t  threshold = 3;
            cin >> threshold;
            cout<<"dwell time:"<<threshold<<endl;
			cout<<"SetDwelltime:"<< (VzReturnStatus::VzRetOK == Vz_PCSetDwelltimeThreshold(threshold) ? "OK":"NG");
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
			if (fprb != 0 )  //> fname is path of file by const char* type
			{
				fwrite(imageMat.data, imageMat.elemSize()*imageMat.rows*imageMat.cols, 1, fprb);
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
		case 27://ESC
			g_isRunning = false;
			break;
		default:
			break;
		}
	}

	cv::destroyAllWindows();

	Vz_PCCloseDevice(&g_deviceHandle);
	cout<<"Vz_PCCloseDevice"<<endl;
	Vz_PCShutdown();
	cout<<"Vz_Shutdown"<<endl;
	return 0;
}

void ShowMenu(void)
{
	cout<<"Press following key to set corresponding feature:"<<endl;
	cout<<"U/u: Test camera firmware upgrade"<<endl;
	cout<<"S/s: Turn image display on and off"<<endl;
	cout<<"L/l: Entering and exiting low-power mode"<<endl;
	cout<<"H/h: Setting the mounting height of the camera"<<endl;
	cout<<"O/o: Setting the opening and closing state of the refrigerator door"<<endl;
	cout<<"D/d: Set dwell time threshold"<<endl;
	cout<<"P/p: Save image once"<<endl;
	cout<<"M/m: Show menu"<<endl;
	return;
}

void UpgradeStateCallback(int status, int params)
{
    if (-1 == params){
        cout<<"status:"<<status<<", upgrade failed,wait for the device to reboot"<<endl;
		g_deviceState = 0;
    }else{
        switch (status)
        {
        case VZDEVICE_UPGRADE_IMG_COPY:
        {
            cout<<"StatusCallback: DEVICE_PRE_UPGRADE_IMG_COPY status:"<< (-1 == params ? "NG" : "OK")<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_IMG_CHECK_DOING:
        {
            cout<<"StatusCallback: DEVICE_UPGRADE_IMG_CHECK_DOING status:"<<params<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_IMG_CHECK_DONE:
        {
            cout<<"StatusCallback: DEVICE_PRE_UPGRADE_IMG_COPY status:" << (-1 == params ? "NG" : "OK")<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_DOING:
        {
            cout<<"StatusCallback: DEVICE_UPGRADE_UPGRAD_DOING percent:"<<params<<"%"<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_RECHECK_DOING:
        {
            cout<<"StatusCallback: DEVICE_UPGRADE_RECHECK_DOING"<<(-1 == params ? "NG" : "OK")<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_RECHECK_DONE:
        {
            cout<<"StatusCallback: DEVICE_UPGRADE_RECHECK_DONE:"<<(-1 == params ? "NG" : "OK")<<endl;
        }
            break;
        case VZDEVICE_UPGRADE_DONE:
        {
            cout<<"StatusCallback: DEVICE_UPGRADE_UPGRAD_DONE:"<<(-1 == params ? "NG" : "OK")<<",wait for the device to reboot"<<endl;
			g_isRunning = false;
        }
            break;
        default:
            cout<<"StatusCallback: other stage:"<<status<<endl;
			g_deviceState = 0;
            break;
        }
    }
}


void DeviceStateCallback(int status)
{ 
	if(2 == g_deviceState)
	{
		return;
	}
	if(0 != status)
	{
		g_deviceState = -1;
	}
	else
	{
		g_deviceState = -2;
	}
}

bool InitDevice()
{
	VzReturnStatus status = Vz_PCOpenDevice(&g_deviceHandle);
	if (status != VzReturnStatus::VzRetOK)
	{
		cout << "Vz_PCOpenDevice failed: " << status << endl;
		return false;
	}
	g_deviceState = 1;
	Vz_PCRegDeviceHotplugStateCallbackFunc(DeviceStateCallback);
	Vz_PCSetShowImg(true);

	return true;
}