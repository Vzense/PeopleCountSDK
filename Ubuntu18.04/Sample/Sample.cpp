#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <stdio.h>
#include "VzPeopleCountAPI.h"

using namespace std;
using namespace cv;

void ShowMenu(void);
void DrawPersonInfo2Img(const Mat& img, const VzPeopleInfo& peopleInfo);

enum DeviceState
{
	HotPlugIn = -2,
	HotPlugOut = -1,
	None = 0,
	Opened = 1,
	Upgrading = 2,
	Upgraded = 3
};

class Sensor
{
private:
	DeviceState m_DeviceState = None;

public:
	static void HotPlugStateCallback(void *pUserData, int state)
	{
		Sensor *p = (Sensor *)pUserData;
		if (p)
		{
			p->HandleHotplugCallback(state);
		}
	}

	static void UpgradeStateCallback(void *pUserData, int status, int params)
	{
		Sensor *p = (Sensor *)pUserData;
		if (p)
		{
			p->HandleUpgradeStateCallback(status, params);
		}
	}

	void registCallback()
	{
		Vz_PCRegDeviceHotplugStateCallbackFunc(HotPlugStateCallback, this);
	}

	void HandleHotplugCallback(int state)
	{
		if (Upgraded == m_DeviceState)
		{
			return;
		}

		//hot plug out
		if (VZDEVICE_HotPlugIN != state)
		{
			m_DeviceState = HotPlugOut;
		}
		else //hot plug in
		{
			m_DeviceState = HotPlugIn;
		}

		cout << "state: " << state << " m_DeviceState:" << m_DeviceState << endl;
	}

	void HandleUpgradeStateCallback(int status, int params)
	{
		if (-1 == params)
		{
			cout << "status:" << status << ", upgrade failed,wait for the device to reboot" << endl;
			m_DeviceState = None;
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
				m_DeviceState = Upgraded;
			}
			break;
			default:
				cout << "StatusCallback: other stage:" << status << endl;
				m_DeviceState = None;
				break;
			}
		}
	}

	void SetDeviceState(DeviceState state)
	{
		m_DeviceState = state;
	}

	DeviceState GetDeviceState()
	{
		return m_DeviceState;
	}

	bool InitDevice(VzDeviceHandler* pDeviceHandler)
	{
		VzReturnStatus status = Vz_PCOpenDevice(pDeviceHandler);
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
		Vz_PCRegUpgradeStateCallbackFunc(*pDeviceHandler, UpgradeStateCallback, this);

		ShowMenu();
		m_DeviceState = Opened;

		return true;
	}
};

bool g_isRunning = true;
bool g_showPeopleInfo = false;
VzDeviceHandler g_deviceHandle = 0;
bool g_bopenDoor = false;
bool g_blowPower = false;
bool g_bShowImg = true;

int main(int argc, char *argv[])
{
	Vz_PCInitialize();
	cout << "Vz_Initialize" << endl;

	Vz_PCSetShowImg(g_bShowImg);

	Sensor s;
	s.registCallback();

OPEN:
	g_isRunning = s.InitDevice(&g_deviceHandle);
	if (false == g_isRunning)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		goto OPEN;
	}

	VzPeopleInfoCount peopleInfoCount = {0};
	cv::Mat imageMat;
	while (g_isRunning)
	{

		if (HotPlugOut == s.GetDeviceState())
		{
			Vz_PCCloseDevice(&g_deviceHandle);
			s.SetDeviceState(None);
		}
		else if(Upgraded == s.GetDeviceState())
		{
			g_isRunning = false;
			break;
		}
		else if (Opened == s.GetDeviceState())
		{
			peopleInfoCount = {0};

			VzReturnStatus result = Vz_PCGetPeopleInfoCount(g_deviceHandle, &peopleInfoCount);
			if (VzReturnStatus::VzRetOK == result 
				&& 0 != peopleInfoCount.frame.pFrameData)
			{
				imageMat = cv::Mat(peopleInfoCount.frame.height, peopleInfoCount.frame.width, CV_8UC1, peopleInfoCount.frame.pFrameData);
				for (int i = 0; i < peopleInfoCount.validPeopleCount; i++)
				{
					DrawPersonInfo2Img(imageMat, peopleInfoCount.peopleInfo[i]);
				}
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
		else if (HotPlugIn == s.GetDeviceState())
		{
			s.InitDevice(&g_deviceHandle);
		}

		unsigned char key = waitKey(1);
		switch (key)
		{
		case 'U':
		case 'u':
		{
			//Test camera firmware upgrade
			//[Warnning]:Only Firmware_20201110_nand_b03 and later firmware support the upgrade. Older firmware versions do not support the upgrade.
			char path[256] = "./Firmware_20210116_nand_B12_15.img";
			cout << path << endl;
			if (VzReturnStatus::VzRetOK == Vz_PCStartUpgradeFirmWare(g_deviceHandle, path))
			{
				s.SetDeviceState(Upgrading);
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

void DrawPersonInfo2Img(const Mat& img, const VzPeopleInfo& peopleInfo)
{
	if (false == img.empty())
	{
		Point headpoint = Point(peopleInfo.headPostion[0], peopleInfo.headPostion[1]);

		int tmpx = headpoint.x - 25;
		int tmpy = headpoint.y - 25;
		tmpx = (tmpx < 0) ? 0 : tmpx;
		tmpy = (tmpy < 0) ? 0 : tmpy;
		Point LeftPoint = Point(tmpx, tmpy);

		tmpx = headpoint.x + 25;
		tmpy = headpoint.y + 25;
		tmpx = (tmpx > img.cols) ? (img.cols) : tmpx;
		tmpy = (tmpy > img.rows) ? (img.rows) : tmpy;
		Point RightPoint = Point(tmpx, tmpy);

		cv::rectangle(img, LeftPoint, RightPoint, Scalar(0, 0, 0), 2);
		cv::circle(img, headpoint, 11, Scalar(0, 0, 255), -1, 8);

		Point string_show_point1 = Point(headpoint.x + 25, headpoint.y - 30);
		Point string_show_point2 = Point(headpoint.x + 25, headpoint.y - 10);
		Point string_show_point3 = Point(headpoint.x + 25, headpoint.y +10 );

		static const int BUFLEN = 50;
		char temp[BUFLEN] = { 0 };
		snprintf(temp, BUFLEN, "id: %X ", peopleInfo.id);
		cv::putText(img, temp,
			string_show_point1,
			cv::FONT_HERSHEY_SIMPLEX,
			0.6,
			Scalar(0, 0, 0),
			2,
			9);

		snprintf(temp, BUFLEN, "T: %d", peopleInfo.duration_time);
		cv::putText(img, temp,
			string_show_point2,
			cv::FONT_HERSHEY_SIMPLEX,
			0.6,
			Scalar(0, 0, 0),
			2,
			9);

		snprintf(temp, BUFLEN, "D: %d ", peopleInfo.distance);
		cv::putText(img, temp,
			string_show_point3,
			cv::FONT_HERSHEY_SIMPLEX,
			0.6,
			Scalar(0, 0, 0),
			2,
			9);
	}
}