#ifndef VTYPES_H
#define VTYPES_H

#define VZ_EXPORT_ON

#ifdef VZ_EXPORT_ON
#ifdef _WIN32
#define VZENSE_API_EXPORT __declspec(dllexport)
#else
#define VZENSE_API_EXPORT __attribute__((visibility("default")))
#endif
#else
#ifdef _WIN32
#define VZENSE_API_EXPORT __declspec(dllimport)
#else
#define VZENSE_API_EXPORT __attribute__((visibility("default")))
#endif
#endif

#ifdef __cplusplus
#define VZENSE_C_API_EXPORT extern "C" VZENSE_API_EXPORT
#else
#define VZENSE_C_API_EXPORT VZENSE_API_EXPORT
#endif

#include <stdint.h>

/**
 * @brief Return status codes for all APIs.\n 
 * 		  <code>VzRetOK = 0</code> means the API successfully completed its operation.\n 
 * 		  All other codes indicate a device, parameter, or API usage error.
 */
typedef enum{
        VzRetOK                         =  0,   //!< The function completed successfully.
        VzRetNoDeviceConnected          = -1,   //!< There is no depth camera connected or the camera has not been connected correctly. Check the hardware connection or try unplugging and re-plugging the USB cable.
        VzRetInvalidDeviceIndex         = -2,   //!< The input device index is invalid.
        VzRetDevicePointerIsNull        = -3,   //!< The device structure pointer is null.
        VzRetInvalidFrameType           = -4,   //!< The input frame type is invalid.
        VzRetFramePointerIsNull         = -5,   //!< The output frame buffer is null.
        VzRetReadNextFrameError         = -11,  //!< An error occurred when capturing the next image frame.
        VzRetInputPointerIsNull         = -12,  //!< An input pointer parameter is null.
        VzRetCameraNotOpened            = -13,  //!< The camera has not been opened.
        VzRetInvalidParams              = -15,  //!< One or more of the parameter values provided are invalid.
        VzRetCurrentVersionNotSupport   = -16,  //!< This feature is not supported in the current version.
        VzRetUpgradeImgError            = -17,  //!< There is an error in the upgrade file.
        VzRetUpgradeImgPathTooLong      = -18,  //!< Upgrade file path length greater than 260.
	VzRetUpgradeCallbackNotSet	= -19,  //!< Ps2_SetUpgradeStatusCallback is not called.
	VzRetNoAdapterConnected		= -100,	//!< There is no adapter connected.
        VzRetDoorWasOpend		= -101,	//!< The door has been opened.


	VzRetOthers = -255,	             //!< An unknown error occurred.
}VzReturnStatus;

typedef enum
{
        VZDEVICE_NORMAL = 0x00,                   //!< The device is in normal working condition.
        VZDEVICE_UPGRADE_BEGIN = 0x01,            //!< The device is in the upgrade state and ready to upgrade.
        VZDEVICE_UPGRADE_IMG_COPY = 0x02,         //!< Finish copying firmware files to the device.
        VZDEVICE_UPGRADE_IMG_CHECK_DOING = 0x03,  //!< The device is checking the integrity of the firmware file.
        VZDEVICE_UPGRADE_IMG_CHECK_DONE = 0x04,   //!< The device completes the firmware file integrity check.
        VZDEVICE_UPGRADE_DOING = 0x05,            //!< The device is burning firmware file.
        VZDEVICE_UPGRADE_RECHECK_DOING = 0x06,    //!< The device is being rechecked.
        VZDEVICE_UPGRADE_RECHECK_DONE = 0x07,     //!< The device upgrade review completed.
        VZDEVICE_UPGRADE_DONE = 0x08,             //!< The device upgrade completed.
}VzDeviceStatus;

typedef enum
{
        VZDEVICE_HotPlugIN = 0x00,                //!< hot plug in 
        VZDEVICE_HotPlugOUT = 0x01,               //!< hot plug out 
}VzHotPlugState;

struct Device;
typedef Device* VzDeviceHandler;

typedef struct
{
	uint8_t*       pFrameData;    //!< The pointer which points to the image buffer.
	uint32_t       dataLen;       //!< The length of pFrameData, in bytes.
	uint16_t       width;		  //!< The width of the frame, in pixels.
	uint16_t       height;        //!< The height of the frame, in pixels.
}VzPCFrame;

typedef struct
{
	uint16_t headPostion[2];        //Pixel coordinates of the center point of the head of the person identified from the image
	uint16_t distance;		        //The distance of the person identified from the image from the device(mm)
	uint16_t dwell_time;            //The time a person identified from the image dwells in front of the device(second)
} VzPeopleInfo;

typedef struct
{
	VzPeopleInfo        peopleInfo[20];         //Information(Up to 20) about the person being identified from the im-age.Reference VzPeopleInfo.
	uint16_t            validPeopleCount;       //The count of people identified from the current image.
    uint16_t            dwellPeopleCount;       //The count of people who stayed longer than the dwell time threshold.
    VzPCFrame           frame;                  //The Image information is used for debugging. Available only if Vz_PCSetShowImg is set to true, otherwise empty.
} VzPeopleInfoCount;


/**
* @brief device firmware upgrade state callback function
* @param[In]    status: device state. Pass in the applicable value defined by ::VzDeviceStatus
* @param[In]    params: result of the execution of the current state
*/
typedef void(*PtrUpgradeStateCallback)(int status, int params);

/**
* @brief hotplug status callback function
* uri    return the uri of the Device, See ::PsDeviceInfo
* state  0:device added , 1:device removed
*/
typedef void(*PtrDeviceHotplugStateCallback)(int state);


#endif //VTYPES_H