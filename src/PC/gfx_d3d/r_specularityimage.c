#include "common_types.h"
#include <math.h>
#include "imports.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif


void Image_BuildSpecularityMap(float shift, byte *pic)
{
    int power;
    double exponent;

    for (power = 0; power <= 31; power++) {
        if (power == 31) {
            exponent = 30.0;
        } else {
            double normalized = (double)((float)power / 30.0f + shift);
            double logInput;

            if (normalized < 0.0)
                logInput = 0.0;
            else if (normalized <= 1.0)
                logInput = normalized * -175.0;
            else
                logInput = -175.0;

            exponent = log(0.1) / log(cos((logInput + 180.0) * (M_PI / 720.0)));
        }

        byte *dst = pic + power;
        int level;
        for (level = 0; level < 256; level++) {
            double intensity = (double)((float)level * (1.0f / 255.0f));
            double value = pow(intensity, exponent) * 255.0;
            *dst = (byte)(int)floorf((float)value + 0.5f);
            dst += 32;
        }
    }
}
