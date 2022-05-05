#include <stdio.h>
#include <math.h>
#include <stdint.h>

typedef unsigned int uint;

#define PI 3.141592653589792f
#define BUFFER_SIZE 256

typedef struct complex
{
    float r;
    float i;
}complex;

complex add(complex a, complex b)
{
    return (complex){ a.r + b.r, a.i + b.i };
}

complex sub(complex a, complex b)
{
    return (complex){ a.r - b.r, a.i - b.i };
}

complex mult(complex a, complex b)
{
    return (complex){ a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r };
}

//approximation from: http://nnp.ucsd.edu/phy120b/student_dirs/shared/gnarly/misc_docs/trig_approximations.pdf
float cos_32s(float x)
{
    float x_sq = x * x;
    //return 0.99940307f + x_sq * (-0.49558072f + 0.03679168f * x_sq);
	return 0.9999932946f + x_sq * (-0.4999124376f + x_sq * (0.0414877472f + x_sq * -0.0012712095f));
}

//only valid to 3.2 digits and for -2*PI to 2*PI
float cos_32(float x)
{
    //x = x % twopi; // Get rid of values > 2* pi
    x = (x < 0.0f) ? -x : x; // cos(-x) = cos(x)

    uint quad = (uint)(x * (2.0f / PI)); // Get quadrant # (0 to 3)
    switch (quad)
    {
    case 0: return cos_32s(x);
    case 1: return -cos_32s(PI - x);
    case 2: return -cos_32s(x - PI);
    case 3: return cos_32s(2.0f * PI - x);
    }

    return 0.0f;
}

//only valid to 3.2 digits and for -3/2*PI to 5/2*PI
inline static float sin_32(float x)
{
    return cos_32(x - (PI / 2.0f));
}

complex complex_exp(float exp)
{
    return (complex){ cos_32(exp), sin_32(exp)};
}

void fft(complex* buffer0)
{
	complex buffer1[BUFFER_SIZE];
    for (uint i = 0; i < BUFFER_SIZE; ++i) buffer1[i] = buffer0[i];
    complex* buffers[] = { buffer0, buffer1 };

    typedef struct StackEntry
    {
        uint16_t start;
        uint16_t step;
        uint8_t swap;
        uint8_t state;
    }StackEntry;

    StackEntry stack[21];
    uint stack_top = 0u;
    stack[stack_top] = (StackEntry){0, 1, 0, 0};

    while (stack_top != ~0)
    {
        StackEntry* current_entry = &stack[stack_top];
        if (current_entry->state == 0)
        {
            uint16_t new_step = current_entry->step * 2;
            if (new_step < BUFFER_SIZE)
            {
                stack[++stack_top] = (StackEntry){            current_entry->start,                        new_step, (uint8_t)(current_entry->swap ^ 0x1), 0};
                stack[++stack_top] = (StackEntry){ (uint16_t)(current_entry->start + current_entry->step), new_step, (uint8_t)(current_entry->swap ^ 0x1), 0};
            }
            current_entry->state = 1;
        }
        else
        {
            complex* temp_buffer0 = buffers[current_entry->swap ^ 0x0] + current_entry->start;
            complex* temp_buffer1 = buffers[current_entry->swap ^ 0x1] + current_entry->start;

            uint step = current_entry->step * 2;
            for (uint i = 0; i < BUFFER_SIZE; i += step)
            {
                complex t = mult(complex_exp(-PI * i / BUFFER_SIZE), temp_buffer1[i + current_entry->step]);
                temp_buffer0[(i + 0          ) / 2] = add(temp_buffer1[i], t);
                temp_buffer0[(i + BUFFER_SIZE) / 2] = sub(temp_buffer1[i], t);
            }
            stack_top--;
        }
    }
}

void fold(complex* in, float* out)
{
    for (uint i = 0; i < BUFFER_SIZE / 2; ++i)
    {
        complex temp = in[BUFFER_SIZE - i - 1];
        out[i] = temp.r * temp.r + temp.i * temp.i;
    }
}

static uint8_t ranges[] = { 1, 1, 2, 3, 3, 6, 7, 10, 14, 20, 28, 38, 54, 75, 104, 146 };
void integrate(float* in, float* out)
{
    uint j = 0;
    for (uint i = 0; i < 16; ++i)
    {
        out[i] = 0;
        uint end = j + ranges[i];
        for (;j < end; ++j)
            out[i] += in[j];
    }
}
