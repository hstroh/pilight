/*
	Copyright (C) 2014 CurlyMo & easy12 & hstroh

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../pilight.h"
#include "common.h"
#include "dso.h"
#include "log.h"
#include "protocol.h"
#include "hardware.h"
#include "binary.h"
#include "gc.h"
#include "rev_v4.h"

static void rev4CreateMessage(int id, int unit, int state) {
    rev4_switch->message = json_mkobject();
    json_append_member(rev4_switch->message, "id", json_mknumber(id, 0));
    json_append_member(rev4_switch->message, "unit", json_mknumber(unit, 0));
    if(state == 1) {
        json_append_member(rev4_switch->message, "state", json_mkstring("on"));
    } else {
        json_append_member(rev4_switch->message, "state", json_mkstring("off"));
	}
}

static void rev4ParseBinary(void) {
	int x = 0;

	/* Convert the one's and zero's into binary */
	for(x=0; x<rev4_switch->rawlen; x+=4) {
		if(rev4_switch->code[x+3] == 1) {
			rev4_switch->binary[x/4]=1;
		} else {
			rev4_switch->binary[x/4]=0;
		}
	}

	int unit = binToDec(rev4_switch->binary, 6, 9);
	int state = rev4_switch->binary[11];
	int id = binToDec(rev4_switch->binary, 0, 5);

	rev4CreateMessage(id, unit, state);
}

static void rev4CreateLow(int s, int e) {
    int i;

    for(i=s;i<=e;i+=4) {
        rev4_switch->raw[i]=(rev4_switch->plslen->length);
        rev4_switch->raw[i+1]=(rev4_switch->pulse*rev4_switch->plslen->length);
        rev4_switch->raw[i+2]=(rev4_switch->pulse*rev4_switch->plslen->length);
        rev4_switch->raw[i+3]=(rev4_switch->plslen->length);
    }
}

static void rev4CreateHigh(int s, int e) {
    int i;

    for(i=s;i<=e;i+=4) {
        rev4_switch->raw[i]=(rev4_switch->plslen->length);
        rev4_switch->raw[i+1]=(rev4_switch->pulse*rev4_switch->plslen->length);
        rev4_switch->raw[i+2]=(rev4_switch->plslen->length);
        rev4_switch->raw[i+3]=(rev4_switch->pulse*rev4_switch->plslen->length);
    }
}

static void rev4ClearCode(void) {
    rev4CreateHigh(0,3);
    rev4CreateLow(4,47);
}

static void rev4CreateUnit(int unit) {
    int binary[255];
    int length = 0;
    int i=0, x=0;

    length = decToBinRev(unit, binary);
    for(i=0;i<=length;i++) {
        if(binary[i]==1) {
            x=i*4;
            rev4CreateHigh(24+x, 24+x+3);
        }
    }
}

static void rev4CreateId(int id) {
    int binary[255];
    int length = 0;
    int i=0, x=0;

    length = decToBinRev(id, binary);
    for(i=0;i<=length;i++) {
        x=i*4;
        if(binary[i]==1) {
            rev4CreateHigh(x, x+3);
        }
    }
}

static void rev4CreateState(int state) {
    if(state == 0) {
        rev4CreateLow(40,43);
        rev4CreateHigh(44,47);
    } else {
        rev4CreateLow(40,43);
        rev4CreateHigh(44,47);
    }
}

static void rev4CreateFooter(void) {
    rev4_switch->raw[48]=(rev4_switch->plslen->length);
    rev4_switch->raw[49]=(PULSE_DIV*rev4_switch->plslen->length);
}

static int rev4CreateCode(JsonNode *code) {
    int id = -1;
    int unit = -1;
    int state = -1;
    double itmp = -1;

    if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);

    if(json_find_number(code, "off", &itmp) == 0)
        state=0;
    else if(json_find_number(code, "on", &itmp) == 0)
        state=1;

    if(json_find_number(code, "unit", &itmp) == 0)
        unit = (int)round(itmp);

    if(id == -1 || unit == -1 || state == -1) {
        logprintf(LOG_ERR, "rev4_switch: insufficient number of arguments");
        return EXIT_FAILURE;
    } else if(id > 63 || id < 0) {
        logprintf(LOG_ERR, "rev4_switch: invalid id range");
        return EXIT_FAILURE;
    } else if(unit > 15 || unit < 0) {
        logprintf(LOG_ERR, "rev4_switch: invalid unit range");
        return EXIT_FAILURE;
    } else {
        rev4CreateMessage(id, unit, state);
        rev4ClearCode();
        rev4CreateUnit(unit);
        rev4CreateId(id);
        rev4CreateState(state);
        rev4CreateFooter();
    }
    return EXIT_SUCCESS;
}

static void rev4PrintHelp(void) {
    printf("\t -t --on\t\t\tsend an on signal\n");
    printf("\t -f --off\t\t\tsend an off signal\n");
    printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
    printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

#ifndef MODULE
__attribute__((weak))
#endif
void rev4Init(void) {
    protocol_register(&rev4_switch);
    protocol_set_id(rev4_switch, "rev4_switch");
    protocol_device_add(rev4_switch, "rev4_switch", "Rev Switches v4");
    protocol_plslen_add(rev4_switch, 264);
    protocol_plslen_add(rev4_switch, 258);
    rev4_switch->devtype = SWITCH;
    rev4_switch->hwtype = RF433;
    rev4_switch->pulse = 3;
    rev4_switch->rawlen = 50;
    rev4_switch->binlen = 12;

    options_add(&rev4_switch->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
    options_add(&rev4_switch->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
    options_add(&rev4_switch->options, 'u', "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]|[1][0-5])$");
    options_add(&rev4_switch->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(6[0123]|[12345][0-9]|[0-9]{1})$");

    options_add(&rev4_switch->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

    rev4_switch->parseBinary=&rev4ParseBinary;
    rev4_switch->createCode=&rev4CreateCode;
    rev4_switch->printHelp=&rev4PrintHelp;
}

#ifdef MODULE
void compatibility(struct module_t *module) {
	module->name = "rev4_switch";
	module->version = "0.9";
	module->reqversion = "5.0";
	module->reqcommit = "84";
}

void init(void) {
	rev4Init();
}
#endif
