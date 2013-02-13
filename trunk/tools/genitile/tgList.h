///////////////////////////////////////////////////////////////////////////////
// tgList
//
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __TGLIST__
#define	__TGLIST__

//////////////////////////////////////////////////////////////////////////////
// Class definition : tgNODE
//////////////////////////////////////////////////////////////////////////////

template<class T>
class tgListNode
{
	public:
	T*	object;
	int index;

	tgListNode<T> *prev,*next;
	tgListNode();
};

//////////////////////////////////////////////////////////////////////////////
// Class definition : tgLIST
///////////////////////////////////////////////////////////////////////////////

template <class T>
class tgList
{
	///////////////////////////////////////////////////////////////////////////
	public:
	///////

	int	Id;
	tgList();
	~tgList();

	int PushBack(T*);
	int Insert(int,T*);
	T* PopBack(void);
	T* Remove(tgListNode<T>*);
	T* Remove(int);

	tgListNode<T>*  MoveFirst(void);
	tgListNode<T>*  MoveNext(tgListNode<T>*);
	tgListNode<T>*  MoveNext(tgListNode<T>*,int);
	tgListNode<T>*  MoveLast(void);
	tgListNode<T>*  MovePrevious(tgListNode<T>*);
	tgListNode<T>*	GetNode(int index);

	T*	Item(int index);
	int	GetCount(void);

	///////////////////////////////////////////////////////////////////////////
	private:
	////////

	void RecomputeNodeIndex(tgListNode<T>*,int);
	tgListNode<T> head,tail;
	tgListNode<T>* last_node;
	int Count;
};

///////////////////////////////////////////////////////////////////////////////
// Constructor tgListNode

template<class T> tgListNode<T>::tgListNode()
{
;
}

///////////////////////////////////////////////////////////////////////////////
// Contructor

template<class T> tgList<T>::tgList()
{
	Id=0;
	Count = 0;

	head.index=-1;
	tail.index=-2;
	last_node=NULL;

	head.prev=NULL;
	head.next=&tail;

	tail.next=NULL;
	tail.prev=&head;
}

///////////////////////////////////////////////////////////////////////////////
// Destructor

template<class T> tgList<T>::~tgList(void)
{

	while(PopBack()!=NULL)
		;
}


///////////////////////////////////////////////////////////////////////////////
// Push an object on the stack

template<class T> int tgList<T>::PushBack(T* obj)
{
	int Index = Id;
	tgListNode<T>	*node;

	if(obj!=NULL)
	{
		node=new tgListNode<T>;

		node->next=&tail;
		node->prev=tail.prev;
		node->object=obj;
		node->index = Count;

		Id ++;
		Count ++;

		(tail.prev)->next=node;
		tail.prev=node;
	}

	return Index;
}

///////////////////////////////////////////////////////////////////////////////
// Insert object

template<class T> int tgList<T>::Insert(int index,T* obj)
{
	tgListNode<T>	*new_node;
	tgListNode<T>	*node=GetNode(index);

	if(node!=NULL)
	{
		new_node=new tgListNode<T>;

		new_node->next=node;
		new_node->prev=node->prev;
		new_node->object=obj;
		new_node->index = index;

		node->prev = new_node;
		(new_node->prev)->next = new_node;

		RecomputeNodeIndex(node,index+1);

		Count ++;

		return index;
	}
	else
	{
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Pop a object

template<class T> T* tgList<T>::PopBack(void)
{
	tgListNode<T>	*node=tail.prev;
	T* obj;

	if(node!=&head)
	{
		tail.prev = node->prev;
		(node->prev)->next = &tail;
		obj = node->object;
		delete(node);

		Count--;

		return obj;
	}
	else
	{
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Move first

template<class T> tgListNode<T>*  tgList<T>::MoveFirst(void)
{
	if(head.next!=&tail)
		return (tgListNode<T>*) head.next;
	else
		return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Move next

template<class T> tgListNode<T>*  tgList<T>::MoveNext(tgListNode<T>* node)
{
	if(node->next!= &tail)
		return (tgListNode<T>*) node->next;
	else
		return NULL;
}

template<class T> tgListNode<T>* tgList<T>::MoveNext(tgListNode<T>* node,int i)
{
	while(node->next!= &tail && i>0)
	{	node=node->next;
	}
	return node;
}

///////////////////////////////////////////////////////////////////////////////
// Move last

template<class T> tgListNode<T>*  tgList<T>::MoveLast(void)
{
	if(tail.prev!=&head)
		return (tgListNode<T>*) tail.prev;
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Move previous

template<class T> tgListNode<T>*  tgList<T>::MovePrevious(tgListNode<T>* node)
{
	if(node->next!= &tail)
		return (tgListNode<T>*) node->next;
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Remove node

template<class T> T*  tgList<T>::Remove(tgListNode<T>* node)
{
	T* obj=NULL;

	if(node!=NULL)
		obj = node->object;
	else
	{	return NULL;}
	Count--;

	RecomputeNodeIndex( (node->next),node->index);

	(node->prev)->next = node->next;
	(node->next)->prev = node->prev;
	node->next=NULL;
	node->prev=NULL;

	delete(node);

	return obj;
}

template<class T> T*  tgList<T>::Remove(int index)
{
	T* obj=NULL;
	tgListNode<T>* node = GetNode(index);

	return Remove(node);
}

///////////////////////////////////////////////////////////////////////////////
// Recompute index

template<class T> void  tgList<T>::RecomputeNodeIndex(tgListNode<T>* node,int start_index)
{
	tgListNode<T>* ptr=node;

	while(ptr!=NULL)
	{
		ptr->index = start_index++;
		ptr=MoveNext(ptr);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Get The item

template<class T> T*  tgList<T>::Item(int index)
{
	tgListNode<T>* ptr=GetNode(index);
	if(ptr==NULL)
		return NULL;
	else
		return ptr->object;
}

///////////////////////////////////////////////////////////////////////////////
// Get the node

template<class T> tgListNode<T>*  tgList<T>::GetNode(int index)
{
	int loop=1;
	tgListNode<T>* ptr=NULL;

	if(last_node!=NULL)
	{	if(last_node->index<index)
			ptr=last_node;
		else
			ptr=MoveFirst();
	}
	else
	{	ptr=MoveFirst();}

	if(index>(Count-1))
		return NULL;

	while(ptr!=NULL && loop==1)
	{
		if(ptr->index==index)
		{
			loop=0;
			last_node=ptr;
			return ptr;
		}
		else
			ptr = MoveNext(ptr);
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Count

template<class T> int tgList<T>::GetCount(void)
{
	return Count;
}


#endif
