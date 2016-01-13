//
//
// The MIT License (MIT)
//
// Copyright (c) 2015  Michael J. Wouters
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdio.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

#include "Debug.h"
#include "Antenna.h"
#include "GPS.h"
#include "HexBin.h"
#include "ReceiverMeasurement.h"
#include "SVMeasurement.h"
#include "Javad.h"

extern ostream *debugStream;

// JAVAD types
#define I1 char
#define I2 short
#define I4 int
#define U1 unsigned char
#define U2 unsigned short
#define U4 unsigned int
#define F4 float
#define F8 double

Javad::Javad(Antenna *ant,string m):Receiver(ant)
{
  //model="Javad ";
	manufacturer="Javad";
	swversion="0.1";
	constellations=Receiver::GPS;
}

Javad::~Javad()
{
}

bool Javad::readLog(string fname,int mjd)
{
	DBGMSG(debugStream,1,"reading " << fname);	
	struct stat statbuf;
	
	if ((0 != stat(fname.c_str(),&statbuf))){
		DBGMSG(debugStream,1," unable to stat " << fname);
		return false;
	}
	
	ifstream infile (fname.c_str());
	string line;
	int linecount=0;
	bool newsecond=false;
	bool gotrxid=false;
	
	string msgid,currpctime,pctime,msg,gpstime;
	
	unsigned int gpstow;
	UINT16 gpswn;
	float rxtimeoffset; // single
	float sawtooth;     // single
	
	vector<SVMeasurement *> gps;
	gotIonoData = false;
	gotUTCdata=false;
	UINT8 fabss,fabmm,fabhh,fabmday,fabmon;
	UINT16 fabyyyy;
	
	U2 uint8buf;
	I1 sint8buf;
	I2 sint16buf;
	I4 sint32buf;
	U4 uint32buf;
	
  if (infile.is_open()){
    while ( getline (infile,line) ){
			linecount++;
			
			if (line.size()==0) continue; // skip empty line
			if ('#' == line.at(0)) continue; // skip comments
			if ('%' == line.at(0)) continue;
			if ('@' == line.at(0)){ 
				if (!gotrxid){
				}
				continue;
			}
			stringstream sstr(line);
			sstr >> msgid >> currpctime >> msg;
			if (sstr.fail()){
				DBGMSG(debugStream,1," bad data at line " << linecount);
				newsecond=false; // reset the state machine
				gps.clear();
				continue;
			}
			
// IO Ionosphere parameters
// 0 0  u4 tot; // Time of week [s]
// 1 4 u2 wn; // Week number (taken from the first subframe)
// // The coefficients of a cubic equation representing
// // the amplitude of the vertical delay
// 2 6 f4 alpha0; // [s]
// 3 10 f4 alpha1; // [s/semicircles]
// 4 14 f4 alpha2; // [s/semicircles2]
// 5 18 alpha3; // [s/semicircles3]
// // The coefficients of a cubic equation representing
// // the period of the model
// 6 22 beta0; // [s]
// 7 26 f4 beta1; // [s/semicircles]
// 8 30 f4 beta2; // [s/semicircles2]
// 9 34 f4 beta3; // [s/semicircles3]
//10 38 u1 cs; // Checksum
			if (!gotIonoData){
				if (msgid=="IO"){
					DBGMSG(debugStream,1,"ionosphere parameters");
					if (msg.size()==39*2){
						HexToBin((char *) msg.substr(6*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.a0);
						HexToBin((char *) msg.substr(10*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.a1);
						HexToBin((char *) msg.substr(14*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.a2);
						HexToBin((char *) msg.substr(18*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.a3);
						HexToBin((char *) msg.substr(22*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.B0);
						HexToBin((char *) msg.substr(26*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.B1);
						HexToBin((char *) msg.substr(30*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.B2);
						HexToBin((char *) msg.substr(34*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &ionoData.B3);
						gotIonoData=true;
						DBGMSG(debugStream,1,"ionosphere parameters: a0=" << ionoData.a0);
					}
					else{
						DBGMSG(debugStream,1,"Bad I0 message size");
					}
				}
			}
			
// UO UTC parameters			
//0 0 f8 a0; // Constant term of polynomial [s]
//1 8 f4 a1; // First order term of polynomial [s/s]
//2 12 u4 tot; // Reference time of week [s]
//3 16 u2 wnt; // Reference week number []
//4 18 i1 dtls; // Delta time due to leap seconds [s]
//5 19 u1 dn; // 'Future' reference day number [1…7] []
//6 20 u2 wnlsf; // 'Future' reference week number []
//7 22 i1 dtlsf; // 'Future' delta time due to leap seconds [s]
//8 23 u1 cs; // Checksum

			if (!gotUTCdata){
				if (msgid=="UO"){
					DBGMSG(debugStream,1,"UTC parameters");
					if (msg.size()==24*2){
						HexToBin((char *) msg.substr(0*2,2*sizeof(F8)).c_str(),sizeof(F8),(unsigned char *) &utcData.A0);
						HexToBin((char *) msg.substr(8*2,2*sizeof(F4)).c_str(),sizeof(F4),(unsigned char *) &utcData.A1);
						HexToBin((char *) msg.substr(12*2,2*sizeof(U4)).c_str(),sizeof(U4),(unsigned char *) &uint32buf);
						utcData.t_ot=uint32buf;
						HexToBin((char *) msg.substr(16*2,2*sizeof(U2)).c_str(),sizeof(U2),(unsigned char *) &utcData.WN_t);
						HexToBin((char *) msg.substr(18*2,2*sizeof(I2)).c_str(),sizeof(I2),(unsigned char *) &utcData.dtlS);
						HexToBin((char *) msg.substr(19*2,2*sizeof(U1)).c_str(),sizeof(U1),(unsigned char *) &uint8buf);
						utcData.DN=uint8buf;
						HexToBin((char *) msg.substr(20*2,2*sizeof(U2)).c_str(),sizeof(U2),(unsigned char *) &utcData.WN_LSF);
				
						HexToBin((char *) msg.substr(22*2,2*sizeof(I1)).c_str(),sizeof(I1),(unsigned char *) &sint8buf);
						utcData.dt_LSF=sint8buf;
						DBGMSG(debugStream,1,"UTC parameters: dtLS=" << utcData.dtlS << ",dt_LSF=" << utcData.dt_LSF);
						
						//Shouldn't get leap seconds == 0
						if (!(utcData.dtlS == 0 && utcData.dt_LSF == 0)) {
							gotUTCdata=true;
							
							// Figure out when the leap second is/was scheduled; we only have the low
							// 8 bits of the week number in WN_LSF, but we know that "the
							// absolute value of the difference between the untruncated WN and Wlsf
							// values shall not exceed 127" (ICD 20.3.3.5.2.4, p122)
							int gpsWeek=int ((mjd-44244)/7);
							int gpsSchedWeek=(gpsWeek & ~0xFF) | utcData.WN_LSF;
							while ((gpsWeek-gpsSchedWeek)> 127) {gpsSchedWeek+=256;}
							while ((gpsWeek-gpsSchedWeek)<-127) {gpsSchedWeek-=256;}
							int gpsSchedMJD=44244+7*gpsSchedWeek+utcData.DN;
							// leap seconds is either tls or tlsf depending on past/future schedule
							leapsecs=(mjd>=gpsSchedMJD? utcData.dt_LSF : utcData.dtlS);
						}
					}
					else{
						DBGMSG(debugStream,1,"Bad U) message size");
					}
				}
			}
			
// GE GPS Ephemeris			
//0 		0	u1 sv; // SV PRN number within the range [1…37]
//1 		1	u4 tow; // Time of week [s]
//2 		5	u1 flags; // Flags (see GPS ICD for details)[bitfield]:
// 			// 0 - curve fit interval
// 			// 1 - data flag for L2 P-code
// 			// 2,3 - code on L2 channel
// 			// 4 - anti-spoof (A-S) flag (from HOW)
// 			// 5 - ‘Alert’ flag (from HOW)
// 			// 6 - ephemeris was retrieved from non-volatile memory
// 			// 7 - reserved
// 			//===== Clock data (Subframe 1) =====
//3 		6	i2 iodc; // Issue of data, clock []
//4 		8	i4 toc; // Clock data reference time [s]
//5 		12	i1 ura; // User range accuracy []
//6 		13 	u1 healthS; // Satellite health []
//7 		14	i2 wn; // Week number []
//8 		16	f4 tgd; // Estimated group delay differential [s]
//9 		20	f4 af2; // Polynomial coefficient [s/(s^2)]
//10 		24	f4 af1; // Polynomial coefficient [s/s]
//11 		28	f4 af0; // Polynomial coefficient [s]
// 			//===== Ephemeris data (Subframes 2 and 3) =====
//12 		32	i4 toe; // Ephemeris reference time [s]
//13 		36	i2 iode; // Issue of data, ephemeris []
// 			//--- Keplerian orbital parameters ---
//14 		38	f8 rootA; // Square root of the semi-major axis [m^0.5]
//15 		46	f8 ecc; // Eccentricity []
//16 		54	f8 m0; // Mean Anomaly at reference time (wn,toe)
// 			// [semi-circles]
//17 		62	f8 omega0; // Longitude of ascending node of orbit plane at the
// 			// start of week ‘wn’ [semi-circles]
//18 		70	f8 inc0; // Inclination angle at reference time [semi-circles]
//19 		78	f8 argPer; // Argument of perigee [semi-circles]
// 			//--- Corrections to orbital parameters ---
//20 		86	f4 deln; // Mean motion difference from computed value
// 			// [semi-circle/s]
//21 		90	f4 omegaDot; // Rate of right ascension [semi-circle/s]
//22 		94	f4 incDot; // Rate of inclination angle [semi-circle/s]
//23 		98	f4 crc; // Amplitude of the cosine harmonic correction term
// 			// to the orbit radius [m]
//24 		102	f4 crs; // Amplitude of the cosine harmonic correction term
// 			// to the orbit radius [m]
//25 		106	f4 cuc; // Amplitude of the cosine harmonic correction term
// 			// to the argument of latitude [rad]
//26 		110	f4 cus; // Amplitude of the cosine harmonic correction term
// 			// to the argument of latitude [rad]
//27 		114	f4 cic; // Amplitude of the cosine harmonic correction term
// 			// to the angle of inclination [rad]
//28 		118	f4 cis; // Amplitude of the sine harmonic correction term
// 			// to the angle of inclination [rad]
//29 		122	u1 cs; // Checksum

			if(msgid == "GE"){  // GPS ephemeris
				if (msg.length() == 123*2){
					EphemerisData *ed = new EphemerisData;
 					HexToBin((char *) msg.substr(0*2,2*sizeof(UINT8)).c_str(), sizeof(UINT8),  (unsigned char *) &(ed->SVN));
					HexToBin((char *) msg.substr(1*2,2*sizeof(UINT32)).c_str(),sizeof(UINT32), (unsigned char *) &(uint32buf));
 					ed->t_ephem=uint32buf;
					
					HexToBin((char *) msg.substr(6*2,2*sizeof(SINT16)).c_str(),sizeof(SINT16), (unsigned char *) &(sint16buf));
						ed-> IODC=sint16buf;
					HexToBin((char *) msg.substr(8*2,2*sizeof(SINT32)).c_str(),sizeof(SINT32), (unsigned char *) &(sint32buf));
					ed->t_OC=sint32buf;
					HexToBin((char *) msg.substr(12*2,2*sizeof(SINT8)).c_str(), sizeof(SINT8),  (unsigned char *) &(sint8buf));
					ed->SV_accuracy_raw=sint8buf;
					HexToBin((char *) msg.substr(13*2,2*sizeof(UINT8)).c_str(), sizeof(UINT8),  (unsigned char *) &(ed->SV_health));
					HexToBin((char *) msg.substr(14*2,2*sizeof(SINT16)).c_str(),sizeof(SINT16), (unsigned char *) &sint16buf);
					ed->week_number=(UINT16) sint16buf;
					HexToBin((char *) msg.substr(16*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->t_GD));
						
						
// 					HexToBin((char *) msg.substr(37*2+2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->SV_accuracy));
					
					
					HexToBin((char *) msg.substr(20*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->a_f2));
					HexToBin((char *) msg.substr(24*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->a_f1));
					HexToBin((char *) msg.substr(28*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->a_f0));
					HexToBin((char *) msg.substr(32*2,2*sizeof(SINT32)).c_str(),sizeof(SINT32), (unsigned char *) &sint32buf);
					ed->t_oe=sint32buf;
					HexToBin((char *) msg.substr(36*2,2*sizeof(SINT16)).c_str(), sizeof(SINT16),  (unsigned char *) &sint16buf);
					ed->IODE=(UINT8) sint16buf; // WARNING! Truncated
					HexToBin((char *) msg.substr(38*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->sqrtA));
					
					HexToBin((char *) msg.substr(46*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->e));
					HexToBin((char *) msg.substr(54*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->M_0));
					ed->M_0 *= M_PI;
					HexToBin((char *) msg.substr(62*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->OMEGA_0));
					ed->OMEGA_0 *= M_PI;
					HexToBin((char *) msg.substr(70*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->i_0));
					ed->i_0 *= M_PI;
					HexToBin((char *) msg.substr(78*2,2*sizeof(DOUBLE)).c_str(),sizeof(DOUBLE), (unsigned char *) &(ed->OMEGA));
					ed->OMEGA *= M_PI;
					HexToBin((char *) msg.substr(86*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->delta_N));
					ed->delta_N *=M_PI;
					HexToBin((char *) msg.substr(90*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->OMEGADOT));
					ed->OMEGADOT *= M_PI;
					HexToBin((char *) msg.substr(94*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->IDOT));
					ed->IDOT *= M_PI;
					HexToBin((char *) msg.substr(98*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_rc));
					HexToBin((char *) msg.substr(102*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_rs));
					HexToBin((char *) msg.substr(106*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_uc));
					HexToBin((char *) msg.substr(110*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_us));
					HexToBin((char *) msg.substr(114*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_ic));
					HexToBin((char *) msg.substr(118*2,2*sizeof(F4)).c_str(),sizeof(F4), (unsigned char *) &(ed->C_is));
						
					DBGMSG(debugStream,3,"Ephemeris: SVN="<< (unsigned int) ed->SVN << ",toe="<< ed->t_oe << ",IODE=" << (int) ed->IODE);
					// Check whether this is a duplicate
					int issue;
					for (issue=0;issue < (int) sortedGPSEphemeris[ed->SVN].size();issue++){
						if (sortedGPSEphemeris[ed->SVN][issue]->t_oe == ed->t_oe)
							break;
					}
					if (issue == (int) sortedGPSEphemeris[ed->SVN].size() || sortedGPSEphemeris[ed->SVN].size()==0){
						ephemeris.push_back(ed);
						sortedGPSEphemeris[ed->SVN].push_back(ed);
					}
					else{
						DBGMSG(debugStream,1,"ephemeris: duplicate SVN= "<< (unsigned int) ed->SVN << " toe= " << ed->t_oe);
					}
					continue;
				}
				
			}
			
		}
	}
	else{
		DBGMSG(debugStream,1," unable to open " << fname);
		return false;
	}
	infile.close();

	
	DBGMSG(debugStream,1,"done: read " << linecount << " lines");
	DBGMSG(debugStream,1,measurements.size() << " measurements read");
	DBGMSG(debugStream,1,ephemeris.size() << " ephemeris entries read");
	return true;
	
}
