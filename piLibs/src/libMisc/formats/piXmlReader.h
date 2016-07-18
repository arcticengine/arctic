#ifndef _PIXMLREADER_H_
#define _PIXMLREADER_H_

typedef void * piXmlReader;
typedef void * piXmlElement;

piXmlReader piXmlReader_Open( const wchar_t *filename );
bool        piXmlReader_Parse( piXmlReader vme, wchar_t *error, int errorlen );
void        piXmlReader_Close( piXmlReader vme );

piXmlElement piXmlReader_GetFirstChildElement( piXmlReader vme );
piXmlElement piXmlElement_GetChild( piXmlElement vme );
piXmlElement piXmlElement_GetChild( piXmlElement vme, const char *tagname );
piXmlElement piXmlElement_GetNextElement( piXmlElement vme, const char *tagname );
piXmlElement piXmlElement_GetNextElement( piXmlElement vme );
const char * piXmlElement_GetName( piXmlElement vme );
bool         piXmlElement_GetAttributeUInt(piXmlElement vme, const char *name, unsigned int *value);
bool         piXmlElement_GetAttributeStr(piXmlElement vme, const char *name, char *value, int len);
bool         piXmlElement_GetAttributeFloat(piXmlElement vme, const char *name, float *value);

#endif

