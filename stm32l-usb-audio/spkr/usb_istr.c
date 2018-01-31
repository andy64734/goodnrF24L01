/**
  ******************************************************************************
  * @file    usb_istr.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   ISTR events interrupt service routines
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"


// Private variables
__IO uint16_t wIstr;               // ISTR register last read value
__IO uint8_t  bIntPackSOF = 0;     // SOFs received between 2 consecutive packets
__IO uint32_t esof_counter = 0;    // expected SOF counter


// function pointers to non-control endpoints service routines
void (*pEpInt_IN[7])(void) = {
		EP1_IN_Callback,
		EP2_IN_Callback,
		EP3_IN_Callback,
		EP4_IN_Callback,
		EP5_IN_Callback,
		EP6_IN_Callback,
		EP7_IN_Callback
};

void (*pEpInt_OUT[7])(void) = {
		EP1_OUT_Callback,
		EP2_OUT_Callback,
		EP3_OUT_Callback,
		EP4_OUT_Callback,
		EP5_OUT_Callback,
		EP6_OUT_Callback,
		EP7_OUT_Callback
};


/*******************************************************************************
* Function Name  : USB_Istr
* Description    : ISTR events interrupt service routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Istr(void) {
	uint32_t i = 0;
	__IO uint32_t EP[8];
  
	wIstr = *ISTR;

#if (IMR_MSK & ISTR_SOF)
	if (wIstr & ISTR_SOF & wInterrupt_Mask) {
		*ISTR = CLR_SOF;
		bIntPackSOF++;

#ifdef SOF_CALLBACK
		SOF_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_SOF

	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
  
#if (IMR_MSK & ISTR_CTR)
	if (wIstr & ISTR_CTR & wInterrupt_Mask) {
		// servicing of the endpoint correct transfer interrupt
		// clear of the CTR flag into the sub
		CTR_LP();
#ifdef CTR_CALLBACK
		CTR_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_CTR

	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_RESET)
	if (wIstr & ISTR_RESET & wInterrupt_Mask) {
		*ISTR = CLR_RESET;
		Device_Property.Reset();
#ifdef RESET_CALLBACK
		RESET_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_RESET

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_DOVR)
	if (wIstr & ISTR_DOVR & wInterrupt_Mask) {
		*ISTR = CLR_DOVR;
#ifdef DOVR_CALLBACK
		DOVR_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_DOVR

	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_ERR)
	if (wIstr & ISTR_ERR & wInterrupt_Mask) {
		*ISTR = CLR_ERR;
#ifdef ERR_CALLBACK
		ERR_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_ERR

	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_WKUP)
	if (wIstr & ISTR_WKUP & wInterrupt_Mask) {
    	*ISTR = CLR_WKUP;
		Resume(RESUME_EXTERNAL);
#ifdef WKUP_CALLBACK
		WKUP_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_WKUP

	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_SUSP)
	if (wIstr & ISTR_SUSP & wInterrupt_Mask) {
		// check if SUSPEND is possible
		if (fSuspendEnabled) {
			Suspend();
		} else {
			// if not possible then resume after xx ms
			Resume(RESUME_LATER);
		}
		// clear of the ISTR bit must be done after setting of CNTR_FSUSP
		*ISTR = CLR_SUSP;
#ifdef SUSP_CALLBACK
		SUSP_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_SUSP

  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#if (IMR_MSK & ISTR_ESOF)
	if (wIstr & ISTR_ESOF & wInterrupt_Mask) {
		// clear ESOF flag in ISTR
		*ISTR = CLR_ESOF;
    
		if (*FNR & FNR_RXDP) {
			// increment ESOF counter
			esof_counter++;
      
			// test if we enter in ESOF more than 3 times with FSUSP =0 and RXDP =1=>> possible missing SUSP flag
			if ((esof_counter > 3) && ((*CNTR & CNTR_FSUSP) == 0)) {
				// this a sequence to apply a force RESET
      
				// Store endpoints registers status
				for (i = 0; i < 8; i++) EP[i] = _GetENDPOINT(i);
      
				// apply FRES
				*CNTR |= CNTR_FRES;
				// clear FRES
				*CNTR &= ~CNTR_FRES;
				// poll for RESET flag in ISTR
				while (!(*ISTR & ISTR_RESET));
  
				// clear RESET flag in ISTR
				*ISTR = CLR_RESET;
   
				// restore endpoints
				for (i = 0; i < 8; i++) _SetENDPOINT(i,EP[i]);
      
				esof_counter = 0;
			}
		} else {
			esof_counter = 0;
		}
    
		// resume handling timing is made with ESOFs
		Resume(RESUME_ESOF); // request without change of the machine state

#ifdef ESOF_CALLBACK
		ESOF_Callback();
#endif
	}
#endif // IMR_MSK & ISTR_ESOF
} // USB_Istr
