///////////////////////////////////////////////////////////////////////////////
// tgPlugin.cpp
//
//
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "tgPlugin.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor

tgPlugin::tgPlugin(const char* filename)
{
	m_Library = LoadLibrary(filename);

	strcpy(m_FileName,filename);
	m_fctGetAuthor=NULL;
	m_fctGetDescription=NULL;
	m_fctGetVersion=NULL;
	m_fctGetContactInfo=NULL;

	GetSymbols();
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////

tgPlugin::~tgPlugin(void)
{
	if(m_Library!=NULL)
	{
		FreeLibrary((HMODULE)m_Library);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Get all the symbols from the plugins

void tgPlugin::GetSymbols(void)
{
	if(m_Library!=NULL)
	{
		m_fctGetDescription=	(fctGetDescription)
								GetProcAddress((HMODULE)m_Library,"GetDescription");

		m_fctGetContactInfo=	(fctGetContactInfo)
								GetProcAddress((HMODULE)m_Library,"GetContactInfo");

		m_fctGetAuthor=	(fctGetAuthor)
						GetProcAddress((HMODULE)m_Library,"GetAuthor");

		m_fctGetVersion=(fctGetVersion)
						GetProcAddress((HMODULE)m_Library,"GetBuildVersion");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Get Description of the plugins

char* tgPlugin::GetDescription(void)
{
	if(m_Library!=NULL)
	{
		if(m_fctGetDescription)
		{	return (m_fctGetDescription());	}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get Author of the plugins

char* tgPlugin::GetAuthor(void)
{
	if(m_Library!=NULL)
	{
		if(m_fctGetAuthor)
		{	return(m_fctGetAuthor());}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get version of the plugins

char* tgPlugin::GetVersion(void)
{
	if(m_Library!=NULL)
	{
		if(m_fctGetVersion)
		{	return (m_fctGetVersion());}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get contact address of the plugins

char* tgPlugin::GetContactInfo(void)
{
	if(m_Library!=NULL)
	{
		if(m_fctGetContactInfo)
		{	return (m_fctGetContactInfo());}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Get the absolute filename of the plugins

char* tgPlugin::GetFileName(void)
{
	return m_FileName;
}

///////////////////////////////////////////////////////////////////////////////
