#include "fft.h"

int main()
{
    complex test[BUFFER_SIZE];
    for (uint i = 0; i < BUFFER_SIZE; ++i)
    {

        test[i].r = cos(1.0 * 2.0f * PI * i / BUFFER_SIZE);
        test[i].i = 0.0f;
    }
    fft(test);

	float folded[BUFFER_SIZE / 2];
    fold(test, folded);

	float integrated[16];
    integrate(folded, integrated);
    for (uint i = 0; i < 16; ++i) printf("%d: %f\n", i, integrated[i]);

    return 0;
}
