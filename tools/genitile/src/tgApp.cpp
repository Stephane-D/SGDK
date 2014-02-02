///////////////////////////////////////////////////////////////////////////////
// tgApp.cpp
//  
//
// 
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "tgApp.h"
#include "tgTools.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// defines
#define PLUGIN_IMG_WILCARD			"img_*.dll"
#define PLUGIN_OUT_WILCARD			"out_*.dll"

///////////////////////////////////////////////////////////////////////////////
// extern

///////////////////////////////////////////////////////////////////////////////
// Constructor

tgApp::tgApp(const char *plugins_path)
{
	strcpy(m_PluginsPath,plugins_path);
	
	m_ImgPlugins=NULL;
	m_OutputPlugins=NULL;

	m_ImgLoader=NULL;
	m_OutputWriter=NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

tgApp::~tgApp()
{
	if(m_ImgPlugins!=NULL)
		delete m_ImgPlugins;

	if(m_OutputPlugins!=NULL)
		delete m_OutputPlugins;
}

///////////////////////////////////////////////////////////////////////////////
// Load every plugins

int tgApp::LoadPlugins(char *argv)
{	// images
	int cnt=0;
		
	char Buffer[256]={""};
	char b[256]={""};
	GetModuleFileName(GetModuleHandle(NULL), Buffer, sizeof(Buffer));
	
	substring(b,Buffer,0,strlen(Buffer)-strlen("genitile.exe"));
	strcat(b,m_PluginsPath);

	m_ImgPlugins=new tgPluginsList<tgImgPlugin>(b,PLUGIN_IMG_WILCARD);
	m_OutputPlugins=new tgPluginsList<tgOutputPlugin>(b,PLUGIN_OUT_WILCARD);
	
	return m_ImgPlugins->GetCount();
}

///////////////////////////////////////////////////////////////////////////////
// check the file extension is supported

int tgApp::IsImgLoadingSupported(char *filename)
{	
	// get filename extension
	char ext[5]={""};	
	int pos=0;

	pos=strpos(filename,'.');
	if(pos!=-1)
	{	substring(ext,filename,pos);	
		
		// check in the plugins
		for(int i=0;i<m_ImgPlugins->GetCount();i++)
		{
			tgImgPlugin *plug=m_ImgPlugins->Item(i);

			if(plug!=NULL)
			{	char supported[256]={""};			
				char *t;			
				t=plug->GetSupportedExt();
				strcpy(supported,t);

				remove_car(supported,'.');
				remove_car(supported,'*');
				remove_car(supported,'|');
				replace_car(supported,';',',');						
				
				char exts[256]={""};				
				strcpy(exts,supported);

				pos=strpos(exts,',');
				while(pos!=-1)
				{	char ext2[5]={""};					
					if(pos>0)
					{	substring(ext2,exts,0,pos);
			
						// test extension
						char ext3[5]={"."};
						strcat(ext3,ext2);
						if(stricmp(strrchr(filename, '.'),ext3)==0)
						{	m_ImgLoader=plug;
							return 1;
						}	
						
						substring(exts,exts,pos+1);						
					}
					else
						substring(exts,exts,1);	

					pos=strpos(exts,',');
				}

				if(strlen(exts)>0)
				{	remove_car(exts,'.');
					remove_car(exts,'*');
					remove_car(exts,'|');
					remove_car(exts,',');
					remove_car(exts,';');

					char ext3[5]={"."};
					strcat(ext3,exts);

					if(stricmp(strrchr(filename, '.'),ext3)==0)
					{	m_ImgLoader=plug;
						return 1;
					}						
				}
			}			
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Check if the output specified is valid

int tgApp::IsOutputSupported(char *id)
{
	m_OutputWriter=NULL;
	for(int i=0;i<m_OutputPlugins->GetCount();i++)
	{
		tgOutputPlugin *plug=m_OutputPlugins->Item(i);
		if(plug!=NULL)
		{
			if(stricmp(plug->GetID(),id)==0)
			{	m_OutputWriter=plug;
				return 1;
			}		
		}
	}

	// use the first plugin as output
	if(m_OutputPlugins->GetCount()>0)
	{	m_OutputWriter=m_OutputPlugins->Item(0);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture information

int tgApp::GetPictureInfo(const char* pFileName,tgPictureInfo* pInfo)
{
	if(m_ImgLoader!=NULL)
	{	return m_ImgLoader->GetPictureInfo(pFileName,pInfo);}

	return tgERR_OPENINGFILE;
}

///////////////////////////////////////////////////////////////////////////////
// Get the picture data

int tgApp::GetPictureData(uint8* Data,tgColor* Palette)
{
	if(m_ImgLoader!=NULL)
	{	return m_ImgLoader->GetPictureData(Data,Palette);	
	}
	return tgERR_COPYDATA;
}

///////////////////////////////////////////////////////////////////////////////
// Get the output plugin

tgOutputPlugin* tgApp::GetOutputWriter(void)
{
	return m_OutputWriter;
}

