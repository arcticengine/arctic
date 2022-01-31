#include <cmath>

#define DDC_PI 3.1415926535897932384626433832795f

static __forceinline unsigned int ReverseBits ( unsigned int index, unsigned int NumBits ) {
    unsigned int rev = 0;
    for (unsigned int i=0; i < NumBits; i++) {
        rev = (rev << 1) | (index & 1);
        index >>= 1;
    }
    return rev;
}

void piFft1( unsigned  int log2NumSamples, int InverseTransform, const float *RealIn, const float *ImagIn, 
    float *RealOut, float *ImagOut) {
    unsigned int i, j, k, n;
    unsigned int BlockSize, BlockEnd;

    float angle_numerator = 2.0f * DDC_PI;
    float tr, ti;     /* temp real, temp imaginary */

    if (InverseTransform) {
      angle_numerator = -angle_numerator;
    }

    const unsigned int NumSamples = 1<<log2NumSamples;

    for (i=0; i < NumSamples; i++) {
        j = ReverseBits ( i, log2NumSamples );
        RealOut[j] = RealIn[i];
        ImagOut[j] = (ImagIn == 0) ? 0.0f : ImagIn[i];
    }

    //   Do the FFT itself...
    BlockEnd = 1;
    for (BlockSize = 2; BlockSize <= NumSamples; BlockSize <<= 1) {
        const float delta_angle = angle_numerator / (float)BlockSize;
        const float sm2 = sinf ( -2.0f * delta_angle );
        const float sm1 = sinf ( -delta_angle );
        const float cm2 = cosf ( -2.0f * delta_angle );
        const float cm1 = cosf ( -delta_angle );
        const float w = 2.0f * cm1;
        float ar[3], ai[3];

        for (i=0; i < NumSamples; i += BlockSize) {
            ar[2] = cm2;
            ar[1] = cm1;

            ai[2] = sm2;
            ai[1] = sm1;

            for (j=i, n=0; n < BlockEnd; j++, n++) {
                ar[0] = w*ar[1] - ar[2];
                ar[2] = ar[1];
                ar[1] = ar[0];

                ai[0] = w*ai[1] - ai[2];
                ai[2] = ai[1];
                ai[1] = ai[0];

                k = j + BlockEnd;
                tr = ar[0]*RealOut[k] - ai[0]*ImagOut[k];
                ti = ar[0]*ImagOut[k] + ai[0]*RealOut[k];

                RealOut[k] = RealOut[j] - tr;
                ImagOut[k] = ImagOut[j] - ti;

                RealOut[j] += tr;
                ImagOut[j] += ti;
            }
        }

        BlockEnd = BlockSize;
    }
}

//------------------------------------------------

void piFft2(float *data, unsigned long nn, int isign) {
    Ui32 m,istep;
    Ui32 n=nn << 1;
    Ui32 j=1;
    for (Ui32 i=1;i<n;i+=2) {
        if (j > i) {
          {
            float tempr=data[j];
            data[j]=data[i];
            data[i]=tempr;
          }
          {
            float tempr=data[j+1];
            data[j+1]=data[i+1];
            data[i+1]=tempr;
          }
        }
        m=nn;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    Ui32 mmax=2;
    while (n > mmax) {
        istep=mmax << 1;
        float theta=(float)isign*(6.28318530717959f/(float)mmax);
        float wtemp=sinf(0.5f*theta);
        float wpr = -2.0f*wtemp*wtemp;
        float wpi=sinf(theta);
        float wr=1.0f;
        float wi=0.0f;
        for (m=1;m<mmax;m+=2) {
            for (Ui32 i=m;i<=n;i+=istep) {
                j=i+mmax; 
                float tempr=wr*data[j]-wi*data[j+1];
                float tempi=wr*data[j+1]+wi*data[j];
                data[j]=data[i]-tempr;
                data[j+1]=data[i+1]-tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr=(wtemp=wr)*wpr-wi*wpi+wr;
            wi=wi*wpr+wtemp*wpi+wi;
        }
        mmax=istep;
    }
}
