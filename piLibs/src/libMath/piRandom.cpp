#define IM1 2147483563
#define IM2 2147483399
#define IMM1 (IM1-1)
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 32
#define NDIV (1+IMM1/NTAB)

/*
Long period (>2^1018) random number generator of LEcuyer with Bays-Durham shuffle
and added safeguards. Returns a uniform random deviate between 0.0 and 1.0 (exclusive of
the endpoint values). Call with idum a negative integer to initialize; thereafter, do not alter
idum between successive deviates in a sequence. RNMX should approximate the largest floating
value that is less than 1.
*/

static long idum2=123456789;
static long iy=0;
static long iv[NTAB];

void nrcp_rand2_init( long *idum )
{
    int j;
    long    k;

    if (-(*idum) < 1) 
        *idum=1;       //             Be sure to prevent idum = 0.
    else
        *idum = -(*idum);
    idum2=(*idum);
    for (j=NTAB+7;j>=0;j--)
        {        //             Load the shuffle table (after 8 warm-ups).
        k=(*idum)/IQ1;
        *idum=IA1*(*idum-k*IQ1)-k*IR1;
        if (*idum < 0) *idum += IM1;
        if (j < NTAB) iv[j] = *idum;
        }
    iy=iv[0];
}

long nrcp_rand2_int31( long *idum )
{
    int j;
    long k;


    k=(*idum)/IQ1;                   // Start here when not initializing.
    *idum=IA1*(*idum-k*IQ1)-k*IR1;   // Compute idum=(IA1*idum) % IM1 without
                                    // overflows by Schrages method. if (*idum < 0) *idum += IM1;
    k=idum2/IQ2;
    idum2=IA2*(idum2-k*IQ2)-k*IR2;    //     Compute idum2=(IA2*idum) % IM2 likewise.
    if (idum2 < 0) 
        idum2 += IM2;
    j=iy/NDIV;                        //     Will be in the range 0..NTAB-1.
    iy=iv[j]-idum2;                   //     Here idum is shuffled, idum and idum2 are
                                       //combined to generate output.
    iv[j] = *idum;
    if (iy < 1) 
        iy += IMM1;

    return iy;
}


//=========================================================================

unsigned int pirand_int15b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;
    return( (mirand[0]>>16)&32767 );
}


float pirand_flo23b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;

    unsigned int a = (unsigned int)mirand[0];
    a = (a>>9) | 0x3f800000;
    float res = *((float*)&a);
    res -= 1.0f;
    return( res );
}



float pirand_sflo23b( int *mirand )
{
    mirand[0] = mirand[0]*0x343fd+0x269ec3;

    unsigned int a = (unsigned int)mirand[0];
    a = (a>>9) | 0x40000000;
    float res = *((float*)&a);
    res -= 3.0f;
    return( res );
}
