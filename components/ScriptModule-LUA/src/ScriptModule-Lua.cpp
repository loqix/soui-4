﻿// luaScriptModule.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <event/SEventSlot.h>
#include "ScriptModule-Lua.h"
#include <lua_tinker.h>
#include <string/strcpcvt.h>

extern BOOL SOUI_Export_Lua(lua_State *L);


namespace SOUI
{
    wchar_t * cast_a2w(char * str)
    {
        return (wchar_t *)str;
    }

    int Utf8ToW(lua_State* L)
    {
        size_t n = 0;
        char* str = (char*)luaL_checklstring(L, -1, &n);
        if(!str)   return 0;
        SStringW strW=S_CA2W(str,CP_UTF8);
        lua_pushlstring(L, (const char*)(LPCWSTR)strW, 2*strW.GetLength()+2);
        return 1;
    }

    TCHAR * cast_a2t(char * str)
    {
        return (TCHAR *)str;
    }


    int Utf8ToT(lua_State *L)
    {
        size_t n = 0;
        char* str = (char*)luaL_checklstring(L, -1, &n);
        if(!str)   return 0;
        SStringT strT=S_CA2T(str,CP_UTF8);
        lua_pushlstring(L, (const char*)(LPCTSTR)strT, (strT.GetLength()+1)*sizeof(TCHAR));
        return 1;
    }

    class LuaFunctionSlot : public TObjRefImpl<IEvtSlot>
    {
    public:
        //! Slot function type.
        LuaFunctionSlot(lua_State *pLuaState,LPCSTR pszLuaFun) 
            : m_pLuaState(pLuaState)
            , m_luaFun(pszLuaFun)
        {}


		STDMETHOD_(BOOL,Run)(THIS_ IEvtArgs *pArg) OVERRIDE
		{
			return lua_tinker::call<bool>(m_pLuaState,m_luaFun,pArg);
		}
		STDMETHOD_(IEvtSlot*, Clone)(THIS) SCONST OVERRIDE
		{
			return new LuaFunctionSlot(m_pLuaState,m_luaFun);
		}
		STDMETHOD_(BOOL,Equal)(THIS_ const IEvtSlot * sour) SCONST OVERRIDE
		{
			if(sour->GetSlotType()!=GetSlotType()) return false;
			const LuaFunctionSlot *psour=static_cast<const LuaFunctionSlot*>(sour);
			SASSERT(psour);
			return psour->m_luaFun==m_luaFun && psour->m_pLuaState==m_pLuaState;
		}

		STDMETHOD_(UINT,GetSlotType)(THIS) SCONST OVERRIDE
		{
			return SLOT_USER+1;
		}

    private:
        SStringA m_luaFun;
        lua_State *m_pLuaState;
    };


    SScriptModule_Lua::SScriptModule_Lua()
    {
        d_state = luaL_newstate();
        if(d_state)
        {
            luaL_openlibs(d_state);
            SOUI_Export_Lua(d_state);
            lua_register(d_state, "A2W", Utf8ToW);
            lua_tinker::def(d_state, "cast_a2w", cast_a2w);
            luaL_dostring(d_state,"function L (str)\n return cast_a2w(A2W(str));\nend");//注册一个全局的"L"函数，用来将utf8编码的字符串转换为WCHAR

            lua_register(d_state, "A2T", Utf8ToT);
            lua_tinker::def(d_state, "cast_a2t", cast_a2t);
            luaL_dostring(d_state,"function T (str)\n return cast_a2t(A2T(str));\nend");//注册一个全局的"T"函数，用来将utf8编码的字符串转换为TCHAR
        }
    }


    SScriptModule_Lua::~SScriptModule_Lua()
    {
        if (d_state)
        {
            lua_close( d_state );
        }
    }


    LPCSTR SScriptModule_Lua::getIdentifierString() const
    {
        return "SOUI.Script.Lua5.1";
    }

    void SScriptModule_Lua::executeScriptFile( LPCSTR pszScriptFile )
    {
        lua_tinker::dofile(d_state,pszScriptFile);
    }


    void SScriptModule_Lua::executeScriptBuffer( const char* buff, size_t sz )
    {
        lua_tinker::dobuffer(d_state,buff,sz);
    }

    BOOL SScriptModule_Lua::executeScriptedEventHandler( LPCSTR handler_name, IEvtArgs *pArg)
    {
        LuaFunctionSlot luaFunSlot(d_state,handler_name);
        BOOL bRet =  luaFunSlot.Run(pArg);
		if(bRet) pArg->IncreaseHandleCount();
		return bRet;
    }

    void SScriptModule_Lua::executeString( LPCSTR str )
    {
        lua_tinker::dostring(d_state,str);
    }

    BOOL SScriptModule_Lua::subscribeEvent(IWindow* target, UINT uEvent, LPCSTR subscriber_name )
    {
		SWindow *pWnd = (SWindow*)target;
        return pWnd->GetEventSet()->subscribeEvent(uEvent,LuaFunctionSlot(d_state,subscriber_name));
    }

    BOOL SScriptModule_Lua::unsubscribeEvent(IWindow* target, UINT uEvent, LPCSTR subscriber_name )
    {
		SWindow *pWnd = (SWindow*)target;
        return pWnd->GetEventSet()->unsubscribeEvent(uEvent,LuaFunctionSlot(d_state,subscriber_name));
    }


    HRESULT SIScriptFactory::CreateScriptModule( IScriptModule ** ppScriptModule )
    {
        *ppScriptModule= new SOUI::SScriptModule_Lua;
        return S_OK;
    }

	namespace SCRIPT_LUA
	{
		BOOL SCreateInstance(IObjRef ** ppScript)
		{
			*ppScript= new SOUI::SIScriptFactory;
			return TRUE;
		}
	}

}

EXTERN_C BOOL Script_Lua_SCreateInstance(IObjRef ** ppScript)
{
	return SOUI::SCRIPT_LUA::SCreateInstance(ppScript);
}

//////////////////////////////////////////////////////////////////////////
