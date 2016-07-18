#ifndef _PIEDITBOX_H_
#define _PIEDITBOX_H_

typedef enum 
{
	KeyLeft = 0,
	KeyRight = 1,
	KeyUp = 2,
	KeyDown = 3,
	KeyBack = 4,
	KeyPageDown = 5,
	KeyPageUp = 6,
	KeyBegining = 7,
	KeyEnd = 8,
	KeyEnter = 9,
	KeyDelete = 10,
    KeyTab = 11,
    KeyNextWord = 12,
    KeyPrevWord = 13,
}piTextContainerKey;

typedef void * piTextContainer;

piTextContainer piTextContainer_Create( int windowLen );
void		    piTextContainer_Destroy( piTextContainer me );
bool		    piTextContainer_NewChar( piTextContainer me, const wchar_t c );
bool            piTextContainer_NewKeys( piTextContainer me, const piTextContainerKey  k, const bool s );
bool            piTextContainer_SetText( piTextContainer me, const wchar_t *str );
int             piTextContainer_GetLength( piTextContainer me );
bool            piTextContainer_GetWText( piTextContainer me, wchar_t *text );
bool            piTextContainer_GetSText( piTextContainer me, char *text );
void            piTextContanier_SetCursor( piTextContainer me, int y, int x );
void            piTextContanier_SetCursorInView( piTextContainer me, int y, int x );
void            piTextContanier_SetView( piTextContainer vme, int y );
void            piTextContanier_ScrollView( piTextContainer vme, int dy );

typedef void (*piTextContainerPrintLineFunc)( void *data, int line, int lineid, const wchar_t *str, const int len, const unsigned int *cursorInfo, const unsigned int *selectionInfo );
void            piTextContainer_Render( piTextContainer me, piTextContainerPrintLineFunc func, void *data );



#endif