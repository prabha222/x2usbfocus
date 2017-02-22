
/*
 
 Copyright (C) 2015  ando.io
 
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

#ifndef __X2Focuser_H_
#define __X2Focuser_H_

#include "../../licensedinterfaces/focuserdriverinterface.h"
#include "../../licensedinterfaces/serialportparams2interface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/focuser/focusertemperatureinterface.h"

// Forward declare the interfaces that this device is dependent upon
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class SimpleIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;

/*!
\brief The X2Focuser example.

\ingroup Example

Use this example to write an X2Focuser driver.
*/
class X2USBFocus : public FocuserDriverInterface, public SerialPortParams2Interface, public ModalSettingsDialogInterface, public FocuserTemperatureInterface
{
public:
	X2USBFocus(const char* pszDisplayName,
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn);

   ~X2USBFocus();

public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual int									queryAbstraction(const char* pszName, void** ppVal);
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const							;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const				;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const				;	
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const		;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)				;
	virtual void deviceInfoModel(BasicStringInterface& str)							;
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void);
	virtual int									terminateLink(void);
	virtual bool								isLinked(void) const;
	//@}
   
   //SerialPortParams2Interface
   virtual void			portName(BasicStringInterface& str) const			;
   virtual void			setPortName(const char* szPort)						;
   
   virtual unsigned int	baudRate() const			{return 9600;};
   virtual void			setBaudRate(unsigned int)	{};
   virtual bool			isBaudRateFixed() const		{return true;}
   
   virtual SerXInterface::Parity	parity() const				{return SerXInterface::B_NOPARITY;}
   virtual void					setParity(const SerXInterface::Parity& parity){};
   virtual bool					isParityFixed() const		{return true;}
   
   //Use default impl of SerialPortParams2Interface for rest
   
   //
   /*!\name ModalSettingsDialogInterface Implementation
    See ModalSettingsDialogInterface.*/
   //@{
   virtual int								initModalSettingsDialog(void){return 0;}
   virtual int								execModalSettingsDialog(void);
   //@}

	/*!\name FocuserGotoInterface2 Implementation
	See FocuserGotoInterface2.*/
	virtual int									focPosition(int& nPosition) 			;
	virtual int									focMinimumLimit(int& nMinLimit) 		;
	virtual int									focMaximumLimit(int& nMaxLimit)			;
	virtual int									focAbort()								;

	virtual int								startFocGoto(const int& nRelativeOffset)	;
	virtual int								isCompleteFocGoto(bool& bComplete) const	;
	virtual int								endFocGoto(void)							;

	virtual int								amountCountFocGoto(void) const				;
	virtual int								amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount);
	virtual int								amountIndexFocGoto(void)					;
	//@}
   
   /*!\name FocuserTemperatureInterface Implementation
    See FocuserTemparaturInterface.*/
   virtual int									focTemperature(double &dTemperature) 			;
   //@}
   
   

private:
   
   int sendAndWaitForReply(const char * request, const char * response) ;
   int sendRequest(const char * request);
   void portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const;
   void clockwise(bool& cw) const;
   void setClockwise(bool cw);
   void halfstep(bool& hs) const;
   void setHalfstep(bool hs);
   void highspeed(bool& hs) const;
   void setHighspeed(bool hs);
   void maxpos(int& mp) const;
   void setMaxpos(int mp);
   
   void applySettings();
   void readFirmwareVersion();
   void resetDevice(const char* name);

	//Standard device driver tools
	SerXInterface*							m_pSerX;		
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*					m_pIniUtil;
	LoggerInterface*						m_pLogger;
	mutable MutexInterface*					m_pIOMutex;
	TickCountInterface*						m_pTickCount;

	SerXInterface 							*GetSerX() const {return m_pSerX; }
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface						*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface					*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex()  {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}

	bool m_bLinked;
   bool m_bDoingGoto;
   int m_dPosition;
   int m_targetPosition;
   int m_nInstanceIndex;
   int m_firmwareVersion;
};


#endif //__X2Focuser_H_
