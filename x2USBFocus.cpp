
/*
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "x2USBFocus.h"
#include <queue>

#include <fcntl.h> 
#include <sys/stat.h> 

#include <sys/types.h> 

#include <unistd.h> 
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <linux/usbdevice_fs.h>

#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/serialportparams2interface.h"

#define X2_FOC_NAM "USB Focus"
#define RESP_TIMEOUT_MS 2000
#define LOCK_IO_PORT X2MutexLocker Lock(m_pIOMutex);
#define DEF_PORT_NAME					"/dev/FTDIusb"


#define TCFSSLEEP(ms)  do { if (m_pSleeper) m_pSleeper->sleep(ms); } while (false)
#define UNKNOWNPOSITION -1

X2USBFocus::X2USBFocus(const char* pszDisplayName,
                     const int& nInstanceIndex,
                     SerXInterface						* pSerXIn,
                     TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
                     SleeperInterface					* pSleeperIn,
                     BasicIniUtilInterface				* pIniUtilIn,
                     LoggerInterface						* pLoggerIn,
                     MutexInterface						* pIOMutexIn,
                     TickCountInterface					* pTickCountIn)

{
   m_nInstanceIndex				= nInstanceIndex;
   m_pSerX							= pSerXIn;
   m_pTheSkyXForMounts			= pTheSkyXIn;
   m_pSleeper						= pSleeperIn;
   m_pIniUtil						= pIniUtilIn;
   m_pLogger						= pLoggerIn;
   m_pIOMutex						= pIOMutexIn;
   m_pTickCount					= pTickCountIn;
   
   m_bLinked = false;
}

X2USBFocus::~X2USBFocus()
{
   if (GetLogger())
      GetLogger()->out((char *)"~X2USBFocus");

   //Delete objects used through composition
   if (GetSerX())
      delete GetSerX();
   if (GetTheSkyXFacadeForDrivers())
      delete GetTheSkyXFacadeForDrivers();
   if (GetSleeper())
      delete GetSleeper();
   if (GetSimpleIniUtil())
      delete GetSimpleIniUtil();
   if (GetLogger())
      delete GetLogger();
   if (GetMutex())
      delete GetMutex();
}

int	X2USBFocus::queryAbstraction(const char* pszName, void** ppVal)
{
   *ppVal = NULL;
   
   if (!strcmp(pszName, FocuserGotoInterface2_Name))
      *ppVal = dynamic_cast<FocuserGotoInterface2*>(this);
   else if (!strcmp(pszName, FocuserTemperatureInterface_Name))
      *ppVal = dynamic_cast<FocuserTemperatureInterface*>(this);
   else if (!strcmp(pszName, SerialPortParams2Interface_Name))
      *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
   else 	if (!strcmp(pszName, LinkInterface_Name))
      *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
   else 	if (!strcmp(pszName, LoggerInterface_Name))
      *ppVal = GetLogger();
   else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
      *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
   
   return SB_OK;
}

void X2USBFocus::driverInfoDetailedInfo(BasicStringInterface& str) const
{
}
double X2USBFocus::driverInfoVersion(void) const
{
   return 1.0;
}

void X2USBFocus::deviceInfoNameShort(BasicStringInterface& str) const
{
   str = X2_FOC_NAM;
}
void X2USBFocus::deviceInfoNameLong(BasicStringInterface& str) const
{
   str = X2_FOC_NAM;
}
void X2USBFocus::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
   str = X2_FOC_NAM;
}
void X2USBFocus::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
   str = "1505";
}

void X2USBFocus::deviceInfoModel(BasicStringInterface& str)
{
   str = "V3.0";
}

int X2USBFocus::establishLink(void)
{
   if (GetLogger()) 
      GetLogger()->out((char *)"X2USBFocus::establishLink called");
   
   m_bDoingGoto = false;
   
   if (GetSerX()) {
      char szPortName [DRIVER_MAX_STRING];
      portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

      if (GetLogger()) {
         GetLogger()->out((char *)szPortName);
      }
      
      this->resetDevice((char const *)szPortName);

      if (!GetSerX()->isConnected())
      {
         if (GetSerX()->open(szPortName) == SB_OK) {
            
	    m_bLinked = true;
	    if (GetSerX())
      		GetSerX()->purgeTxRx();

            if (GetLogger())
               GetLogger()->out((char *)"usbfocus connected");
            
            this->applySettings();
            return SB_OK;
         } else {
            if (GetLogger())
               GetLogger()->out((char *)"Failed to open link with focuser");
         }
      } 
   }
   return ERR_NOINTERFACE;
}
   
int X2USBFocus::terminateLink(void)
{
   if (GetLogger())
      GetLogger()->out((char *)"Will terminate link");
   
   focAbort();
   
   if (GetSerX()) {
      
      if (GetSerX()->isConnected())
         GetSerX()->close();
   }
   
   // For some unidentified reason, the USB device refuses to be opened twice in a row. Either the user must unplug / replug it or we use
   // this reset trick to reset it when done with it. this seems to allow the device to work properly more that once...
   char szPortName [DRIVER_MAX_STRING];
   portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);
   this->resetDevice((char const *)szPortName);
   
   m_bLinked = false;
   
   if (GetLogger())
      GetLogger()->out((char *)"Link terminated");
   
   return SB_OK;
}

bool X2USBFocus::isLinked(void) const
{
   return m_bLinked;
}

int X2USBFocus::focTemperature(double &dTemperature) {
   LOCK_IO_PORT
   
   int nErr = SB_OK;
   
   unsigned long read = 0;
   char buf[9];
   memset(buf,0, 9);
   int nTimes = 5;
   int nCount = 0;
   bool bDone = false;

   do
   {
      if (nCount) TCFSSLEEP(500);
      
      nErr = sendRequest("FTMPRO");
      if (nErr == SB_OK) {
         GetSerX()->readFile(buf, 9, read);
         if (buf[0] == 'T'  &&
             buf[1] == '=')
         {
            dTemperature = atof(&buf[2]);
            nErr = SB_OK;
            bDone = true;
         }
      }
      ++nCount;
   } while (!bDone && nErr==0 && nCount < nTimes);
   
   if (nCount >= nTimes) {
      nErr = ERR_NORESPONSE;
   }
   
   return nErr;
}

int	X2USBFocus::focPosition(int& nPosition)
{
   LOCK_IO_PORT
   
   int nErr = SB_OK;
   
   unsigned long read = 0;
   char buf[9];
   memset(buf,0, 9);
   int nTimes = 5;
   int nCount = 0;
   bool bDone = false;

   do
   {
      if (nCount) TCFSSLEEP(500);

      nErr = sendRequest("FPOSRO");
      if (nErr == SB_OK) {
         GetSerX()->readFile(buf, 9, read);
         if (buf[0] == 'P'  &&
             buf[1] == '=')
         {
            nPosition = atoi(&buf[2]);
            m_dPosition = nPosition;
            nErr = SB_OK;
            bDone = true;
         }
         
      }
      ++nCount;
   } while (!bDone && nErr==0 && nCount < nTimes);
   
   if (nCount >= nTimes) {
      nErr = ERR_NORESPONSE;
   }
   
   return nErr;
}


int	X2USBFocus::focMinimumLimit(int& nMinLimit)
{
   nMinLimit = 0;
   return SB_OK;
}
int	X2USBFocus::focMaximumLimit(int& nMaxLimit)
{
   this->maxpos(nMaxLimit);
   return SB_OK;
}

int	X2USBFocus::focAbort()
{ 
   if (m_bDoingGoto) {
      m_bDoingGoto = false;
      sendRequest("FQUITx");
   }
   
   return SB_OK;
}

int	X2USBFocus::startFocGoto(const int& nRelativeOffset) {
   int result = SB_OK;
   char goto_str[7];

   if (m_bDoingGoto) {
      return ERR_CMDFAILED;
   }
   
   m_bDoingGoto = true;
   
   m_targetPosition = m_dPosition + nRelativeOffset;
   
   if (nRelativeOffset < 0) {
      sprintf(goto_str, "I%05d", abs(nRelativeOffset));
      result = sendRequest(goto_str);
   } else if (nRelativeOffset > 0) {
      sprintf(goto_str, "O%05d", nRelativeOffset);
      result = sendRequest(goto_str);
   }
   
   return result;
}

int	X2USBFocus::isCompleteFocGoto(bool& bComplete) const {

   if (!m_bDoingGoto)
      bComplete = true;
   else
      bComplete = m_targetPosition == m_dPosition;
   
   return SB_OK;
}

int	X2USBFocus::endFocGoto(void) {
   
   if (!m_bDoingGoto) {
      return ERR_CMDFAILED;
   }
   
   m_bDoingGoto = false;
   
   return SB_OK;
}

int X2USBFocus::amountCountFocGoto(void) const
{
   return 3;
}
int	X2USBFocus::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)
{
   switch (nZeroBasedIndex)
   {
      default:
      case 0: strDisplayName="Small";
         nAmount=10;break;
      case 1: strDisplayName="Medium";
         nAmount=100;break;
      case 2: strDisplayName="Large";
         nAmount=1000;break;
   }
   return SB_OK;
}

int	X2USBFocus::amountIndexFocGoto(void)
{
   return 0;
}

int X2USBFocus::sendRequest(const char * request) {
   int nErr = SB_OK;
   unsigned long wrote = 0;

   LOCK_IO_PORT
   
   
   if (GetSerX())
      GetSerX()->purgeTxRx();
   
   nErr = m_pSerX->writeFile((void *)request, strlen(request), wrote);
   if (nErr == SB_OK) {
      if (GetSerX())
         nErr = GetSerX()->flushTx();
   }
   
   if (GetLogger() && strncmp("FPOSRO", request, 6) != 0  && strncmp("FTMPRO", request, 6) != 0 ) {    // no spam in log
      char tmp[50];
      sprintf(tmp, "sent %s = %d", request, nErr);
      GetLogger()->out((char *)tmp);
   }
   return nErr;
}

int X2USBFocus::sendAndWaitForReply(const char * request, const char * response) {
   int nErr = SB_OK;
   char buf[255];
   unsigned long wrote = 0;
   int nWaitErr;
   unsigned long responseLength = strlen(response);
   
   if (GetLogger())
      GetLogger()->out((char *)"X2USBFocus::sendAndWaitForReply");
   if (GetLogger())
      GetLogger()->out((char *)request);

   LOCK_IO_PORT
   
   if (GetSerX())
      GetSerX()->purgeTxRx();
   
   memset(buf,0,255);
   
   nErr = m_pSerX->writeFile((char *)request, strlen(request), wrote);
   if (nErr == SB_OK) {
      
      if (GetLogger())
         GetLogger()->out((char *)request);
      
      if (GetSerX())
         GetSerX()->flushTx();
      nWaitErr = m_pSerX->waitForBytesRx(responseLength, RESP_TIMEOUT_MS);
      
      if (SB_OK != nWaitErr) {
         // no response or not enough char
         return ERR_NORESPONSE;
      } else {
         nErr = m_pSerX->readFile(buf, responseLength, wrote);
         if (nErr == SB_OK) {
            int cmp = strncmp(response, buf, responseLength);
            if (cmp == 0) {
               return SB_OK;
            }
         }
      }
   }
   return ERR_CMDFAILED;
}


#define BUF_SIZE 10

void X2USBFocus::readFirmwareVersion() {
   int nErr = SB_OK;
   char buf[255];
   unsigned long wrote = 0;
   int nWaitErr;
   unsigned long responseLength = 26;
   
   if (GetLogger())
      GetLogger()->out((char *)"X2USBFocus::readFirmwareVersion");

   LOCK_IO_PORT
   
   if (GetSerX())
      GetSerX()->purgeTxRx();
   
   memset(buf,0,255);
   
   nErr = m_pSerX->writeFile((char *)"SGETAL", 6, wrote);
   if (nErr == SB_OK) {
      if (GetLogger())
         GetLogger()->out((char *)"SGETAL");

      m_firmwareVersion = 0;

      if (GetSerX())
         GetSerX()->flushTx();
      
      nWaitErr = m_pSerX->waitForBytesRx(responseLength, RESP_TIMEOUT_MS);
      if (SB_OK == nWaitErr) {
         
         nErr = m_pSerX->readFile(buf, responseLength, wrote);
         if (nErr == SB_OK) {
            
            if (GetLogger())
               GetLogger()->out((char *)buf);
            
            if (buf[0] == 'C'  &&
                buf[1] == '=')
            {
               buf[20] = '\0';
               m_firmwareVersion = atoi(&buf[16]);
            }
         }
      }
   }
}

void X2USBFocus::applySettings() {
   bool bvalue = false;

   if (GetLogger())
      GetLogger()->out((char *)"X2USBFocus::applySettings");

   this->halfstep(bvalue);
   if (bvalue) {
      sendRequest("SMSTPD");
   } else {
      sendRequest("SMSTPF");
   }
   
   this->clockwise(bvalue);
   if (bvalue) {
      sendRequest("SMROTH");
   } else {
      sendRequest("SMROTT");
   }

   this->highspeed(bvalue);
   if (bvalue) {
      sendAndWaitForReply("SMO005","DONE\n\r");
   } else {
      sendAndWaitForReply("SMO003","DONE\n\r");
   }
   
   int ivalue = 0;
   char buf[BUF_SIZE];
   this->maxpos(ivalue);
   
   sprintf(buf,"M%05d", ivalue );
   sendAndWaitForReply(buf,"DONE\n\r");
   
   this->readFirmwareVersion();
   
}

int X2USBFocus::execModalSettingsDialog()
{
   int nErr = SB_OK;
   X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
   X2GUIInterface*					ui = uiutil.X2UI();
   X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
   bool bPressedOK = false;
   
   if (NULL == ui)
      return ERR_POINTER;
   
   //Intialize the user interface
   nErr = ui->loadUserInterface("USBFocus.ui", deviceType(), m_nInstanceIndex);
   if (nErr)
      return nErr;
   
   if (NULL == (dx = uiutil.X2DX()))
      return ERR_POINTER;
   
   // initialize UI
   bool bvalue = false;
   this->halfstep(bvalue);
   dx->setChecked("checkBox_hs",bvalue);
   
   this->clockwise(bvalue);
   dx->setChecked("checkBox_cw",bvalue);
   
   this->highspeed(bvalue);
   if (bvalue)
      dx->setChecked("radioButton_Fast",true);
   else
      dx->setChecked("radioButton_Normal",true);

   int ivalue = 0;
   this->maxpos(ivalue);
   
   dx->setPropertyInt("spinBox_maxpos","decimals", 0);
   dx->setPropertyDouble("spinBox_maxpos","maximum", 65535);
   dx->setPropertyDouble("spinBox_maxpos","minimum", 0);
   dx->setPropertyDouble("spinBox_maxpos","singleStep", 100);
   dx->setPropertyDouble("spinBox_maxpos","value", (double)ivalue);
   
   char buf[10];
   sprintf(buf, "%05d", m_firmwareVersion);
   dx->setText("fw_version", buf);
   
   //Display the user interface
   nErr = ui->exec(bPressedOK);
   if (nErr)
      return nErr;
   
   //Retreive values from the user interface
   if (bPressedOK)
   {
      double value = 0;
      dx->propertyDouble("spinBox_maxpos","value", value);
      int max = (int) value;
      this->setMaxpos(max);
      this->setHalfstep(dx->isChecked("checkBox_hs"));
      this->setClockwise(dx->isChecked("checkBox_cw"));
      this->setHighspeed(dx->isChecked("radioButton_Fast"));
      this->applySettings();
   }
   
   return nErr;
}

#define PARENT_KEY			  "X2USBFocus"
#define CHILD_KEY_PORTNAME	  "PortName"
#define CHILD_KEY_CLOCKWISE  "Clockwise"
#define CHILD_KEY_HALFSTEP   "Halfstep"
#define CHILD_KEY_HIGHSPEED  "Highspeed"
#define CHILD_KEY_MAX_POS    "Max"


void X2USBFocus::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const {
   if (NULL == pszPort)
      return;
   
   sprintf(pszPort, DEF_PORT_NAME);
   
   if (m_pIniUtil)
      m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
}

void X2USBFocus::portName(BasicStringInterface& str) const {
   char szPortName[DRIVER_MAX_STRING];
   portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);
   str = szPortName;
}

void X2USBFocus::setPortName(const char* szPort) {
   if (m_pIniUtil)
      m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORTNAME, szPort);
}

void X2USBFocus::clockwise(bool& cw) const {
   int read = 0;
   if (m_pIniUtil)
      read = m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_CLOCKWISE, 0);

   cw = (read == 0);
}

void X2USBFocus::setClockwise(bool cw) {
   if (m_pIniUtil)
      m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_CLOCKWISE, cw ? 0 : 1);
}

void X2USBFocus::halfstep(bool& hs) const {
   int read = 0;
   if (m_pIniUtil)
      read = m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_HALFSTEP, 0);
   
   hs = (read == 0);
}

void X2USBFocus::setHalfstep(bool hs) {
   if (m_pIniUtil)
      m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_HALFSTEP, hs ? 0 : 1);
}

void X2USBFocus::highspeed(bool& hs) const {
   int read = 0;
   if (m_pIniUtil)
      read = m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_HIGHSPEED, 0);
   
   hs = (read == 0);
}

void X2USBFocus::setHighspeed(bool hs) {
   if (m_pIniUtil)
      m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_HIGHSPEED, hs ? 0 : 1);
}

void X2USBFocus::maxpos(int& mp) const {
   int read = 65535;
   if (m_pIniUtil)
      read = m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_MAX_POS, 65535);
   
   mp = (int)fmin(read, 65535);
}

void X2USBFocus::setMaxpos(int mp) {
   int value = (int)fmin(mp, 65535);
   value = (int)fmax(value, 0);
   if (m_pIniUtil)
      m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_MAX_POS, value);
}


void X2USBFocus::resetDevice(const char * name) {
   int fd,rc;
   struct termios options;

   if (GetLogger())
      GetLogger()->out((char *)"X2USBFocus::resetDevice");

    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
	if (GetLogger())
	      GetLogger()->out((char *)"Error opening output file");
        return;
    }

  fcntl(fd, F_SETFL, 0);

  tcgetattr(fd, &options); 
  //setting baud rates and stuff
  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);
  options.c_cflag |= (CLOCAL | CREAD);
  tcsetattr(fd, TCSANOW, &options);
  sleep(2);
  tcsetattr(fd, TCSAFLUSH, &options);
  if (GetLogger())
	GetLogger()->out((char *)"reset successful");

  close(fd);
}

