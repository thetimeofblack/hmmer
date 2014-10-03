/* P7_TRACE:  a traceback (alignment of seq to profile).
 *****************************************************************
 * Traceback structure for alignment of a model to a sequence.
 *
 * A traceback usually only makes sense in a triplet (tr, gm, dsq),
 * for a given profile or HMM (with nodes 1..M) and a given digital
 * sequence (with positions 1..L).
 *
 * A traceback is always relative to a profile model (not a core HMM):
 * so minimally, S->N->B->{GL}->...->E->C->T.
 * 
 * It does not contain I0 or IM states.
 * D1 state can only occur as a G->D1 glocal entry.
 * 
 * N,C,J states emit on transition, not on state, so a path of N emits
 * 0 residues, NN emits 1 residue, NNN emits 2 residues, and so on. By
 * convention, the trace always associates an emission-on-transition
 * with the trailing (destination) state, so the first N, C, or J is
 * stored in a trace as a nonemitter (i=0).
 *
 * A i coords in a traceback are usually 1..L with respect to an
 * unaligned digital target sequence, but in the special case of
 * traces faked from existing MSAs (as in hmmbuild), the coords may
 * be 1..alen relative to an MSA's columns. 
 * 
 * tr->i[] and tr->pp[] values are only nonzero for an emitted residue
 * x_i; so nonemitting states {DG,DL,S,B,L,G,E,T} always have i[]=0
 * and pp[] = 0.0.
 * 
 * tr->k[] values are only nonzero for a main model state; so special
 * states {SNBLGECJT} always have k[] = 0.
 */
#ifndef p7TRACE_INCLUDED
#define p7TRACE_INCLUDED

#include "easel.h"
#include "esl_alphabet.h"
#include "esl_msa.h"

#include "base/p7_hmm.h"
#include "base/p7_profile.h"

/* State types */
enum p7t_statetype_e {
  p7T_BOGUS =  0,	/* only needed once: in _EncodeStatetype() as an error code  */
  p7T_ML    =  1,
  p7T_MG    =  2,
  p7T_IL    =  3,
  p7T_IG    =  4,
  p7T_DL    =  5,
  p7T_DG    =  6,
  p7T_S     =  7,
  p7T_N     =  8,
  p7T_B     =  9, 
  p7T_L     = 10,
  p7T_G     = 11,
  p7T_E     = 12,
  p7T_C     = 13, 
  p7T_J     = 14,
  p7T_T     = 15, 
};
#define p7T_NSTATETYPES 16	/* used when we collect statetype usage counts, for example */
#define p7_trace_IsMain(s)   ( (s) >= p7T_ML && (s) <= p7T_DG )
#define p7_trace_IsM(s)      ( (s) == p7T_ML || (s) == p7T_MG )
#define p7_trace_IsI(s)      ( (s) == p7T_IL || (s) == p7T_IG )
#define p7_trace_IsD(s)      ( (s) == p7T_DL || (s) == p7T_DG )
#define p7_trace_IsGlocal(s) ( (s) == p7T_G || (s) == p7T_MG || (s) == p7T_DG || (s) == p7T_IG)
#define p7_trace_IsLocal(s)  ( (s) == p7T_L || (s) == p7T_ML || (s) == p7T_DL || (s) == p7T_IL)

typedef struct p7_trace_s {
  int    N;		/* length of traceback                       */  // N=0 means "no traceback": viterbi score = -inf and no possible path, for example.
  int    nalloc;        /* allocated length of traceback             */
  char  *st;		/* state type code                   [0..N-1]*/
  int   *k;		/* node index; 1..M if M,D,I; else 0 [0..N-1]*/
  int   *i;		/* pos emitted in dsq, 1..L; else 0  [0..N-1]*/
  float *pp;		/* posterior prob of x_i; else 0     [0..N-1]*/
  int    M;		/* model length M (maximum k)                */
  int    L;		/* sequence length L (maximum i)             */

  /* The following section is data generated by "indexing" a trace's domains */
  int   ndom;		/* number of domains in trace (= # of B or E states) */
  int  *tfrom,   *tto;	/* locations of B/E states in trace (0..tr->N-1)     */
  int  *sqfrom,  *sqto;	/* first/last M-emitted residue on sequence (1..L)   */
  int  *hmmfrom, *hmmto;/* first/last M/D state on model (1..M)              */
  int  *anch;		/* anchor position (1.N)                             */  // I think this is only used by the mass trace stuff, now deprecated. When we delete mass trace, try to delete this
  int   ndomalloc;	/* current allocated size of these stacks            */
} P7_TRACE;



/* 1. The P7_TRACE structure */
extern P7_TRACE *p7_trace_Create(void);
extern P7_TRACE *p7_trace_CreateWithPP(void);
extern int       p7_trace_Reuse       (P7_TRACE *tr);
extern int       p7_trace_Grow        (P7_TRACE *tr);
extern int       p7_trace_GrowIndex   (P7_TRACE *tr);
extern int       p7_trace_GrowTo      (P7_TRACE *tr, int N);
extern int       p7_trace_GrowIndexTo (P7_TRACE *tr, int ndom);
extern void      p7_trace_Destroy     (P7_TRACE *tr);
extern void      p7_trace_DestroyArray(P7_TRACE **tr, int N);

/* 2. Access routines */
extern int  p7_trace_GetDomainCount   (const P7_TRACE *tr, int *ret_ndom);
extern int  p7_trace_GetStateUseCounts(const P7_TRACE *tr, int *counts);
extern int  p7_trace_GetDomainCoords  (const P7_TRACE *tr, int which, int *ret_i1, int *ret_i2, int *ret_k1, int *ret_k2);

/* 3. Debugging tools */
extern char *p7_trace_DecodeStatetype(char st);
extern int   p7_trace_Validate(const P7_TRACE *tr, const ESL_ALPHABET *abc, const ESL_DSQ *dsq, char *errbuf);
extern int   p7_trace_Dump         (FILE *fp, const P7_TRACE *tr);
extern int   p7_trace_DumpAnnotated(FILE *fp, const P7_TRACE *tr, const P7_PROFILE *gm, const ESL_DSQ *dsq);
extern int   p7_trace_Compare       (P7_TRACE *tr1, P7_TRACE *tr2, float pptol);
extern int   p7_trace_CompareLoosely(P7_TRACE *tr1, P7_TRACE *tr2, ESL_DSQ *dsq);
extern int   p7_trace_Score         (const P7_TRACE *tr, const ESL_DSQ *dsq, const P7_PROFILE *gm, float *ret_sc);
extern int   p7_trace_ScoreKahan    (const P7_TRACE *tr, const ESL_DSQ *dsq, const P7_PROFILE *gm, float *ret_sc);
extern int   p7_trace_ScoreBackwards(const P7_TRACE *tr, const ESL_DSQ *dsq, const P7_PROFILE *gm, float *ret_sc);
extern int   p7_trace_ScoreDomain(P7_TRACE *tr, ESL_DSQ *dsq, P7_PROFILE *gm, int which, float *ret_sc);
extern float p7_trace_GetExpectedAccuracy(const P7_TRACE *tr);

/* 4. Visualization tools */
extern int  p7_trace_PlotDomainInference(FILE *ofp, const P7_TRACE *tr, int ia, int ib);
extern int  p7_trace_PlotHeatMap(FILE *ofp, P7_TRACE *tr, int ia, int ib, int ka, int kb);

/* 5. Creating traces by DP traceback */
extern int  p7_trace_Append(P7_TRACE *tr, char st, int k, int i);
extern int  p7_trace_AppendWithPP(P7_TRACE *tr, char st, int k, int i, float pp);
extern int  p7_trace_Reverse(P7_TRACE *tr);
extern int  p7_trace_Index(P7_TRACE *tr);

/* 6. Creating faux traces from MSAs */
extern int  p7_trace_FauxFromMSA(ESL_MSA *msa, int *matassign, int optflags, P7_TRACE **tr);
extern int  p7_trace_Doctor(P7_TRACE *tr, int *opt_ndi, int *opt_nid);

/* 7. Counting traces into new HMMs */
extern int  p7_trace_Count(P7_HMM *hmm, ESL_DSQ *dsq, float wt, P7_TRACE *tr);

#endif /*p7TRACE_INCLUDED*/
/************************************************************
 * @LICENSE@
 *
 * SVN $Id$
 * SVN $URL$
 ************************************************************/
