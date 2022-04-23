#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "fft.h"

//#include "pico_display.hpp"
//using namespace pimoroni;

const uint LED_PIN = 25;
const uint ADC_PIN = 26;

uint8_t debouncer = 0;

#define RING_BUFFER_SIZE (BUFFER_SIZE * 2)

uint16_t ring_buffer[RING_BUFFER_SIZE];
uint16_t ring_buffer_last = 0;

complex  complex_buffer[BUFFER_SIZE];
float    power_spectrum[BUFFER_SIZE/2];

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


//uint16_t buffer[PicoDisplay::WIDTH * PicoDisplay::HEIGHT];
//PicoDisplay pico_display(buffer);

int main()
{
	stdio_init_all();

	//pico_display.init();
    //pico_display.set_backlight(100);
    //iobank0_hw.io[LED_PIN].ctrl |= IO_BANK0_GPIO25_CTRL_OEOVER_VALUE_ENABLE << IO_BANK0_GPIO25_CTRL_OEOVER_LSB;
	//iobank0_hw.io[LED_PIN].ctrl |= IO_BANK0_GPIO25_CTRL_OUTOVER_VALUE_HIGH << IO_BANK0_GPIO25_CTRL_OUTOVER_LSB;

	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	//setup adc
	adc_init();
	adc_gpio_init(ADC_PIN);
	adc_select_input(0); //0 is pin 26

	//setup timer to drive adc sampling
	struct repeating_timer adc_timer;
	add_repeating_timer_us(98, adc_timer_callback, NULL, &adc_timer);

	while (1)
	{
		copy_data_from_ring_buffer_to_complex_buffer();
		fft(complex_buffer);
		fold(complex_buffer, power_spectrum);

		debouncer <<= 1;
		if(power_spectrum[12] > 2.0f) debouncer |= 1;
		gpio_put(LED_PIN, 0);
		if(debouncer == (uint8_t)~0) gpio_put(LED_PIN, 1);
	}
}
