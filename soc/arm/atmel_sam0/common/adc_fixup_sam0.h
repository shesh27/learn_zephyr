/*
 * Copyright (c) 2021 Argentum Systems Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_ADC_FIXUP_H_
#define _ATMEL_SAM_ADC_FIXUP_H_

#if defined(ADC_SYNCBUSY_MASK)
#define ADC_SYNC(adc) ((adc)->SYNCBUSY.reg)
#define ADC_SYNC_MASK (ADC_SYNCBUSY_MASK)
#elif defined(ADC_STATUS_SYNCBUSY)
#define ADC_SYNC(adc) ((adc)->STATUS.reg)
#define ADC_SYNC_MASK (ADC_STATUS_SYNCBUSY)
#else
#error ADC not supported...
#endif

#if defined(ADC_INPUTCTRL_DIFFMODE)
#define ADC_DIFF(adc) (inputctrl)
#define ADC_DIFF_MASK (ADC_INPUTCTRL_DIFFMODE)
#elif defined(ADC_CTRLB_DIFFMODE)
#define ADC_DIFF(adc) ((adc)->CTRLB.reg)
#define ADC_DIFF_MASK (ADC_CTRLB_DIFFMODE)
#elif defined(ADC_CTRLC_DIFFMODE)
#define ADC_DIFF(adc) ((adc)->CTRLC.reg)
#define ADC_DIFF_MASK (ADC_CTRLC_DIFFMODE)
#else
#error ADC not supported...
#endif

#if defined(ADC_CTRLB_RESSEL)
#define ADC_RESSEL(adc)  ((adc)->CTRLB.bit.RESSEL)
#define ADC_RESSEL_8BIT  ADC_CTRLB_RESSEL_8BIT_Val
#define ADC_RESSEL_10BIT ADC_CTRLB_RESSEL_10BIT_Val
#define ADC_RESSEL_12BIT ADC_CTRLB_RESSEL_12BIT_Val
#define ADC_RESSEL_16BIT ADC_CTRLB_RESSEL_16BIT_Val
#elif defined(ADC_CTRLC_RESSEL)
#define ADC_RESSEL(adc)  ((adc)->CTRLC.bit.RESSEL)
#define ADC_RESSEL_8BIT  ADC_CTRLC_RESSEL_8BIT_Val
#define ADC_RESSEL_10BIT ADC_CTRLC_RESSEL_10BIT_Val
#define ADC_RESSEL_12BIT ADC_CTRLC_RESSEL_12BIT_Val
#define ADC_RESSEL_16BIT ADC_CTRLC_RESSEL_16BIT_Val
#else
#error ADC not supported...
#endif

#if defined(ADC_CTRLA_PRESCALER)
#define ADC_PRESCALER(adc) ((adc)->CTRLA.bit.PRESCALER)
#define ADC_CTRLx_PRESCALER_DIV ADC_CTRLA_PRESCALER_DIV
#elif defined(ADC_CTRLB_PRESCALER)
#define ADC_PRESCALER(adc) ((adc)->CTRLB.bit.PRESCALER)
#define ADC_CTRLx_PRESCALER_DIV ADC_CTRLB_PRESCALER_DIV
#else
#error ADC not supported...
#endif

#if defined(SYSCTRL_VREF_TSEN)
#define ADC_TSEN (SYSCTRL->VREF.bit.TSEN)
#elif defined(SUPC_VREF_TSEN)
#define ADC_TSEN (SUPC->VREF.bit.TSEN)
#else
#error ADC not supported...
#endif

#if defined(SYSCTRL_VREF_BGOUTEN)
#define ADC_BGEN (SYSCTRL->VREF.bit.BGOUTEN)
#elif defined(SUPC_VREF_VREFOE)
#define ADC_BGEN (SUPC->VREF.bit.VREFOE)
#else
#error ADC not supported...
#endif

#if defined(MCLK)
/* a trailing underscore and/or lumpy concatenation is used to prevent expansion */
#define ADC_SAM0_CALIB(prefix, val) \
	UTIL_CAT(ADC_CALIB_, val)( \
		(((*(uint32_t *)UTIL_CAT(UTIL_CAT(UTIL_CAT(prefix, FUSES_), val), _ADDR)) \
		>> UTIL_CAT(UTIL_CAT(UTIL_CAT(prefix, FUSES_), val), _Pos)) \
		& UTIL_CAT(UTIL_CAT(UTIL_CAT(prefix, FUSES_), val), _Msk)) \
	)

#if ADC_INST_NUM == 1
#  define ADC_FUSES_PREFIX(n) ADC_
#else
#  define ADC_FUSES_PREFIX(n) UTIL_CAT(AD, UTIL_CAT(C, UTIL_CAT(n, _)))
#endif

#if defined(ADC_FUSES_BIASCOMP) || defined(ADC0_FUSES_BIASCOMP)
#  define ADC_SAM0_BIASCOMP(n) ADC_SAM0_CALIB(ADC_FUSES_PREFIX(n), BIASCOMP)
#else
#  define ADC_SAM0_BIASCOMP(n) 0
#endif

#if defined(ADC_FUSES_BIASR2R) || defined(ADC0_FUSES_BIASR2R)
#  define ADC_SAM0_BIASR2R(n) ADC_SAM0_CALIB(ADC_FUSES_PREFIX(n), BIASR2R)
#else
#  define ADC_SAM0_BIASR2R(n) 0
#endif

#if defined(ADC_FUSES_BIASREFBUF) || defined(ADC0_FUSES_BIASREFBUF)
#  define ADC_SAM0_BIASREFBUF(n) ADC_SAM0_CALIB(ADC_FUSES_PREFIX(n), BIASREFBUF)
#else
#  define ADC_SAM0_BIASREFBUF(n) 0
#endif
#endif /* MCLK */

#ifndef ADC_REFCTRL_REFSEL_INTERNAL
#  ifdef ADC_REFCTRL_REFSEL_INTREF
#    define ADC_REFCTRL_REFSEL_INTERNAL ADC_REFCTRL_REFSEL_INTREF
#  else
#    define ADC_REFCTRL_REFSEL_INTERNAL ADC_REFCTRL_REFSEL_INT1V
#  endif
#endif

#ifndef ADC_REFCTRL_REFSEL_VDD_1_2
#  ifdef ADC_REFCTRL_REFSEL_INTVCC0
#    define ADC_REFCTRL_REFSEL_VDD_1_2 ADC_REFCTRL_REFSEL_INTVCC0
#  else
#    define ADC_REFCTRL_REFSEL_VDD_1_2 ADC_REFCTRL_REFSEL_INTVCC1
#  endif
#endif

#ifndef ADC_REFCTRL_REFSEL_VDD_1
#  ifdef ADC_REFCTRL_REFSEL_INTVCC1
#    define ADC_REFCTRL_REFSEL_VDD_1 ADC_REFCTRL_REFSEL_INTVCC1
#  endif
#endif

#endif /* _ATMEL_SAM0_ADC_FIXUP_H_ */
