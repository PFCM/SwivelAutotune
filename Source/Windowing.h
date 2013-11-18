//
//  Windowing.h
//  SwivelAutotune
//
//  Created by Paul Francis Cunninghame Mathews on 18/11/13.
//
//

#ifndef SwivelAutotune_Windowing_h
#define SwivelAutotune_Windowing_h
#include <cmath>
#include <Accelerate/Accelerate.h>
#define TWOPI 2*M_PI

class Windowing
{
public:
    static void hann(double* input, int size)
    {
        static double* window = nullptr;
        static int lastSize = 0;
        if (lastSize != size)
        {
            if (window != nullptr) free(window);
            window = (double*) malloc(size*sizeof(double));
            /*for (int i = 0; i < size; i++)
            {
                window[i] = 0.5 * (1.0-cos((TWOPI*i)/size-1));
            }*/
            vDSP_hann_windowD(window, size, 0);
        }
        vDSP_vmulD(window, 1, input, 1, input, 1, size);
        
        lastSize = size;
    }
};

#endif
