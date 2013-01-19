///////////////////////////////////////////////////////////////////////////////
// tgOutputPlugin.cpp
//  
//
// 
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "tgOutputPlugin.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor

tgOutputPlugin::tgOutputPlugin(const char *filename):tgPlugin(filename)
{
	m_fctGetID=NULL;
	m_fctOutputData=NULL;
	m_fctGetOutputType=NULL;
	m_fctOutputPicture=NULL;
	m_fctGetExt=NULL;

	if(m_Library!=NULL)
	{	m_fctGetID=	(fctGetID) GetProcAddress((HMODULE)m_Library,"GetID");								
		m_fctGetExt=(fctGetExt) GetProcAddress((HMODULE)m_Library,"GetExt");								
		m_fctOutputData=(fctOutputData)GetProcAddress((HMODULE)m_Library,"OutputData");					
		m_fctOutputPicture=(fctOutputPicture)GetProcAddress((HMODULE)m_Library,"OutputPicture");					
		m_fctGetOutputType=	(fctGetOutputType) GetProcAddress((HMODULE)m_Library,"GetOutputType");								
	}
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

tgOutputPlugin::~tgOutputPlugin()
{
}

///////////////////////////////////////////////////////////////////////////////
// Get ID

char* tgOutputPlugin::GetID(void)
{	
	if(m_Library!=NULL)
	{	if(m_fctGetID)
		{	return (m_fctGetID());	}		
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get extension

char* tgOutputPlugin::GetExt(void)
{	
	if(m_Library!=NULL)
	{	if(m_fctGetExt)
		{	return (m_fctGetExt());	}		
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// output the file name

int tgOutputPlugin::OutputData(const char *filename,const char *name,int type,uint8* data,int size)
{
	if(m_Library!=NULL)
	{	if(m_fctOutputData)
		{	return (m_fctOutputData(filename,name,type,data,size));	}		
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// output a picture

int tgOutputPlugin::OutputPicture(const char *filename,tgPictureInfo *info,uint8* pixels,tgColor *pal)
{
	if(m_Library!=NULL)
	{	if(m_fctOutputPicture)
		{	return (m_fctOutputPicture(filename,info,pixels,pal));	}		
	}
	return NULL;

}

///////////////////////////////////////////////////////////////////////////////
// Get the type of output

int tgOutputPlugin::GetOutputType(void)
{
	if(m_Library!=NULL)
	{	if(m_fctGetOutputType)
		{	return (m_fctGetOutputType());	}		
	}
	return NULL;

}

