*
* cc_ieee_lvo.asm - mathieeedoubbas.library LVO jumpers for CClib
*
* Callers (C __ASM__/__REG__) place the library base in A6 and IEEE
* arguments in d0/d1 (and d2/d3).  These stubs tail-jump to the LVO so the
* library returns directly to the C caller.
*
* Assemble with SAS/C:  asm -u cc_ieee_lvo.asm
* (-u prepends '_' to XDEF names so they match C symbols.)
*

	csect	text

	xdef	cc_ieeedp_flt_raw
	xdef	cc_ieeedp_fix_raw
	xdef	cc_ieeedp_tst_raw
	xdef	cc_ieeedp_cmp_raw
	xdef	cc_ieeedp_abs_raw
	xdef	cc_ieeedp_neg_raw
	xdef	cc_ieeedp_add_raw
	xdef	cc_ieeedp_sub_raw
	xdef	cc_ieeedp_mul_raw
	xdef	cc_ieeedp_div_raw
	xdef	cc_ieeedp_floor_raw

*
* LONG in d0 -> DOUBLE in d0/d1
*
cc_ieeedp_flt_raw:
	jmp	-36(a6)			; IEEEDPFlt

*
* DOUBLE in d0/d1 -> LONG in d0
*
cc_ieeedp_fix_raw:
	jmp	-30(a6)			; IEEEDPFix

*
* DOUBLE in d0/d1 -> cmp result in d0
*
cc_ieeedp_tst_raw:
	jmp	-48(a6)			; IEEEDPTst

*
* DOUBLE a in d0/d1, b in d2/d3 -> cmp in d0
*
cc_ieeedp_cmp_raw:
	jmp	-42(a6)			; IEEEDPCmp

cc_ieeedp_abs_raw:
	jmp	-54(a6)			; IEEEDPAbs

cc_ieeedp_neg_raw:
	jmp	-60(a6)			; IEEEDPNeg

cc_ieeedp_add_raw:
	jmp	-66(a6)			; IEEEDPAdd

cc_ieeedp_sub_raw:
	jmp	-72(a6)			; IEEEDPSub

cc_ieeedp_mul_raw:
	jmp	-78(a6)			; IEEEDPMul

cc_ieeedp_div_raw:
	jmp	-84(a6)			; IEEEDPDiv

cc_ieeedp_floor_raw:
	jmp	-90(a6)			; IEEEDPFloor

	end
