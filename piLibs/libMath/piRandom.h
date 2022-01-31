#pragma once

unsigned int pirand_int15b( int *mirand );
float        pirand_flo23b( int *mirand );
float        pirand_sflo23b( int *mirand );


//-----------------------------------------------------
// numerical recipes
//-----------------------------------------------------

// period = 2*10^18
void nrcp_rand2_init( long *idnum );
long nrcp_rand2_int31( long *idum );   

