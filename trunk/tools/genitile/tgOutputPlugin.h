///////////////////////////////////////////////////////////////////////////////
// tgImgPlugin
//
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __TGOUTPUTPLUGIN_H__
#define __TGOUTPUTPLUGIN_H__

///////////////////////////////////////////////////////////////////////////////
// Class definition
///////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "tgPlugin.h"
#include "mdttsdk.h"

class tgOutputPlugin:public tgPlugin
{
	///////////////////////////////////////////////////////////////////////////
	public:
	///////

	tgOutputPlugin(const char* filename);
	~tgOutputPlugin();

	char* GetID(void);
	int OutputData(const char *filename,const char *name,int type,uint8* data,int size);
	int GetOutputType(void);
	int OutputPicture(const char *filename,tgPictureInfo *info,uint8* pixels,tgColor *pal);
	char* GetExt(void);

	///////////////////////////////////////////////////////////////////////////
	protected:
	//////////

	typedef	char* (*fctGetID)(void);
	typedef	char* (*fctGetExt)(void);
	typedef int	(*fctGetOutputType)(void);
	typedef	int (*fctOutputData)(const char *filename,const char *name,int type,uint8* data,int size);
	typedef int (*fctOutputPicture)(const char *filename,tgPictureInfo *info,uint8* pixels,tgColor *pal);

	fctGetID			m_fctGetID;
	fctGetExt			m_fctGetExt;
	fctOutputData		m_fctOutputData;
	fctGetOutputType	m_fctGetOutputType;
	fctOutputPicture	m_fctOutputPicture;

	///////////////////////////////////////////////////////////////////////////
	private:
	///////
};

///////////////////////////////////////////////////////////////////////////////

#endif
