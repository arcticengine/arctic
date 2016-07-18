#include <malloc.h>
#include "piTextContainer.h"

#include "../../libSystem/piStr.h"
#include "../../libDataUtils/piTArray.h"
#include "../../libDataUtils/piString.h"

namespace piLibs {

typedef struct
{
	piString	 mText;
}piILine;

typedef struct
{
    bool         mEnabled;
    unsigned int mStart[2];  // line, column
    unsigned int mEnd[2];
}piISelection;

typedef struct
{
	unsigned int mStart;
	unsigned int mStop;
    unsigned int mLen;
}piIView;

typedef struct
{
	piTArray<piILine>      mLines;
	unsigned int mCursor[3]; // line, column, columndesired

    piIView      mView;
    piISelection mSelection;
}piITextContainer;

static bool allocateLine( piITextContainer *me )
{
    piILine *line = me->mLines.Alloc(1);
	if( !line ) 
		return false;

    if (!line->mText.Init(8))
		return false;

	return true;
}

static void iDestroyLine( piILine *line )
{
    line->mText.End();
}

static void iCopyLine( piILine *dst, const piILine *src )
{
    dst->mText = src->mText;
}

static void iDestroyAllLines( piITextContainer *me )
{
    const int num = me->mLines.GetLength();
    for( int i=0; i<num; i++ )
    {
        piILine *line = (piILine*)me->mLines.GetAddress(i);
        iDestroyLine( line );
    }
}

piTextContainer piTextContainer_Create( int windowLen )
{
	piITextContainer *me = (piITextContainer*)malloc( sizeof(piITextContainer) );
	if( !me ) return 0;

    me->mView.mLen = windowLen;

    if (!me->mLines.Init(32, true))
		return 0;

	if( !allocateLine(me) )
		return 0;

	me->mCursor[0] = 0;
	me->mCursor[1] = 0;
    me->mCursor[2] = 0;
	me->mView.mStart = 0;
	me->mView.mStop = me->mView.mLen;

    me->mSelection.mEnabled = false;
    me->mSelection.mStart[0] = 0;
    me->mSelection.mStart[1] = 0;
    me->mSelection.mEnd[0] = 0;
    me->mSelection.mEnd[1] = 0;


	return (piTextContainer)me;
}

bool piTextContainer_SetText( piTextContainer vme, const wchar_t *str )
{
	piITextContainer *me = (piITextContainer*)vme;
    
    iDestroyAllLines( me );
    me->mLines.SetLength( 0);
	if( !allocateLine(me) )
		return 0;
    me->mCursor[0] = 0;
    me->mCursor[1] = 0;
    me->mCursor[2] = 0;
    me->mView.mStart = 0;
    me->mView.mStop = me->mView.mLen;

    const int len = piwstrlen( str );
    for( int i=0; i<len; i++ )
    {
        wchar_t c = str[i];

        if( c==0x0D ) 
        { 
            if( !piTextContainer_NewKeys( me, KeyEnter, false ) ) 
                return false;  
            if( str[i+1]==0x0A ) i++;
        }
        else if( c>=31 && c<128 )
        { 
            if( !piTextContainer_NewChar( me, c ) ) 
                return false; 
        }
    }
    me->mCursor[0] = 0;
    me->mCursor[1] = 0;
    me->mCursor[2] = 0;
    me->mSelection.mEnabled = false;
    me->mSelection.mStart[0] = 0;
    me->mSelection.mStart[1] = 0;
    me->mSelection.mEnd[0] = 0;
    me->mSelection.mEnd[1] = 0;
    return true;
}

void piTextContanier_SetCursor( piTextContainer vme, int y, int x )
{
	piITextContainer *me = (piITextContainer*)vme;

    const int numlines = me->mLines.GetLength();
    if( y>=numlines ) y=numlines-1;
    piILine *line = (piILine*)me->mLines.GetAddress(y);
    const int len = line->mText.GetLength();
    if( x>=len ) x=len;

    me->mCursor[0] = y;
    me->mCursor[1] = x;
    me->mCursor[2] = 0;
}

void piTextContanier_SetCursorInView( piTextContainer vme, int y, int x )
{
	piITextContainer *me = (piITextContainer*)vme;

    const int numlines = me->mLines.GetLength();
    if( y>=numlines ) y=numlines-1;
    piILine *line = (piILine*)me->mLines.GetAddress( y);
    const int len = line->mText.GetLength();
    if( x>=len ) x=len;

    me->mCursor[0] = y + me->mView.mStart;
    me->mCursor[1] = x;
    me->mCursor[2] = 0;
}


void piTextContanier_SetView( piTextContainer vme, int y )
{
	piITextContainer *me = (piITextContainer*)vme;

    const int numlines = me->mLines.GetLength();
    if( y>=numlines ) y=numlines-1;
	if( y<0 ) y=0;

    me->mView.mStart = y;
	if( me->mView.mStart<0 ) me->mView.mStart=0;
    me->mView.mStop = me->mView.mStart + me->mView.mLen;
}

void piTextContanier_ScrollView( piTextContainer vme, int dy )
{
	piITextContainer *me = (piITextContainer*)vme;

	int y = me->mView.mStart + dy;


    const int numlines = me->mLines.GetLength();
    if( y>=numlines ) y=numlines-1;
	if( y<0 ) y=0;

    me->mView.mStart = y;
	if( me->mView.mStart<0 ) me->mView.mStart=0;
    me->mView.mStop = me->mView.mStart + me->mView.mLen;
}


void piTextContainer_Destroy( piTextContainer vme )
{
	piITextContainer *me = (piITextContainer*)vme;

    iDestroyAllLines( me );

    me->mLines.End();
	free( vme );
}

bool piTextContainer_NewChar( piTextContainer vme, const wchar_t c )
{
	piITextContainer *me = (piITextContainer*)vme;

	if( c<32 ) return true;

    piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
	if( !line ) return false;

    const int res = line->mText.AppendCAt(c, me->mCursor[1]);
	if( !res ) return false;
	me->mCursor[1]++;
    me->mCursor[2] = me->mCursor[1];

	return true;
}
/*
unsigned int piTextContanier_GetCursor( piTextContainer vme )
{
	piITextContainer *me = (piITextContainer*)vme;
	return me->mCursor;
}*/

static int imin( int a, int b ) { return (a<b)?a:b; }

static void adjustCursorH( piITextContainer *me )
{
    piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
    const unsigned int len = line->mText.GetLength();
    if( me->mCursor[1]<me->mCursor[2] ) me->mCursor[1] = me->mCursor[2];
	if( me->mCursor[1]>len ) me->mCursor[1] = len;
}

static bool isLeter( wchar_t c )
{
    if( c>=L'a' && c<=L'z' ) return true;
    if( c>=L'A' && c<=L'Z' ) return true;
    if( c>=L'0' && c<=L'9' ) return true;
    if( c==L'.' ) return true;
    return false;
}

static bool isSeparator( wchar_t c )
{
    if( c==L' ' ) return true;
    if( c==L'(' ) return true;
    if( c==L')' ) return true;
    if( c==L',' ) return true;
    if( c==L'*' ) return true;
    if( c==L'/' ) return true;
    if( c==L'+' ) return true;
    if( c==L'-' ) return true;
    return false;
}

static int sortCursor( const unsigned int *a, const unsigned int *b )
{
    if( a[0]<b[0] ) return -1;
    if( a[0]>b[0] ) return +1;

    if( a[1]<b[1] ) return -1;
    if( a[1]>b[1] ) return +1;

    return 0;
}

static void iExpandSelection( piITextContainer* me, const unsigned int *cyx )
{
    if( sortCursor( cyx, me->mSelection.mStart )<0 ) 
    {
        me->mSelection.mStart[0] = cyx[0];
        me->mSelection.mStart[1] = cyx[1];
    }
    if( sortCursor( cyx, me->mSelection.mEnd )>0 ) 
    {
        me->mSelection.mEnd[0] = cyx[0];
        me->mSelection.mEnd[1] = cyx[1];
    }
}

bool piTextContainer_NewKeys( void *vme, const piTextContainerKey key, const bool isSelectionPressed )
{
	piITextContainer *me = (piITextContainer*)vme;

    const bool stopSelection  = !isSelectionPressed && me->mSelection.mEnabled;
    const bool startSelection = isSelectionPressed && !me->mSelection.mEnabled;
    const bool keepSelection  = isSelectionPressed && me->mSelection.mEnabled;
    const bool iS = me->mCursor[0]==me->mSelection.mStart[0] && me->mCursor[1]==me->mSelection.mStart[1];
    const bool iE = me->mCursor[0]==me->mSelection.mEnd[0]   && me->mCursor[1]==me->mSelection.mEnd[1];

	switch( key )
	{

        case KeyDelete:
        {
            piILine *cline = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
            const unsigned int len = cline->mText.GetLength();
            if( len==0 )
            {
                const unsigned int numlines = me->mLines.GetLength();
                if( me->mCursor[0]<(numlines-1) )
                {
                    // delete this line
                    piILine *nline = (piILine*)me->mLines.GetAddress(me->mCursor[0] + 1);
                    iDestroyLine( cline );
                    iCopyLine( cline, nline );
                    me->mCursor[1] = 0;
                    me->mCursor[2] = 0;
                    me->mLines.RemoveAndShift( me->mCursor[0] + 1);
                }
            }
            else if( me->mCursor[1]<len )
            {
                // delete this character
                piILine *line = (piILine*)me->mLines.GetAddress(me->mCursor[0]);
                int res = line->mText.RemoveCAt(me->mCursor[1]);
            }
            else if( me->mCursor[1]==len )
            {
                // merge with line bellow
                piILine *cline = (piILine*)me->mLines.GetAddress(me->mCursor[0]);
                piILine *nline = (piILine*)me->mLines.GetAddress( me->mCursor[0] + 1);

                cline->mText.AppendS(&nline->mText);

                iDestroyLine( nline );
                me->mLines.RemoveAndShift(me->mCursor[0] + 1);
            }

            return true;
        }

		case KeyEnter:
		{
            const unsigned int numlines = me->mLines.GetLength();
            piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
            const unsigned int len = line->mText.GetLength();
			piILine *nline;
			if( me->mCursor[0]==(numlines-1) )
                nline = (piILine*)me->mLines.Alloc(1, true);
			else
                nline = me->mLines.InsertAndShift( 0, me->mCursor[0] + 1, true);
            if( !nline )
                return false;

			const unsigned int nlen = len - me->mCursor[1];
            if (!nline->mText.Init((nlen == 0) ? 8 : nlen))
				return false;

			if( nlen>0 ) 
			{
                nline->mText.CopyW( line->mText.GetS() + me->mCursor[1]);
                nline->mText.SetLength( me->mCursor[1]);
			}

			me->mCursor[0]++;
			me->mCursor[1]=0;
            me->mCursor[2]=0;

			return true;
		}

		case KeyBack:
		{
		    if( me->mCursor[1]>0 )
			{
                piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
                int res = line->mText.RemoveCAt(me->mCursor[1] - 1);
				me->mCursor[1]--;
                me->mCursor[2] = me->mCursor[1];
			}
			else if( me->mCursor[0]>0 )
			{
                piILine *pline = (piILine*)me->mLines.GetAddress( me->mCursor[0] - 1);
                piILine *cline = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
                piILine *nline = (piILine*)me->mLines.GetAddress( me->mCursor[0] + 1);

                me->mCursor[1] = pline->mText.GetLength();
                pline->mText.AppendS(&cline->mText);

                iDestroyLine( cline );
                me->mLines.RemoveAndShift( me->mCursor[0]);

				me->mCursor[0]--;
			}
			return true;
		}

		case KeyBegining:
		{
			me->mCursor[1]=0;
			me->mCursor[2]=0;
			return true;
		}
		case KeyEnd:
		{
             const unsigned int numlines = me->mLines.GetLength();
            piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
            const unsigned int len = line->mText.GetLength();
			me->mCursor[1]=len;
            me->mCursor[2]=len;
			return true;
		}

		case KeyUp:
		{
		    if(me->mCursor[0]>0 )
			{
                if( me->mCursor[0] == me->mView.mStart )
                {
                    if( me->mView.mStart>0 ) { me->mView.mStart--; me->mView.mStop--; }
                }

                me->mCursor[0]--;
                adjustCursorH(me);
			}
			return true;
		}
		case KeyDown:
		{
            const unsigned int numlines = me->mLines.GetLength();
			if( me->mCursor[0]<(numlines-1) )
			{
                me->mCursor[0]++;
                adjustCursorH(me);
                if( me->mCursor[0] > me->mView.mStop ) { me->mView.mStart++; me->mView.mStop++; }
			}

			return true;
		}
        case KeyPageDown:
        {
            const int off = imin( me->mView.mStop - me->mView.mStart + 1, me->mView.mStart );
            me->mCursor[0] -= off;
            me->mView.mStart -= off;
            me->mView.mStop -= off;
            adjustCursorH(me);
            return true;
        }
        case KeyPageUp:
        {
             const unsigned int numlines = me->mLines.GetLength();
            const int off = imin( me->mView.mStop - me->mView.mStart + 1, numlines-me->mView.mStop-1 );
            me->mCursor[0] += off;
            me->mView.mStart += off;
            me->mView.mStop += off;
            adjustCursorH(me);
            return true;
        }
            
		case KeyLeft:
		{

			if(me->mCursor[1]>0 )
			{
				me->mCursor[1]--;
                me->mCursor[2]=me->mCursor[1];
			}
			else
			{
				if( me->mCursor[0]>0 )
				{
					me->mCursor[0]--;
                    piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
                    me->mCursor[1] = line->mText.GetLength();
                    me->mCursor[2] = me->mCursor[1];
				}
			}

            int where = 0;
            if( stopSelection )
            {
                me->mSelection.mEnabled = false;
            }
            else if( startSelection )
            {
                me->mSelection.mEnabled = true;
                me->mSelection.mStart[0] = me->mCursor[0];
                me->mSelection.mStart[1] = me->mCursor[1];
                me->mSelection.mEnd[0] = me->mCursor[0];
                me->mSelection.mEnd[1] = me->mCursor[1];
            }
            else if( keepSelection )
            {
                if( iS ) where = 1;
                if( iE ) where = 2;
                if( iS && iE ) where=3;
            }


            if( keepSelection )
            {
                if( iS )
                {
                    me->mSelection.mStart[0] = me->mCursor[0];
                    me->mSelection.mStart[1] = me->mCursor[1];
                }
                if( iE ) 
                {
                    me->mSelection.mEnd[0] = me->mCursor[0];
                    me->mSelection.mEnd[1] = me->mCursor[1];
                }


            }
			return true;
		}

		case KeyRight:
		{
            if( stopSelection )
            {
                me->mSelection.mEnabled = false;
            }
            else if( startSelection )
            {
                me->mSelection.mEnabled = true;
                me->mSelection.mStart[0] = me->mCursor[0];
                me->mSelection.mStart[1] = me->mCursor[1];
                me->mSelection.mEnd[0] = me->mCursor[0];
                me->mSelection.mEnd[1] = me->mCursor[1];
            }

            piILine *line = (piILine*)me->mLines.GetAddress(me->mCursor[0]);
            const unsigned int len = line->mText.GetLength();
			if( me->mCursor[1]<len )
			{
				me->mCursor[1]++;
                me->mCursor[2] = me->mCursor[1];
			}
			else
			{
                const unsigned int numlines = me->mLines.GetLength();
				if( me->mCursor[0]<(numlines-1) )
				{
					me->mCursor[0]++;
					me->mCursor[1] = 0;
                    me->mCursor[2] = me->mCursor[1];
                    if( me->mCursor[0] > me->mView.mStop ) { me->mView.mStart++; me->mView.mStop++; }
				}
			}
            if( keepSelection )
            {
                iExpandSelection( me, me->mCursor+0 );
            }
			return true;
		}

        case KeyTab:
        {
            piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
	        if( !line ) return false;

            for( int i=0; i<4; i++ )
            {
                const int res = line->mText.AppendCAt(32, me->mCursor[1]);
	            if( !res ) return false;
	            me->mCursor[1]++;
                me->mCursor[2] = me->mCursor[1];
            }
        }

        case KeyNextWord:
        {
            // find first line with content
            const piILine *line = (piILine*)me->mLines.GetAddress(me->mCursor[0]);
            if( !line ) return false;
            const int len = line->mText.GetLength();
            if( len==0 ) 
            {
                me->mCursor[0]++;
                adjustCursorH(me);
                if( me->mCursor[0] > me->mView.mStop ) { me->mView.mStart++; me->mView.mStop++; }
            }
            else
            {
                const wchar_t *wstr = line->mText.GetS();
                int found = -1;
                for( int i=me->mCursor[1]; i<len; i++ )
                {
                    if( isSeparator(wstr[i]) && isLeter(wstr[i+1]) )
                    { found=i+1; break; }
                }
                if( found>0 )
                {
                    me->mCursor[1] = found;
                    me->mCursor[2] = me->mCursor[1];
                }
                else
                {
                    me->mCursor[1] = len;
                    me->mCursor[2] = me->mCursor[1];
                }
            }
            return true;
        }

        case KeyPrevWord:
        {
            // find first line with content
            const piILine *line = (piILine*)me->mLines.GetAddress( me->mCursor[0]);
            if( !line ) return false;
            const int len = line->mText.GetLength();
            if( len==0 ) 
            {
		        if(me->mCursor[0]>0 )
			    {
                    if( me->mCursor[0] == me->mView.mStart )
                    {
                        if( me->mView.mStart>0 ) { me->mView.mStart--; me->mView.mStop--; }
                    }
                    me->mCursor[0]--;
                    adjustCursorH(me);
                }
            }
            else
            {
                const int len = line->mText.GetLength();
                const wchar_t *wstr = line->mText.GetS();
                int found = -1;
                for( int i=me->mCursor[1]-1; i>=1; i-- )
                {
                    if( isSeparator(wstr[i-1]) && isLeter(wstr[i]) )
                    { found=i; break; }
                }
                if( found>0 )
                {
                    me->mCursor[1] = found;
                    me->mCursor[2] = me->mCursor[1];
                }
                else
                {
                    me->mCursor[1] = 0;
                    me->mCursor[2] = me->mCursor[1];
                }
            return true;
            }
        }

	}

	return true;
}


//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
/*
#include "comix.inc"
#include "mac.inc"
#include "breeze.inc"
#include "hearst.inc"
#include "8x8thin.inc"

typedef struct
{
    int             cw, ch;
    unsigned char   *data;
}FONT_DESCR;

static const FONT_DESCR fontdescrs[] = {
    { 8, 16, comix    },
    { 8, 16, mac      },
    { 8, 16, breeze   },
    { 8, 16, hearst   },
    { 8, 16, _8x8thin },
};

typedef struct
{
    unsigned char *mTexture;
	unsigned int   mTextureXres;
	unsigned int   mTextureYres;
	unsigned int   mCurrentFont;
}piITextRenderer;


static bool piITextRenderer_CreateTexture( piITextRenderer *me )
{
	const int fontID = me->mCurrentFont;
	const unsigned char *ptr = fontdescrs[fontID].data; 

	const int cw = fontdescrs[fontID].cw;
	const int ch = fontdescrs[fontID].ch;

	const unsigned int xres = 128 * cw;
	const unsigned int yres =       ch;

	if( me->mTexture==0 || (xres*yres)>(me->mTextureXres*me->mTextureYres) )
	{
		unsigned char *tmp = (unsigned char*)malloc( xres*yres );
		if( !tmp ) return false;

		if( me->mTexture ) free( me->mTexture );
		me->mTexture = tmp;

	}
	me->mTextureXres = xres;
	me->mTextureYres = yres;

	for( int k=0; k<128; k++ )
	{
		for( int j=0; j<ch; j++ )
		{
			const unsigned char b = *ptr++;
			int bit = (1<<(cw-1));
			for( int i=0; i<cw; i++ )
			{
				if( b & bit ) me->mTexture[xres*j+i+k] = 0xff;
				else          me->mTexture[xres*j+i+k] = 0x00;
				bit >>= 1;
			}
		}
	}

	return true;
}

bool piITextRenderer_Init( piITextRenderer *me )
{
	me->mTexture = 0;
	me->mCurrentFont = 0;

	if( !piITextRenderer_CreateTexture( me ) )
		return false;

	return true;
}
*/


void piTextContainer_Render( piTextContainer vme, piTextContainerPrintLineFunc func, void *data )
{
	piITextContainer *me = (piITextContainer*)vme;

    const unsigned int numlines = me->mLines.GetLength();

    const unsigned int ia = me->mView.mStart;
    const unsigned int ib = imin( me->mView.mStop, numlines-1 );
	for( unsigned int i=ia; i<=ib; i++ )
	{
        const piILine *line = (piILine*)me->mLines.GetAddress( i);
        const int     len = line->mText.GetLength();
        const wchar_t *str = line->mText.GetS();

		unsigned int cursorpos[2] = { 0, 0 };
        if( i==me->mCursor[0] ) { cursorpos[0] = 1; cursorpos[1] = me->mCursor[1]; }
        
        unsigned int selectHasStartEnd[3] = { 0, 0, 0 };
        if( me->mSelection.mEnabled && (i>=me->mSelection.mStart[0]) && (i<=me->mSelection.mEnd[0]) )
        {   
            selectHasStartEnd[0] = 1;
            selectHasStartEnd[1] = 0;
            selectHasStartEnd[2] = len;
            if( i==me->mSelection.mStart[0] ) selectHasStartEnd[1] = me->mSelection.mStart[1];
            if( i==me->mSelection.mEnd[0]   ) selectHasStartEnd[2] = me->mSelection.mEnd[1];
        }

	    func( data, i-ia, i, str, len, cursorpos, selectHasStartEnd );
	}
}

int piTextContainer_GetLength( piTextContainer vme )
{
	piITextContainer *me = (piITextContainer*)vme;

    const unsigned int numlines = me->mLines.GetLength();
    int len = 0;
	for( unsigned int i=0; i<numlines; i++ )
	{
        piILine *line = (piILine*)me->mLines.GetAddress( i);
        len += line->mText.GetLength();
    }
    // add numlines so we can add returns
    // add 1 for \0
    return len + 2*numlines + 1;
}

bool piTextContainer_GetWText( piTextContainer vme, wchar_t *text )
{
	piITextContainer *me = (piITextContainer*)vme;

    const unsigned int numlines = me->mLines.GetLength();
	for( unsigned int i=0; i<numlines; i++ )
	{
        piILine *line = (piILine*)me->mLines.GetAddress(i);
        const int len = line->mText.GetLength();
        for( int j=0; j<len; j++ )
        {
            int c = line->mText.GetC(j);
            if( c<128 )
                *text++ = c;
        }
        *text++ = 0x0D;
        *text++ = 0x0A;
    }
    *text++ = 0;
    
    return true;
    return false;
}

bool piTextContainer_GetSText( piTextContainer vme, char *text )
{
	piITextContainer *me = (piITextContainer*)vme;

    const unsigned int numlines = me->mLines.GetLength();
	for( unsigned int i=0; i<numlines; i++ )
	{
        piILine *line = (piILine*)me->mLines.GetAddress( i);
        const int len = line->mText.GetLength();
        for( int j=0; j<len; j++ )
        {
            int c = line->mText.GetC(j);
            if( c<128 )
                *text++ = c;
        }
        *text++ = 0x0D;
        *text++ = 0x0A;
    }
    *text++ = 0;
    
    return true;
}


}