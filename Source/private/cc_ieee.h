/*
 * private/cc_ieee.h - mathieeedoubbas.library via per-task mathbase
 *
 * CClib.Doc: the shared library uses only IEEE doubles through the base
 * passed to SetupSTDIO.  Never use C `double` arithmetic here (that pulls
 * SAS/C softfloat / sc.lib).  Call IEEEDP* with the task's mathbase in A6.
 */

#ifndef CCLIB_PRIVATE_CC_IEEE_H
#define CCLIB_PRIVATE_CC_IEEE_H

#include <exec/types.h>

/* mathieeedoubbas LVOs (bias from library base) */
#define CC_LVO_IEEEDPFix   (-30)
#define CC_LVO_IEEEDPFlt   (-36)
#define CC_LVO_IEEEDPCmp   (-42)
#define CC_LVO_IEEEDPTst   (-48)
#define CC_LVO_IEEEDPAbs   (-54)
#define CC_LVO_IEEEDPNeg   (-60)
#define CC_LVO_IEEEDPAdd   (-66)
#define CC_LVO_IEEEDPSub   (-72)
#define CC_LVO_IEEEDPMul   (-78)
#define CC_LVO_IEEEDPDiv   (-84)
#define CC_LVO_IEEEDPFloor (-90)
#define CC_LVO_IEEEDPCeil  (-96)

APTR cc_get_mathbase(void);

DOUBLE cc_ieeedp_flt(APTR base, LONG v);
LONG cc_ieeedp_fix(APTR base, DOUBLE v);
LONG cc_ieeedp_tst(APTR base, DOUBLE v);
LONG cc_ieeedp_cmp(APTR base, DOUBLE a, DOUBLE b);
DOUBLE cc_ieeedp_abs(APTR base, DOUBLE v);
DOUBLE cc_ieeedp_neg(APTR base, DOUBLE v);
DOUBLE cc_ieeedp_add(APTR base, DOUBLE a, DOUBLE b);
DOUBLE cc_ieeedp_sub(APTR base, DOUBLE a, DOUBLE b);
DOUBLE cc_ieeedp_mul(APTR base, DOUBLE a, DOUBLE b);
DOUBLE cc_ieeedp_div(APTR base, DOUBLE a, DOUBLE b);
DOUBLE cc_ieeedp_floor(APTR base, DOUBLE v);

DOUBLE cc_ieeedp_zero(void);
DOUBLE cc_ieeedp_ten(APTR base);
DOUBLE cc_ieeedp_pow10(APTR base, LONG exp);

/* Parse like atof using only IEEEDP* (base must be non-NULL). */
DOUBLE cc_atof_ieee(APTR base, STRPTR s);

/*
 * Format DOUBLE into buf (at least 64 bytes).  style: 'f','e','g' (or upper).
 * prec: digits after decimal for f/e, significant for g; <0 means default 6.
 * Returns length excluding NUL.
 */
LONG cc_dtoa_ieee(APTR base, DOUBLE x, STRPTR buf, LONG prec, LONG style);

#endif /* CCLIB_PRIVATE_CC_IEEE_H */
