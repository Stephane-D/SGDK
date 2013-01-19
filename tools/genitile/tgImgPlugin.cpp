///////////////////////////////////////////////////////////////////////////////
// tgImgPlugin.cpp
//  
//
// 
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "tgImgPlugin.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor

tgImgPlugin::tgImgPlugin(const char *filename):tgPlugin(filename)
{
	m_fctGetPictureData=NULL;
	m_fctGetPictureInfo=NULL;
	m_fctGetSupportedExt=NULL;
	
	if(m_Library!=NULL)
	{			
		m_fctGetPictureData=	(fctGetPictureData) 
								GetProcAddress((HMODULE)m_Library,"GetPictureData");
								
		m_fctGetPictureInfo=	(fctGetPictureInfo) 
								GetProcAddress((HMODULE)m_Library,"GetPictureInfo");
								
		m_fctGetSupportedExt=	(fctGetSupportedExt) 
								GetProcAddress((HMODULE)m_Library,"GetSupportedExt");								
		
	}

}

///////////////////////////////////////////////////////////////////////////////
// Destructor

tgImgPlugin::~tgImgPlugin()
{
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture info

int tgImgPlugin::GetPictureInfo(const char* pFileName,tgPictureInfo* pInfo)
{	
	if(m_Library!=NULL)
	{	return m_fctGetPictureInfo(pFileName,pInfo);}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture data

int tgImgPlugin::GetPictureData(uint8* Data,tgColor* Palette)
{
	if(m_Library!=NULL)
	{	return m_fctGetPictureData(Data,Palette);}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get the supported file extension

char* tgImgPlugin::GetSupportedExt(void)
{
	
	if(m_Library!=NULL)
	{	if(m_fctGetSupportedExt)
		{	return (m_fctGetSupportedExt());	}		
	}

	return NULL;

}

///////////////////////////////////////////////////////////////////////////////

