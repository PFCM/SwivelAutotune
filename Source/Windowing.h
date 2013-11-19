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

/**
    Provides access to vDSP window generation functions, using them to window a given vector.
    Could definitely involve less copy-paste.
    vDSP includes only Hamming, Hann and Blackman windows.
 */
class Windowing
{
public:
    static void hann(double* input, int size)
    {
        static double* window = nullptr;
        static int lastSize = 0;
        if (lastSize != size)
        { // cache the actual window
            if (window != nullptr) delete[] window;
            window = new double[size];
            /*for (int i = 0; i < size; i++)
            {
                window[i] = 0.5 * (1.0-cos((TWOPI*i)/size-1));
            }*/
            vDSP_hann_windowD(window, size, vDSP_HANN_NORM);
        }
        vDSP_vmulD(window, 1, input, 1, input, 1, size);
        
        lastSize = size;
    }
    
    static void hamming(double* input, int size)
    {
        static double* window = nullptr;
        static int lastSize = 0;
        if (lastSize != size)
        { // cache the actual window
            if (window != nullptr) delete[] window;
            window = new double[size];
            /*for (int i = 0; i < size; i++)
             {
             window[i] = 0.5 * (1.0-cos((TWOPI*i)/size-1));
             }*/
            vDSP_hamm_windowD(window, size, 0);
        }
        vDSP_vmulD(window, 1, input, 1, input, 1, size);
        
        lastSize = size;
    }
    
    static void blkman(double* input, int size)
    {
        static double* window = nullptr;
        static int lastSize = 0;
        if (lastSize != size)
        { // cache the actual window
            if (window != nullptr) delete[] window;
            window = new double[size];
            /*for (int i = 0; i < size; i++)
             {
             window[i] = 0.5 * (1.0-cos((TWOPI*i)/size-1));
             }*/
            vDSP_blkman_windowD(window, size, 0);
        }
        vDSP_vmulD(window, 1, input, 1, input, 1, size);
        
        lastSize = size;
    }
};

#endif
