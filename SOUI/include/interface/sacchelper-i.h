#pragma once
#include <utilities-def.h>

SNSBEGIN

interface IWindow;

#undef INTERFACE
#define INTERFACE IObjRef
DECLARE_INTERFACE_IID_(IAccHelper, IUnknown, "9FAF3067-6723-40DA-A56B-CDCB11B8902B")
{
    STDMETHOD_(HRESULT, QueryInterface)
    (THIS_
         /* [in] */ REFIID riid,
     /* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD_(void, SetOwner)(THIS_ IWindow * pOwner) PURE;
    STDMETHOD_(IWindow *, GetOwner)(THIS) SCONST PURE;
};

SNSEND