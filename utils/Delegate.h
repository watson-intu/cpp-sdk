/**
* Copyright 2016 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#ifndef WDC_DELEGATE_H
#define WDC_DELEGATE_H

#include "WDCLib.h"

//! This delegate class allows you to store a function call to object. This is useful for passing a callback
//! around through functions or storing that callback as a data member in a class.
//!
//! Delegate<int> d = Delegate<int>::Create<MyObject,&MyObject::Func>( pObject );
//! 
//! d( 42 );		// invoke the delegate function
//!

template<typename ARG>
class Delegate
{
public:
	Delegate()
		: object_ptr(0)
		, stub_ptr(0)
	{}

	template <class T, void (T::*TMethod)(ARG)>
#ifndef _DEBUG
	static Delegate Create(T* object_ptr)
#else
	static Delegate Create(T* object_ptr, const char * a_pFile, int a_nLine )
#endif
	{
		Delegate d;
		if ( object_ptr != 0 )
		{
			d.object_ptr = object_ptr;
			d.stub_ptr = &method_stub<T, TMethod>; // #1
#ifdef _DEBUG
			d.m_pFile = a_pFile;
			d.m_nLine = a_nLine;
#endif
		}
		return d;
	}

	bool operator()(ARG a1) const
	{
		if (object_ptr != 0) {
			(*stub_ptr)(object_ptr, a1);
			return true;
		}

		return false;
	}

	void Reset()
	{
		object_ptr = 0;
		stub_ptr = 0;
	}

	bool IsValid() const
	{
		return object_ptr != 0 && stub_ptr != 0;
	}

	bool IsObject(void * obj) const
	{
		return obj == object_ptr;
	}

private:
	typedef void (*stub_type)(void* object_ptr, ARG);

	void* object_ptr;
	stub_type stub_ptr;
#ifdef _DEBUG
	const char * m_pFile;
	int m_nLine;
#endif

	template <class T, void (T::*TMethod)(ARG)>
	static void method_stub(void* object_ptr, ARG a1)
	{
		T* p = static_cast<T*>(object_ptr);
		return (p->*TMethod)(a1); // #2
	}
};

//! Helper macro for making a delegate a little bit less wordy, e.g.
//! Delegate<int> d = DELEGATE( int, MyObject, Func, pObject );
#ifndef _DEBUG
#define DELEGATE( CLASS, FUNC, ARG, OBJ )		Delegate<ARG>::Create<CLASS,&CLASS::FUNC>( OBJ )
#else
#define DELEGATE( CLASS, FUNC, ARG, OBJ )		Delegate<ARG>::Create<CLASS,&CLASS::FUNC>( OBJ, __FILE__, __LINE__ )
#endif

//! Delegate for a function that doesn't take any arguments.
class VoidDelegate
{
public:
	VoidDelegate()
		: object_ptr(0)
		, stub_ptr(0)
	{}

	template <class T, void (T::*TMethod)()>
#ifndef _DEBUG
	static VoidDelegate Create(T* object_ptr)
#else
	static VoidDelegate Create(T* object_ptr, const char * a_pFile, int a_nLine)
#endif
	{
		VoidDelegate d;
		if ( object_ptr != 0 )
		{
			d.object_ptr = object_ptr;
			d.stub_ptr = &method_stub<T, TMethod>; // #1
#ifdef _DEBUG
			d.m_pFile = a_pFile;
			d.m_nLine = a_nLine;
#endif
		}
		return d;
	}

	bool operator()() const
	{
		if ( object_ptr != 0 )
		{
			(*stub_ptr)(object_ptr);
			return true;
		}

		return false;
	}

	void Reset()
	{
		object_ptr = 0;
		stub_ptr = 0;
	}

	bool IsValid() const
	{
		return object_ptr != 0 && stub_ptr != 0;
	}

	bool IsObject(void * obj) const
	{
		return obj == object_ptr;
	}

private:
	typedef void (*stub_type)(void* object_ptr);

	void* object_ptr;
	stub_type stub_ptr;
#ifdef _DEBUG
	const char * m_pFile;
	int m_nLine;
#endif

	template <class T, void (T::*TMethod)()>
	static void method_stub(void* object_ptr)
	{
		T* p = static_cast<T*>(object_ptr);
		return (p->*TMethod)(); // #2
	}
};

//! Helper macro for making a delegate a little bit less wordy, e.g.
//! Delegate<int> d = DELEGATE( int, MyObject, Func, pObject );
#ifndef _DEBUG
#define VOID_DELEGATE( CLASS, FUNC, OBJ )		VoidDelegate::Create<CLASS,&CLASS::FUNC>( OBJ )
#else
#define VOID_DELEGATE( CLASS, FUNC, OBJ )		VoidDelegate::Create<CLASS,&CLASS::FUNC>( OBJ, __FILE__, __LINE__ )
#endif

#endif


