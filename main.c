#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define SOC	PB0 /* PULSE on MSX (pin 8) PCINT0 */
#define CLOCK	PB1 /* TRG A on MSX (pin 6) INT0 */
#define DATA	PB2 /* UP    on MSX (pin 1) */

#define MINVAL	110
#define MAXVAL	390

static volatile uint16_t reg; /* bssセクションの先頭(00800060)に配置 */

ISR(INT0_vect, ISR_NAKED) /* メインルーチンはsleepとrjmpの繰り返しなのでレジスタの退避処理は不要 */
{
	asm volatile (
		"lds	r16, %[regh]		\n\t"
		"sbrs	r16, 0			\n\t"
		"rjmp	1f			\n\t"
		"cbi	%[ddr], %[data]		\n\t" /* Hi-Z */
		"0:				\n\t"
		"lds	r17, %[regl]		\n\t"
		"add	r17, r17		\n\t"
		"adc	r16, r16		\n\t"
		"sts	%[regh], r16		\n\t"
		"sts	%[regl], r17		\n\t"
		"in	r16, %[gifr]		\n\t"
		"ori	r16, %[pcif]		\n\t"
		"out	%[gifr], r16		\n\t"
		"reti				\n\t"
		"1:				\n\t"
		"sbi	%[ddr], %[data]		\n\t" /* Low */
		"rjmp	0b			\n\t"
		::
		[regh] "M" (0x61), /* reg上位アドレス */
		[regl] "M" (0x60), /* reg下位アドレス */
		[ddr] "I" _SFR_IO_ADDR(DDRB),
		[data] "M" (DATA),
		[gifr] "I" _SFR_IO_ADDR(GIFR),
		[pcif] "M" _BV(PCIF));
}

ISR(PCINT0_vect)
{
	if (PINB & _BV(SOC)) { /* 上昇端 */
		GIMSK = 0; /* 外部割り込み0禁止 ﾋﾟﾝ変化割り込み禁止 */
		ADCSRA |= _BV(ADEN); /* ADC有効 */
		sei();
		set_sleep_mode(SLEEP_MODE_ADC); /* A/D変換雑音低減動作 */
		sleep_cpu(); /* A/D変換開始 */
		set_sleep_mode(SLEEP_MODE_IDLE); /* ｱｲﾄﾞﾙ動作 */
		cli();
		reg = (uint32_t)ADC * (MAXVAL - MINVAL) / 1023 + MINVAL;
		ADCSRA &= ~_BV(ADEN); /* ADC無効 */
		GIMSK = _BV(INT0) | _BV(PCIE); /* 外部割り込み0許可 ﾋﾟﾝ変化割り込み許可 */
		if (reg & _BV(8)) {
			DDRB &= ~_BV(DATA); /* Hi-Z */
		} else {
			DDRB |= _BV(DATA); /* Low */
		}
		reg <<= 1;
	}
}

ISR(ADC_vect, ISR_NAKED)
{
	reti();
}

int __attribute__((OS_main)) main(void)
{
	PRR = _BV(PRTIM0); /* ﾀｲﾏ/ｶｳﾝﾀ0電力削減 */
	ACSR = _BV(ACD); /* ｱﾅﾛｸﾞ比較器禁止 */

	PORTB = _BV(CLOCK) | _BV(SOC); /* プルアップ */

	ADMUX = _BV(MUX1) | _BV(MUX0); /* ADC3 (PB3) */
	ADCSRA = _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); /* A/D変換完了割り込み許可 A/D変換ｸﾛｯｸCK/128 */
	DIDR0 = _BV(ADC3D); /* ADC3ﾃﾞｼﾞﾀﾙ入力禁止 */

	MCUCR = _BV(ISC01); /* 下降端 */
	PCMSK = _BV(PCINT0); /* PB0 */
	GIMSK = _BV(INT0) | _BV(PCIE); /* 外部割り込み0許可 ﾋﾟﾝ変化割り込み許可 */

	sleep_enable();
	sei();

	while (1) {
		sleep_cpu();
	}
}
