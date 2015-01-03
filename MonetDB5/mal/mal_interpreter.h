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

/*
 * Author M. Kersten
 * The MAL Interpreter
 */

#ifndef _MAL_INTERPRET_H
#define _MAL_INTERPRET_H

#include "mal_client.h"
#include "mal_factory.h"
#include "mal_profiler.h"
#include "mal_recycle.h"

/*
 * Activation of a thread requires construction of the argument list
 * to be passed by a handle.
 */

/* #define DEBUG_FLOW */

mal_export void showErrors(Client cntxt);
mal_export MalStkPtr prepareMALstack(MalBlkPtr mb, int size);
mal_export void initMALstack(MalBlkPtr mb, MalStkPtr stk);
mal_export str runMAL(Client c, MalBlkPtr mb, MalBlkPtr mbcaller, MalStkPtr env);
mal_export str runMALsequence(Client cntxt, MalBlkPtr mb, int startpc, int stoppc, MalStkPtr stk, MalStkPtr env, InstrPtr pcicaller);
mal_export str reenterMAL(Client cntxt, MalBlkPtr mb, int startpc, int stoppc, MalStkPtr stk);
mal_export str callMAL(Client cntxt, MalBlkPtr mb, MalStkPtr *glb, ValPtr argv[], char debug);
mal_export void garbageElement(Client cntxt, ValPtr v);
mal_export void garbageCollector(Client cntxt, MalBlkPtr mb, MalStkPtr stk, int flag);
mal_export void releaseBAT(MalBlkPtr mb, MalStkPtr stk, int bid);
mal_export str malCommandCall(MalStkPtr stk, InstrPtr pci);
mal_export int isNotUsedIn(InstrPtr p, int start, int a);
mal_export str safeguardStack(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci);
mal_export str catchKernelException(Client cntxt, str ret);

mal_export ptr getArgReference(MalStkPtr stk, InstrPtr pci, int k);
#if !defined(NDEBUG) && defined(__GNUC__)
/* for ease of programming and debugging (assert reporting a useful
 * location), we use a GNU C extension to check the type of arguments,
 * and of course only when assertions are enabled */
#define getArgReference_TYPE(s, pci, k, TYPE)					\
	({															\
		assert((s)->stk[(pci)->argv[k]].vtype == TYPE_##TYPE);	\
		(TYPE *) getArgReference((s), (pci), (k));				\
	})
#define getArgReference_bit(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_bit);				\
		(bit *) &v->val.btval;						\
	})
#define getArgReference_sht(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_sht);				\
		&v->val.shval;								\
	})
#define getArgReference_bat(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_bat);				\
		&v->val.bval;								\
	})
#define getArgReference_int(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_int);				\
		&v->val.ival;								\
	})
#define getArgReference_wrd(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_wrd);				\
		&v->val.wval;								\
	})
#define getArgReference_bte(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_bte);				\
		&v->val.btval;								\
	})
#define getArgReference_oid(s, pci, k)							\
	({															\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];				\
		assert(v->vtype == TYPE_oid || v->vtype == TYPE_void);	\
		&v->val.oval;											\
	})
#define getArgReference_ptr(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_ptr);				\
		&v->val.pval;								\
	})
#define getArgReference_flt(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_flt);				\
		&v->val.fval;								\
	})
#define getArgReference_dbl(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_dbl);				\
		&v->val.dval;								\
	})
#define getArgReference_lng(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_lng);				\
		&v->val.lval;								\
	})
#define getArgReference_str(s, pci, k)				\
	({												\
		ValRecord *v = &(s)->stk[(pci)->argv[k]];	\
		assert(v->vtype == TYPE_str);				\
		&v->val.sval;								\
	})
#else
#define getArgReference_TYPE(s, pci, k, TYPE)	((TYPE *) getArgReference(s, pci, k))
#define getArgReference_bit(s, pci, k)	((bit *) &(s)->stk[(pci)->argv[k]].val.btval)
#define getArgReference_sht(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.shval)
#define getArgReference_bat(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.bval)
#define getArgReference_int(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.ival)
#define getArgReference_wrd(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.wval)
#define getArgReference_bte(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.btval)
#define getArgReference_oid(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.oval)
#define getArgReference_ptr(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.pval)
#define getArgReference_flt(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.fval)
#define getArgReference_dbl(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.dval)
#define getArgReference_lng(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.lval)
#define getArgReference_str(s, pci, k)	(&(s)->stk[(pci)->argv[k]].val.sval)
#endif

#define FREE_EXCEPTION(p) do { if (p && p != M5OutOfMemory) GDKfree(p); } while (0)
#endif /*  _MAL_INTERPRET_H*/
