#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "fft.h"

const uint LED_PIN = 25;
const uint ADC_PIN = 26;

uint8_t debouncer = 0;

#define RING_BUFFER_SIZE (BUFFER_SIZE * 2)

uint16_t ring_buffer[RING_BUFFER_SIZE];
uint16_t ring_buffer_last = 0;

#define POWER_BUFFER_SIZE (BUFFER_SIZE / 2)

complex  complex_buffer[BUFFER_SIZE];
float    power_spectrum[POWER_BUFFER_SIZE];

bool adc_timer_callback(struct repeating_timer *t)
{
	ring_buffer[ring_buffer_last] = adc_read();
    ring_buffer_last = (ring_buffer_last + 1) % RING_BUFFER_SIZE;
    return true;
}

void copy_data_from_ring_buffer_to_complex_buffer()
{
	uint16_t current_index = ((ring_buffer_last - BUFFER_SIZE) + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
	for(uint i = 0; i < BUFFER_SIZE; ++i)
	{
		complex_buffer[i].r = (ring_buffer[current_index] / (4096.0f) - 0.5f) * 2.0f;
		complex_buffer[i].i = 0.0f;
		current_index = (current_index + 1) % RING_BUFFER_SIZE;
	}
}

#define F697 39
#define F770 43
#define F852 47
#define F941 52

#define F1209 67
#define F1336 74
#define F1477 82
#define F1633 91

uint8_t f_vert[] = {F697, F770, F852, F941};
uint8_t f_hor[] = {F1209, F1336, F1477, F1633};

uint8_t number_lut[4][4] = {{0x1, 0x2, 0x3, 0xa},
                            {0x4, 0x5, 0x6, 0xb},
					        {0x7, 0x8, 0x9, 0xc},
				            {0xf, 0x0, 0xe, 0xd}};

uint8_t debouncers[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t code[] = {3, 1, 4, 1, 5, 9};
#define CODE_LENGTH 6

#define max(a,b) (a > b ? a : b)

uint check_number()
{
	float average = 0.0f;
	for(uint i = 0; i < POWER_BUFFER_SIZE; ++i)	average += power_spectrum[i];
	average /= (float)POWER_BUFFER_SIZE;

	float max_valuev = 0.0f;
	uint max_indexv = 4;
	for(uint i = 0; i < 4; ++i)
	{
		uint fv = f_vert[i];
		float intensity =  (power_spectrum[fv] + power_spectrum[fv + 1] * 0.5f + power_spectrum[fv - 1] * 0.5f) * 0.5f;
		if(intensity > max_valuev)
		{
			max_valuev = intensity;
			max_indexv = i;
		}
	}

	float max_valueh = 0.0f;
	uint max_indexh = 4;
	for(uint i = 0; i < 4; ++i)
	{
		uint fh = f_hor[i];
		float intensity =  (power_spectrum[fh] + power_spectrum[fh + 1] * 0.5f + power_spectrum[fh - 1] * 0.5f) * 0.5f;
		if(intensity > max_valueh)
		{
			max_valueh = intensity;
			max_indexh = i;
		}
	}

	for(uint i = 0; i < 16; ++i)
	{
		debouncers[i] <<= 1;
		debouncers[i] |= (~0x0u << 4); //tighten up debouncers to work with auto dial
	}

	float threashold = average * 2.0f;
	if(max_valueh > threashold && max_valuev > threashold)
	{
		uint number = number_lut[max_indexv][max_indexh];
		debouncers[number] |= 1;
	}

	for(uint i = 0; i < 16; ++i)
	{
		if(debouncers[i] == ((uint8_t)~0u)) return i;
	}

	return 16;
}

int main()
{
	stdio_init_all();

	//setup GPIO for output
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	//setup adc
	adc_init();
	adc_gpio_init(ADC_PIN);
	adc_select_input(0); //0 is pin 26

	//setup timer to drive adc sampling
	struct repeating_timer adc_timer;
	add_repeating_timer_us(217, adc_timer_callback, NULL, &adc_timer);

	uint digit = 0;
	uint correct = 1;
	uint digit_correct = 2;
	uint locked = 1;
	while (1)
	{
		copy_data_from_ring_buffer_to_complex_buffer();
		fft(complex_buffer);
		fold(complex_buffer, power_spectrum);
		uint number = check_number();

		if(digit < CODE_LENGTH)
		{
			if(number >= 16)
			{
				if(digit_correct == 1)
				{
					digit++;
					digit_correct = 2;
				}
				else if(digit_correct == 0)
				{
					digit++;
					digit_correct = 2;
					correct = 0;
				}
			}
			else
			{
				if(number == code[digit]) digit_correct = 1;
				else                      digit_correct = 0;
			}
		}
		else if(correct) locked = 0;

		if(number == 0xe)
		{
			digit = 0;
			digit_correct = 2;
			locked = 1;
			correct = 1;
		}

		if(locked == 0) gpio_put(LED_PIN, 1);
		else            gpio_put(LED_PIN, 0);
	}
}
