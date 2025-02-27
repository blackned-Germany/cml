/**
 * Mini card service for key generator
 *
 * Copyright (c) 2020, CardContact Systems GmbH, Minden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of CardContact Systems GmbH nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CardContact Systems GmbH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file sc-hsm-cardservice.c
 * @author Andreas Schwier
 * @brief Minimal card service for key generator
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ctapi.h>
#include "cardservice.h"

static unsigned char aid[] = { 0xE8, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x81, 0xC3, 0x1F, 0x02, 0x01 };

static unsigned char inittemplate[] = {
	0x80, 0x02, 0x00, 0x02,					    // Option Transport PIN
	0x81, 0x06, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31,		    // T-PIN, Offset 6
	0x82, 0x08, 0x35, 0x37, 0x36, 0x32, 0x31, 0x38, 0x38, 0x30, // SO-PIN, Offset 14
	0x91, 0x01, 0x03,					    // Retry counter 3
	0x97, 0x01, 0x01
}; // One Key Domain

// Generated as PKCS#15 Secret Key Description structure in pinmgnt.js
static unsigned char skd_dskkey[] = { 0xA8, 0x2F, 0x30, 0x13, 0x0C, 0x11, 0x44, 0x69, 0x73, 0x6B,
				      0x45, 0x6E, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6F, 0x6E,
				      0x4B, 0x65, 0x79, 0x30, 0x08, 0x04, 0x01, 0x01, 0x03, 0x03,
				      0x07, 0xC0, 0x10, 0xA0, 0x06, 0x30, 0x04, 0x02, 0x02, 0x00,
				      0x80, 0xA1, 0x06, 0x30, 0x04, 0x30, 0x02, 0x04, 0x00 };

static unsigned char algo_dskkey[] = { 0x91, 0x01, 0x99 };

/**
 * Select the SmartCard-HSM application on the device
 *
 * @param ctn the card terminal number
 * @return < 0 in case of an error or SW1/SW2 return by the token.
 */
static int
selectSE(int ctn)
{
	unsigned char rdata[256];
	unsigned short SW1SW2;
	int rc;

	rc = processAPDU(ctn, 0, 0x00, 0xA4, 0x04, 0x04, sizeof(aid), aid, 0, rdata, sizeof(rdata),
			 &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Initialize SmartCard-HSM with Transport PIN and one key domain
 *
 * @param ctn the card terminal number
 * @param sopin the Security Officer PIN (SO-PIN)
 * @param sopinlen the length of the SO-PIN (must be 8)
 * @param pin the transport PIN to be set
 * @param pinlen the length of the transport PIN (must be 6)
 * @return < 0 in case of an error or SW1/SW2 return by the token.
 */
static int
initializeDevice(int ctn, unsigned char *sopin, int sopinlen, unsigned char *pin, int pinlen)
{
	unsigned short SW1SW2;
	unsigned char cdata[32];
	int rc, len;

	if ((sopin == NULL) || (sopinlen != 8) || (pin == NULL) || (pinlen != 6)) {
		return -1;
	}

	len = sizeof(inittemplate);
	memcpy(cdata, inittemplate, len);
	memcpy(cdata + 6, pin, pinlen);
	memcpy(cdata + 14, sopin, sopinlen);

	rc = processAPDU(ctn, 0, 0x80, 0x50, 0x00, 0x00, len, cdata, 0, NULL, 0, &SW1SW2);

	memset(cdata, 0, sizeof(cdata));

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Query the PIN status
 *
 * @param ctn the card terminal number
 * @return < 0 in case of an error or SW1/SW2
 */
static int
queryPIN(int ctn)
{
	unsigned short SW1SW2;
	int rc;

	rc = processAPDU(ctn, 0, 0x00, 0x20, 0x00, 0x81, 0, NULL, 0, NULL, 0, &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Query the Life-Cycle status to check if the SE is operational
 *
 * @param ctn the card terminal number
 * @return < 0 in case of an error or one of LC_OPERATIONAL or LC_CONFIGURED
 */
static int
getLifeCycleState(int ctn)
{
	int sw = queryPIN(ctn);
	return sw != 0x6984 ? LC_OPERATIONAL : LC_CONFIGURED;
}

/**
 * Verify the User PIN
 *
 * @param ctn the card terminal number
 * @param pin the PIN
 * @param pinlen the length of the PIN
 *
 * @return < 0 in case of an error or SW1/SW2
 */
static int
verifyPIN(int ctn, unsigned char *pin, int pinlen)
{
	unsigned short SW1SW2;
	int rc;

	if ((pin == NULL) || (pinlen > 16)) {
		return -1;
	}

	rc = processAPDU(ctn, 0, 0x00, 0x20, 0x00, 0x81, pinlen, pin, 0, NULL, 0, &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Change PIN
 *
 * @param ctn the card terminal number
 * @param oldpin the old PIN
 * @param oldpinlen the length of the old PIN
 * @param newpin the new PIN
 * @param newpinlen the length of the new PIN
 * @return < 0 in case of an error or SW1/SW2
 */
static int
changePIN(int ctn, unsigned char *oldpin, int oldpinlen, unsigned char *newpin, int newpinlen)
{
	unsigned char cdata[32];
	unsigned short SW1SW2;
	int rc;

	if ((oldpin == NULL) || (oldpinlen > 16) || (newpin == NULL) || (newpinlen > 16)) {
		return -1;
	}

	memcpy(cdata, oldpin, oldpinlen);
	memcpy(cdata + oldpinlen, newpin, newpinlen);

	rc = processAPDU(ctn, 0, 0x00, 0x24, 0x00, 0x81, oldpinlen + newpinlen, cdata, 0, NULL, 0,
			 &SW1SW2);

	memset(cdata, 0, sizeof(cdata));

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Generate AES-128 key as master secret
 *
 * @param ctn the card terminal number
 * @param id the key id on the device
 * @return < 0 in case of an error or SW1/SW2 return by the token.
 */
static int
generateMasterKey(int ctn)
{
	unsigned char cdata[256];
	unsigned short SW1SW2;
	int rc;

	rc = processAPDU(ctn, 0, 0x00, 0x48, 1, 0xB0, sizeof(algo_dskkey), algo_dskkey, 0, NULL, 0,
			 &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	if (SW1SW2 != 0x9000) {
		return SW1SW2;
	}

	int l = 0;
	cdata[l++] = 0x54;
	cdata[l++] = 0x02;
	cdata[l++] = 0x00;
	cdata[l++] = 0x00;
	cdata[l++] = 0x53;
	cdata[l++] = sizeof(skd_dskkey);

	memcpy(cdata + l, skd_dskkey, sizeof(skd_dskkey));
	l += sizeof(skd_dskkey);

	rc = processAPDU(ctn, 0, 0x00, 0xD7, 0xC4, 1, l, cdata, 0, NULL, 0, &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Derive a key from the master key
 *
 * @param ctn the card terminal number
 * @param label the derivation parameter (aka label)
 * @param labellen the length of the label
 * @param keybuff a 32 byte key buffer
 * @param keybuff the length of the key buffer
 * @return < 0 in case of an error or SW1/SW2 return by the token.
 */
static int
deriveKey(int ctn, unsigned char *label, int labellen, unsigned char *keybuff, int keybufflen)
{
	unsigned short SW1SW2;
	int rc;

	if ((label == NULL) || (labellen > 127) || (keybuff == NULL) || (keybufflen != 32)) {
		return -1;
	}

	rc = processAPDU(ctn, 0, 0x80, 0x78, 1, 0x99, labellen, label, 0, keybuff, keybufflen,
			 &SW1SW2);

	if (rc < 0) {
		return rc;
	}

	return SW1SW2;
}

/**
 * Terminate SE
 *
 * @param ctn the card terminal number
 * @return < 0 in case of an error or SW1/SW2
 */
static int
terminate(int ctn)
{
	unsigned char badpin[16];
	int rc, cnt, len;

	len = 17;
	do {
		len--;
		for (size_t i = 0; i < sizeof(badpin); i++) {
			badpin[i] = len;
		}
		rc = verifyPIN(ctn, badpin, len);
	} while (rc == 0x6700 && len > 5);

	cnt = 10;
	while (rc != 0x6983 && cnt > 0) {
		for (size_t i = 0; i < sizeof(badpin); i++) {
			badpin[i]++;
		}
		rc = verifyPIN(ctn, badpin, len);
		cnt--;
	}

	return rc == 0x6983 ? 0 : -1;
}

struct cardService *
getSmartCardHSMCardService()
{
	static struct cardService cs = { "SmartCard-HSM",

					 selectSE,	    initializeDevice, queryPIN,
					 getLifeCycleState, verifyPIN,	      changePIN,
					 generateMasterKey, deriveKey,	      terminate };

	return &cs;
}
