/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.monetdb.org/Legal/MonetDBLicense
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2015 MonetDB B.V.
 * All Rights Reserved.
 */

#ifndef _ARGVCMDS_H
#define _ARGVCMDS_H 1

#include <utils/utils.h>

int command_help(int argc, char *argv[]);
int command_version(void);
int command_create(int argc, char *argv[]);
int command_get(confkeyval *ckv, int argc, char *argv[]);
int command_set(confkeyval *ckv, int argc, char *argv[]);
int command_stop(confkeyval *ckv, int argc, char *argv[]);

#endif
