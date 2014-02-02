///////////////////////////////////////////////////////////////////////////////
// tgPluginsList
//
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __TGPLUGINSLIST_H__
#define __TGPlUGINSLIST_H__

///////////////////////////////////////////////////////////////////////////////
// Include

#include "windows.h"
#include "tgList.h"

///////////////////////////////////////////////////////////////////////////////
// Class definition

template<class T> class tgPluginsList
{
	///////////////////////////////////////////////////////////////////////////
	public:
	///////

	tgPluginsList(const char* path,const char* wildcard);
	~tgPluginsList();

	int GetCount(void);
	T* Item(int index);

	///////////////////////////////////////////////////////////////////////////
	protected:
	//////////

	char			m_Wildcard[256];
	char			m_Path[256];
	tgList<T>		*m_List;

	void Load(void);

	///////////////////////////////////////////////////////////////////////////
	private:
	////////
};

///////////////////////////////////////////////////////////////////////////////
// Constructor

template<class T>
tgPluginsList<T>::tgPluginsList(const char* path,
								const char* wildcard)
{

	strcpy(m_Path,path);
	strcat(m_Path,"\\");
	strcpy(m_Wildcard,wildcard);

	m_List=new tgList<T>;

	if(strlen(m_Path)>0 && strlen(m_Wildcard)>0)
		Load();
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

template<class T>
tgPluginsList<T>::~tgPluginsList(void)
{
	tgPlugin	*p;
	while( (p=m_List->PopBack())!=NULL)
	{	delete(p);}
}

///////////////////////////////////////////////////////////////////////////////
// Number of plugins

template<class T>
int tgPluginsList<T>::GetCount(void)
{
	return m_List->GetCount();
}

///////////////////////////////////////////////////////////////////////////////
// Load plugins

template<class T>
void tgPluginsList<T>::Load(void)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFirst;

	char path[256]={""};

	strcpy(path,m_Path);
	strcat(path,m_Wildcard);

	if ( (hFirst = FindFirstFile (path, &FindData)) != INVALID_HANDLE_VALUE)
	{
		do
		{	char pFile[256]={""};

			strcpy(pFile,m_Path);
			strcat(pFile,FindData.cFileName);

			m_List->PushBack(new T(pFile));


		}while ( FindNextFile (hFirst, &FindData));
	}

}

///////////////////////////////////////////////////////////////////////////////
// Return the object at that index

template<class T>
T* tgPluginsList<T>::Item(int index)
{
	return m_List->Item(index);
}

///////////////////////////////////////////////////////////////////////////////
//
#endif
