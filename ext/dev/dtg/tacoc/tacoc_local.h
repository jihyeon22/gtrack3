#ifndef __TACOC_LOCAL_H__
#define __TACOC_LOCAL_H__

/**
 *       @file  tococ_local.h
 *      @brief  taco control service local procedure definition
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013년 02월 27일
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technologt, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */


void *tacoc_sms_thread(char *);
void tacoc_mdmc_power_off();
int tacoc_breakdown_report(int* );

void mdmc_power_off_thread_wrapper(void * data);

#endif // __TACOC_LOCAL_H__