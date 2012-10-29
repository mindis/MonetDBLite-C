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
 * Copyright August 2008-2012 MonetDB B.V.
 * All Rights Reserved.
 */

/*
 * @f sql_optimizer
 * @t SQL catalog management
 * @a N. Nes, M.L. Kersten
 * @+ SQL scenario
 * The SQL scenario implementation is a derivative of the MAL session scenario.
 *
 * It is also the first version that uses state records attached to
 * the client record. They are initialized as part of the initialization
 * phase of the scenario.
 *
 * August 18, 2007 the following coverage list was extracted from the SQL test set
 *
 * The SQL compiler uses only a small subset of the MAL language
 * using the default optimizer sequence.
 * This reference list is illustrative for optimizer developers .
 *
 *    2429 sql.bind                         17 algebra.select
 *    1734 algebra.join                     14 sql.append_idxbat
 *    1337 bat.reverse                      14 algebra.selectNotNil
 *     966 sql.rsColumn                     13 str.toLower
 *     964 constraints.emptySet             12 mmath.cos
 *     816 algebra.markT                    12 batcalc.hash
 *     688 bat.append                       12 batcalc.*
 *     517 algebra.joinPath                 12 algebra.markH
 *     506 algebra.project                  10 sql.bbp
 *     495 algebra.uselect                   9 sql.bind_idxbat
 *     408 bat.mirror                        9 aggr.sum
 *     366 bat.setWriteMode                  8 sql.setVariable
 *     343 sql.resultSet                     8 batcalc.length
 *     343 sql.exportResult                  7 sql.getVariable
 *     225 group.new                         7 batcalc.isnil
 *     214 algebra.semijoin                  7 batcalc.int
 *     158 group.derive                      7 algebra.thetajoin
 *     158 aggr.count                        7 algebra.find
 *     143 algebra.kdifference               6 sql.next_value
 *     113 sql.columnBind                    6 sql.exportOperation
 *     111 bat.new                           6 mmath.sin
 *     106 sql.assert                        6 calc.or
 *     104 bat.hasMoreElements               6 calc.lng
 *     104 calc.ifthenelse                   6 calc.abs
 *      98 aggr.count_no_nil                 6 calc.>
 *      85 calc.int                          6 batcalc.-
 *      85 algebra.kunion                    5 sql.importTable
 *      82 mkey.bulk_rotate_xor_hash         5 mtime.current_timestamp
 *      80 sql.bind_dbat
 *      67 sql.exportValue                   5 batcalc.bte
 *      64 bat.insert                        5 algebra.slice
 *      56 calc.sht                          5 aggr.avg
 *      52 exit MALException:str             4 str.stringlength
 *      52 exit                              4 sql.clear_table
 *      52 catch MALException:str            4 io.stdin
 *      52 bat.newIterator                   4 calc.dbl
 *      51 sql.append                        4 batcalc./
 *      51 algebra.sortTail                  4 aggr.max
 *      50 sql.affectedRows                  3 str.trim
 *      50 group.refine                      3 sql.zero_or_one
 *      48 batcalc.==                        3 mtime.current_date
 *      45 calc.!=                           3 calc.second_interval
 *      40 pcre.like                         3 bat.inplace
 *      37 calc.str                          3 batcalc.<
 *      34 calc.*                            3 algebra.groupby
 *      34 batcalc.str                       3 aggr.min
 *      33 nil:dbl                           2 str.like
 *      30 algebra.reuse                     2 sql.sql_environment
 *      28 str.stringleft                    2 sql.not_unique
 *      23 sql.dump_opt_stats                2 sql.dump_cache
 *      23 calc.isnil                        2 nil:lng
 *      23 calc.-                            2 mtime.hours
 *      21 calc./                            2 mtime.diff
 *      20 calc.==                           2 mmath.sqrt
 *      17 calc.+                            2 mmath.rand
 *      17 batcalc.+                         2 mmath.atan
 *       2 mmath.acos                        1 mapi.disconnect
 *       2 calc.not                          1 mmath.floor
 *       2 calc.date                         1 group.refine_reverse
 *       2 batcalc.not                       1 calc.month_interval
 *       2 batcalc.!=                        1 calc.min
 *       2 batcalc.<=                        1 calc.length
 *       2 aggr.rank_grp                     1 calc.flt
 *       2 aggr.exist                        1 calc.daytime
 *       1 str.substring                     1 calc.bte
 *       1 streams.openRead                  1 calc.and
 *       1 streams.close                     1 calc.<
 *       1 sql.round                         1 bstream.destroy
 *       1 sql.restart                       1 bstream.create
 *       1 sql.dec_round                     1 batcalc.sht
 *       1 pqueue.topn_max                   1 batcalc.>
 *       1 mtime.minutes                     1 algebra.kunique
 *       1 mtime.date_add_sec_interval
 *
 * Organized by module:
 *    4657 algebra
 *    4652 sql
 *    3232 bat
 *     964 constraints
 *     434 group
 *     409 calc
 *     281 aggr
 *     171 batcalc
 *      82 mkey
 *      51 str
 *      40 pcre
 *      27 mmath
 *      14 mtime
 *       4 io
 *       2 streams
 *       2 bstream
 *       1 pqueue
 *       1 mserver
 */
/*
 * @-
 * The queries are stored in the user cache after they have been
 * type checked and optimized.
 * The Factory optimizer encapsulates the query with a re-entrance
 * structure. However, this structure is only effective if
 * quite some (expensive) instructions can be safed.
 * The current heuristic is geared at avoiding trivial
 * factory structures.
 * @-
 */
#include "monetdb_config.h"
#include "mal_builder.h"
#include "mal_debugger.h"
#include "opt_prelude.h"
#include "sql_mvc.h"
#include "sql_optimizer.h"
#include "sql_scenario.h"
#include "sql_gencode.h"
#include "opt_pipes.h"

#define TOSMALL 10

#if 0
str
FXoptimizer(Client cntxt, MalBlkPtr mb, MalStkPtr stk, InstrPtr pci)
{
	InstrPtr *ns, p;
	int v;
	int i, k, newssize;
	lng clk = GDKusec();

#ifdef _SQL_OPTIMIZER_DEBUG
	mnstr_printf(GDKout, "First call to FXoptimizer\n");
	printFunction(GDKout, mb, 0, LIST_MAL_ALL);
#endif
	(void) cntxt;
	(void)stk;
	if( mb->errors) {
		/* remove the FXoptimizer request */
		if (pci)
			removeInstruction(mb, pci);
		return MAL_SUCCEED;
	}
	if( !factoryHasFreeSpace())
		return MAL_SUCCEED;

	/*
	 * @-
	 * The factory code is also used for small blocks when there
	 * is a bind operation.
	 */
	if (mb->stop <= TOSMALL){
		for(i=0; i< mb->stop; i++){
			p= getInstrPtr(mb,0);
			if (getFunctionId(p)== bindRef &&
				getModuleId(p)== sqlRef)  break;
		}
		if(i == mb->stop)
			return MAL_SUCCEED;
	}
	/*
	 * @-
	 * The first step is to collect all the sql bind operations and
	 * to extract some compiler relevant properties from the catalogue.
	 * Double bind operations should also be eliminated.
	 */
	newssize = mb->ssize * 2;
	ns = (InstrPtr *) GDKmalloc(sizeof(InstrPtr) * newssize);
	k = 1;
	ns[0] = getInstrPtr(mb, 0);	/* its signature */
	ns[0]->token = FACTORYsymbol;
 	setVarType(mb, getArg(ns[0],0), TYPE_bit);
	setVarUDFtype(mb,getArg(ns[0],0));
	for (i = 0; i < mb->stop; i++) {
		p = getInstrPtr(mb, i);
		if ( (getFunctionId(p) == bindRef ||
			  getFunctionId(p) == bindidxRef ||
			  getFunctionId(p) == binddbatRef ) &&
			getModuleId(p) == sqlRef) {
			ns[k++] = p;
		}
	}
	/*
	 * @-
	 * The prelude code has been generated, now we can inject the remaining
	 * instructions, producing a syntactic correct MAL program again.
	 */
	p = newInstruction(mb, ASSIGNsymbol);
	v = newTmpVariable(mb, TYPE_bit);
	p->barrier = BARRIERsymbol;
	p->argv[0] = v;
	pushBit(mb,p,TRUE);
	ns[k++] = p;

	for (i = 1; i < mb->stop - 1; i++) {
		p = getInstrPtr(mb, i);
		if ( !(getModuleId(p) == sqlRef &&
		     (getFunctionId(p) == bindRef ||
		      getFunctionId(p) == binddbatRef ||
		      getFunctionId(p) == bindidxRef)) ){
		}
			ns[k++] = p;
	}
	/*
	 * @-
	 * Finalize the factory loop
	 */
	p = newInstruction(mb,ASSIGNsymbol);
	p->barrier = YIELDsymbol;
	p->argv[0] = v;
	ns[k++] = p;
	p = newInstruction(mb,ASSIGNsymbol);
	p->barrier = REDOsymbol;
	p->argv[0] = v;
	ns[k++] = p;
	p = newInstruction(mb,ASSIGNsymbol);
	p->barrier = EXITsymbol;
	p->argv[0] = v;
	ns[k++] = p;
	ns[k++] = getInstrPtr(mb, i);

	mb->stop = k;
	mb->ssize = newssize;
	GDKfree(mb->stmt);
	mb->stmt = ns;

	optimizerCheck(cntxt,mb,"sql.factorize",1,GDKusec()-clk, OPT_CHECK_ALL);
/*
 * @-
 * At this stage we can once call upon the optimizers to do their work.
 * Static known information is known and stored in constant variables,
 * which can be used by the rewrite rules.
 * This all works under the assumption that the SQL layer properly invalidates
 * the cache when the underlying table is changed.
 */
#ifdef _SQL_OPTIMIZER_DEBUG
	printFunction(GDKout, mb, 0, LIST_MAL_STMT | LIST_MAPI);
#endif
	return MAL_SUCCEED;
}
#endif
/*
 * @-
 * Cost-based optimization and semantic evaluations require statistics to work with.
 * They should come from the SQL catalog or the BATs themselves.
 * The properties passed at this point are the number of rows.
 * A better way is to mark all BATs used as a constant, because that permits
 * access to all properties. However, this creates unnecessary locking during stack
 * initialization. Therfore, we store the BAT id as a property for the optimizer
 * to work with. It can pick up the BAT if needed.
 *
 * Care should be taken in marking the delta bats as empty, because their
 * purpose is to fill them during the query. Therefore, we keep track
 * of all bound tables and mark them not-empty when a direct update
 * takes place using append().
 *
 * We also reduce the number of bind operations by keeping track
 * of those seen already.  This can not be handled by the
 * common term optimizer, because the first bind has a side-effect.
 */

static int
BATlocation(str *fnme, int *bid)
{
	/* this function was formerly ATTlocation in removed file
	 * monetdb5/modules/mal/attach.c */
	BAT *b = BBPquickdesc(*bid, FALSE);
	char path[BUFSIZ], *s;

	if (b == NULL)
		return 0;

	snprintf(path, BUFSIZ, "%s%c%s%c", GDKgetenv("gdk_dbfarm"),
		 DIR_SEP, GDKgetenv("gdk_dbname"), DIR_SEP);
	GDKfilepath(path + strlen(path), BATDIR,
		    (b->T->heap.filename ? b->T->heap.filename: b->H->heap.filename), 0);
	s = strrchr(path, '.');
	if (s)
		*s = 0;
	*fnme = GDKstrdup(path);
	return 1;
}

static void
SQLgetStatistics(Client cntxt, mvc *m, MalBlkPtr mb)
{
	InstrPtr *old = NULL;
	int oldtop, i, actions = 0, size = 0;
	lng clk = GDKusec();

	old = mb->stmt;
	oldtop= mb->stop;
	size = (mb->stop *1.2 < mb->ssize)? mb->ssize: (int) (mb->stop *1.2);
	mb->stmt = (InstrPtr *) GDKzalloc(size  * sizeof(InstrPtr));
	mb->ssize = size ;
	mb->stop = 0;

	for(i=0; i<oldtop; i++){
		InstrPtr p = old[i];
		char *f = getFunctionId(p);

		if( getModuleId(p) == sqlRef &&
		    (f == bindRef || f == bindidxRef || f == binddbatRef ) ){
			ValRecord vr;
			int upd = (p->argc == 7 || p->argc == 9);
			char *sname = getVarConstant(mb, getArg(p,2+upd)).val.sval;
			char *tname = getVarConstant(mb, getArg(p,3+upd)).val.sval;
			char *cname = NULL;
			int not_null = 0;
			wrd rows = 1; /* default to cope with delta bats */
			int mode = 0;
			int k = getArg(p,0);
			sql_schema *s = mvc_bind_schema(m, sname);
			BAT *b;

			if (!s || strcmp(s->base.name, dt_schema) == 0) {
				pushInstruction(mb,p);
				continue;
			}
			if (f == binddbatRef) {
				mode = getVarConstant(mb, getArg(p,4)).val.ival;
			} else {
				cname = getVarConstant(mb, getArg(p,4+upd)).val.sval;
				mode = getVarConstant(mb, getArg(p,5+upd)).val.ival;
			}

			if (s && f == bindidxRef && cname) {
				size_t cnt;
				sql_idx *i = mvc_bind_idx(m, s, cname);

				if (i && !isRemote(i->t)) { /* skip alter and remote statements */
					cnt = store_funcs.count_idx(i);
					assert(cnt <= (size_t) GDK_oid_max);
					b = store_funcs.bind_idx(m->session->tr,i,0);
					if ( b ) {
						str loc;
						if (b->batPersistence == PERSISTENT && BATlocation(&loc,&b->batCacheid) && loc)
							varSetProp(mb, k, fileProp, op_eq, VALset(&vr, TYPE_str, loc));
						cnt = BATcount(b);
						BBPreleaseref(b->batCacheid);
					}
					rows = (wrd) cnt;
				}
			} else if (s && f == bindRef && cname) {
				size_t cnt;
				sql_table *t = mvc_bind_table(m, s, tname);
				sql_column *c = mvc_bind_column(m, t, cname);

				if (c && !isRemote(c->t)) {
					not_null = !c->null;

					cnt = store_funcs.count_col(c);
					assert(cnt <= (size_t) GDK_oid_max);
					b = store_funcs.bind_col(m->session->tr,c,0);
					if ( b ){
						str loc;
						if (b->batPersistence == PERSISTENT &&  BATlocation(&loc,&b->batCacheid) && loc)
							varSetProp(mb, k, fileProp, op_eq, VALset(&vr, TYPE_str, loc));
						cnt = BATcount(b);
						BBPreleaseref(b->batCacheid);
					}
					rows = (wrd) cnt;
				}
			} else if (s && f == binddbatRef) {
				size_t cnt;
				sql_table *t = mvc_bind_table(m, s, tname);
				sql_column *c = NULL;

				if (t->columns.set->h) {
					c = t->columns.set->h->data;

					cnt = store_funcs.count_col(c);
					rows = (wrd) cnt;
				}
			}
			if (rows > 1 && mode != RD_INS)
				varSetProp(mb, k, rowsProp, op_eq, VALset(&vr, TYPE_wrd, &rows));
			if (not_null)
				varSetProp(mb, k, notnilProp, op_eq, NULL);

			{
				int lowprop = hlbProp, highprop = hubProp;
				/* rows == cnt has been checked above to be <= GDK_oid_max */
				oid low = 0, high = low + (oid)rows;
				pushInstruction(mb, p);

				if (mode == RD_INS) {
					if (f != binddbatRef)
						low = high;
					high += 1024*1024;
				}
				if (f == binddbatRef) {
					lowprop = tlbProp;
					highprop = tubProp;
				}
				varSetProp(mb, getArg(p,0), lowprop, op_gte, VALset(&vr, TYPE_oid, &low));
				varSetProp(mb, getArg(p,0), highprop, op_lt, VALset(&vr, TYPE_oid, &high));
			}

			if (not_null)
				actions++;
		} else {
			pushInstruction(mb,p);
		}
	}
	GDKfree(old);
	optimizerCheck(cntxt,mb,"optimizer.SQLgetstatistics",actions,GDKusec()-clk,0);
}
/*
 * Optimizers steps are identified by a pipeline name. The default pipeline in the distribution has been
 * tested extensively and should provide overall good performance.
 * Additional pipelines are defined in the opt_pipes.mx file.
 *
 */
static str optimizerpipe;		/* the active pipeline */

str
initSQLoptimizer(void)
{
	char *pipe;

	/* do nothing if the pipe line is already set */
	if (optimizerpipe == NULL ){
		pipe = GDKgetenv("sql_optimizer");
		if ( pipe == NULL)
			optimizerpipe = GDKstrdup("default_pipe");
		else optimizerpipe= GDKstrdup(pipe);
	} 
	return GDKstrdup(optimizerpipe);
}

void
addOptimizers(Client c, MalBlkPtr mb)
{
	int i;
	InstrPtr q;
	ValRecord *val;
	backend *be;

	be = (backend *) c->sqlcontext;
	assert( be && be->mvc ); 	/* SQL clients should always have their state set */

	val = stack_get_var(be->mvc,"optimizer");
	addOptimizerPipe(c, mb, val? val->val.sval:"default_pipe");
	/* point queries do not require mitosis and dataflow */
	if ( be->mvc->point_query)
	for( i = mb->stop -1; i > 0; i--){
		q= getInstrPtr(mb,i);
		if (q->token == ENDsymbol)
			break;
		if ( getFunctionId(q) == mitosisRef || getFunctionId(q) == dataflowRef)
			q->token = REMsymbol;	/* they are ignored */
	}
}

void
addQueryToCache(Client c)
{
	MalBlkPtr mb;
	mvc *m;

	insertSymbol(c->nspace, c->curprg);
	trimMalBlk(c->curprg->def);
	c->blkmode = 0;
	mb = c->curprg->def;
	chkProgram(c->fdout, c->nspace, mb);
	m = ((backend *)c->sqlcontext)->mvc;
#ifdef _SQL_OPTIMIZER_DEBUG
	mnstr_printf(GDKout, "ADD QUERY TO CACHE\n");
	printFunction(GDKout,mb,0,LIST_MAL_ALL);
#endif
	/*
	 * @-
	 * An error in the compilation should be reported to the user.
	 * And if the debugging option is set, the debugger is called
	 * to allow inspection.
	 */
	if (mb->errors) {
		showErrors(c);

		if (c->listing)
			printFunction(c->fdout, mb,0, c->listing);
		if ( m->debug )
			runMALDebugger(c,c->curprg);
		return;
	}
	addOptimizers(c, mb);
	SQLgetStatistics(c,m,mb);
	if ( m->emod & mod_debug )
		addtoMalBlkHistory(mb,"getStatistics");
	optimizeMALBlock(c,mb);

	/* time to execute the optimizers */
	if( c->debug)
		optimizerCheck(c,mb,"sql.baseline",-1,0, OPT_CHECK_ALL);
#ifdef _SQL_OPTIMIZER_DEBUG
	mnstr_printf(GDKout, "ADD optimized QUERY TO CACHE\n");
	printFunction(GDKout,mb,0,LIST_MAL_ALL);
#endif
}

/*
 * @-
 * The default SQL optimizer performs a limited set of operations
 * that are known to be (reasonably) stable and effective.
 * Finegrained control over the optimizer steps is available thru
 * setting the corresponding SQL variable.
 *
 * This version simply runs through the MAL script and re-orders the instructions
 * into catalog operations, query graph, and result preparation.
 * This distinction is used to turn the function into a factory, which would
 * enable re-entry when used as a cache-optimized query.
 * The second optimization is move access mode changes on the base tables
 * to the front of the plan.
 *
 *
 */
str
SQLoptimizer(Client c)
{
	(void) c;
#ifdef _SQL_OPTIMIZER_DEBUG
	mnstr_printf(GDKout, "SQLoptimizer\n");
	printFunction(c->fdout, c->curprg->def,0, LIST_MAL_STMT | LIST_MAPI);
	mnstr_printf(GDKout, "done\n");
#endif
	return MAL_SUCCEED;
}
