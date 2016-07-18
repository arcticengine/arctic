#ifdef WINDOWS
#include <windows.h>
#include <wchar.h>
#endif

#include "piXmlReader.h"

#include "xml/tinyxml.h"

typedef struct
{
    TiXmlDocument *mDoc;
}piIXmlReader;

/*typedef struct
{
	int dummy;
}piIXmlElement;
*/
piXmlReader piXmlReader_Open( const wchar_t *filename )
{
	piIXmlReader *me = (piIXmlReader*)malloc( sizeof(piIXmlReader) );
	if( !me )
		return 0;

    #ifdef WINDOWS
	char tmp[1024];
	int len = (int)wcslen( filename );
	WideCharToMultiByte(CP_ACP,0,filename,len,tmp,1023,0,0);
	tmp[len] = 0;
    #endif


    me->mDoc = new TiXmlDocument( tmp );
	if( me->mDoc == NULL )
		return 0;

	return me;
}

bool piXmlReader_Parse( piXmlReader vme, wchar_t *error, int errorlen )
{
	piIXmlReader *me = (piIXmlReader*)vme;

    if (!me->mDoc->LoadFile(TIXML_ENCODING_UTF8))
    {
        if (error)
        {
            if (me->mDoc->Error())
            {
              //  *error = pData->doc.ErrorDesc();
            }
            else
            {
            //    *error = "Unkown error";
            }
        }
        return false;
    }
    return true;
}

void piXmlReader_Close( piXmlReader vme )
{
	piIXmlReader *me = (piIXmlReader*)vme;

	delete me->mDoc;
}


/// returns first tag of the xml file
piXmlElement piXmlReader_GetFirstChildElement( piXmlReader vme )
{
	piIXmlReader *me = (piIXmlReader*)vme;

	return me->mDoc->FirstChildElement();
}


/// get tag name    
const char * piXmlElement_GetName( piXmlElement vme )
{
	TiXmlElement *me = (TiXmlElement*)vme;
	return me->Value();
}

piXmlElement piXmlElement_GetChild( piXmlElement vme )
{
	TiXmlElement *me = (TiXmlElement*)vme;
    return me->FirstChildElement(); 
}

/// get next tag of the same level
piXmlElement piXmlElement_GetChild( piXmlElement vme, const char *tagname )
{
	TiXmlElement *me = (TiXmlElement*)vme;
	return me->FirstChildElement(tagname); 
}
piXmlElement piXmlElement_GetNextElement( piXmlElement vme, const char *tagname )
{
	TiXmlElement *me = (TiXmlElement*)vme;
	return me->NextSiblingElement(tagname);
}

piXmlElement piXmlElement_GetNextElement( piXmlElement vme )
{
	TiXmlElement *me = (TiXmlElement*)vme;
	return me->NextSiblingElement();
}

bool piXmlElement_GetAttributeUInt(piXmlElement vme, const char *name, unsigned int *value)
{
	TiXmlElement *me = (TiXmlElement*)vme;
	int val = 0;
	if (!(me->Attribute(name,&val)))
		return false;
	*value = (unsigned int)val;
    return true;
}

bool piXmlElement_GetAttributeStr(piXmlElement vme, const char *name, char *value, int len)
{
	TiXmlElement *me = (TiXmlElement*)vme;

    const char *val = me->Attribute(name);
    if (val==NULL)
        return false;
    strncpy(value,val,len);
    return true;
}

bool piXmlElement_GetAttributeFloat(piXmlElement vme, const char *name, float *value)
{
	TiXmlElement *me = (TiXmlElement*)vme;

    double dval = 0.0;
	if( !me->Attribute(name, &dval) )
        return false;
		
	*value = (float)dval;

	return true;
}


#if 0

/// saves the file
bool VRXmlFile::saveFile(const VRUniString &filename, VRString *error)
{
	VRString fn(filename, VR_ENCODING_UTF8);

	if (!pData->doc.SaveFile(fn.c_str()))
	{
        if (error)
        {
            if (pData->doc.Error())
            {
                *error = pData->doc.ErrorDesc();
            }
            else
            {
                *error = "Unkown error";
            }
        }
        return false;
	}
	return true;
}


VRXmlElement::operator bool()
{
    ASSERT(pData!=NULL);
    return pData->element != NULL;
}

VRXmlElement& VRXmlElement::operator=(const VRXmlElement &elem)
{
	if (this != &elem)
	{
		ASSERT(pData);
		ASSERT(pData->refcounter>0);
		pData->refcounter--;
		if (pData->refcounter==0)
			delete pData;
	        
		pData = elem.pData;
		if (pData)
			pData->refcounter++;
	}
    
    return *this;
}

/// VRUniString getName() const;
/// returns specified attribute of the tag : ex: '<tag attribute="Victor">'
bool VRXmlElement::attribute(const VRUniString &name, VRUniString *value)
{
    if(pData == NULL) return false;
	VRString str(name,VR_ENCODING_UTF8);
	if(pData->element == NULL) return false;
    const char *val = pData->element->Attribute(str);
    if (val==NULL)
        return false;
    *value = VRUniString(val,VR_ENCODING_UTF8);
    return true;
}

bool VRXmlElement::attribute(const VRUniString &name, bool *value)
{
    VRUniString val;

    ASSERT(pData!=NULL);

    if( !attribute(name,&val) )
        return false;

    *value = (val==L"true") || (val==L"True") || (val==L"TRUE");

    return true;
}

bool VRXmlElement::attribute(const VRUniString &name, int *value)
{
    ASSERT(pData!=NULL);
	VRString str(name,VR_ENCODING_UTF8);
	int val = 0;
    if (!pData->element->Attribute(str,&val))
		return false;
		
	*value = val;
	return true;
}

bool VRXmlElement::attribute(const VRUniString &name, unsigned int *value)
{
	int val = 0;
    ASSERT(pData!=NULL);
	VRString str(name,VR_ENCODING_UTF8);
	if (!(pData->element->Attribute(str,&val)))
		return false;
		
	*value = (unsigned int)val;
    return true;
}

bool VRXmlElement::attribute(const VRUniString &name, double *value)
{
    ASSERT(pData!=NULL);
    VRString str(name,VR_ENCODING_UTF8);
    double val = 0.0;
    if (!(pData->element->Attribute(str,&val)))
		return false;
		
	*value = val;
	return true;
}
bool VRXmlElement::attribute(const VRUniString &name, VRInt64 *value)
{
    ASSERT(pData!=NULL);
	VRString str(name,VR_ENCODING_UTF8);
	const char *res = pData->element->Attribute(str);
	if (res != NULL)
	{
		VRString val(res);
		*value = val.toInt64();
	}	
    return res != NULL;
}

/// Create a child element
VRXmlElement VRXmlElement::createChild(const VRUniString& name)
{
	VRXmlElement elm;
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	TiXmlElement xmlNode(name2.c_str());
	TiXmlNode *node = pData->element->InsertEndChild(xmlNode);
	elm.pData = new VRXmlElementData(node->ToElement());
	return elm;
}

/// Sets value
void VRXmlElement::setValue(const VRUniString& value)
{
    ASSERT(pData!=NULL);
	VRString val2(value.c_str(), VR_ENCODING_UTF8);
	pData->element->SetValue(val2.c_str());
}

/// Sets/inserts attributes
void VRXmlElement::setAttribute(const VRUniString& name, const VRUniString& value)
{
    ASSERT(pData!=NULL);
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	VRString val2(value.c_str(), VR_ENCODING_UTF8);
	pData->element->SetAttribute(name2.c_str(), val2.c_str());
}
void VRXmlElement::setAttribute(const VRUniString& name, const int value)
{
    ASSERT(pData!=NULL);
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	pData->element->SetAttribute(name2.c_str(), value);
}
void VRXmlElement::setAttribute(const VRUniString& name, const unsigned int value)
{
    ASSERT(pData!=NULL);
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	pData->element->SetAttribute(name2.c_str(), value);
}
void VRXmlElement::setAttribute(const VRUniString& name, const double value)
{
    ASSERT(pData!=NULL);
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	pData->element->SetDoubleAttribute(name2.c_str(), value);
}
void VRXmlElement::setAttribute(const VRUniString& name, const bool value)
{
    ASSERT(pData!=NULL);
	VRString name2(name.c_str(), VR_ENCODING_UTF8);
	pData->element->SetAttribute(name2.c_str(), value);
}
void VRXmlElement::setAttribute(const VRUniString& name, const VRInt64 value)
{
	/// That sucks: VRString does not have a constructor for VRInt64 yet,
	/// so we cannot convert the value to a string and store it as an attribute,
	/// and Tiny XML does not support int64.
    ASSERT(false);
}

/// get tag value : ex: '<tag> hello </tag>' returns "hello" as value
VRUniString VRXmlElement::value()
{
    ASSERT(pData!=NULL);
    return VRUniString(pData->element->FirstChild()->Value(),VR_ENCODING_UTF8);
}

/// get next tag of the same level
VRXmlElement VRXmlElement::nextElement()
{
    ASSERT(pData!=NULL);
    VRXmlElement next;
    TiXmlElement *elm = pData->element->NextSiblingElement();
    VRXmlElement element;
    element.pData = new VRXmlElementData(elm);
    return element;
}


///get first child tag
VRXmlElement VRXmlElement::nextElement(const VRUniString &tagname)
{
    ASSERT(pData!=NULL);
    VRXmlElement next;
    VRString str(tagname,VR_ENCODING_UTF8);
    TiXmlElement *elm = NULL;
	if (pData->element)
		elm = pData->element->NextSiblingElement(str.c_str());
    VRXmlElement element;
    element.pData = new VRXmlElementData(elm);
    return element;
}


#endif