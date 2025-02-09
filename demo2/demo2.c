//从PE文件加载，注意从文件加载路径位置
#define RES_TYPE 0
//#define SYSRES_TYPE 0
// #define RES_TYPE 0   //PE
// #define RES_TYPE 1   //ZIP
// #define RES_TYPE 2   //7z
// #define RES_TYPE 2   //文件
//去掉多项支持，以免代码显得混乱
#if (RES_TYPE==1)
#include "resprovider-zip\zipresprovider-param.h"
#else 
#if (RES_TYPE==2)
#include "resprovider-7zip\zip7resprovider-param.h"
#endif
#endif
#ifdef _DEBUG
#define SYS_NAMED_RESOURCE _T("soui-sys-resourced.dll")
#else
#define SYS_NAMED_RESOURCE _T("soui-sys-resource.dll")
#endif


#include <core-def.h>
#include <com-def.h>


#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#include <windows.h>

#include <CommCtrl.h>
#include <Shlwapi.h>
#include <OleCtl.h>
#include <tchar.h>
#include <stdio.h>
#include <helper/SAssertFmt.h>
#include <utilities-def.h>
#include <utilities.h>


#include <core/SDefine.h>

#include <interface/SFactory-i.h>
#include <interface/SRender-i.h>
#include <interface/SWindow-i.h>
#include <interface/SAdapter-i.h>
#include <interface/SCtrl-i.h>

#include <event/SEvents.h>
#define INIT_R_DATA
#include "res/resource.h"

#include "ComLoader.h"
#include "TestLvAdapter.h"

#define INITGUID
#include <Guiddef.h>
DEFINE_GUID(IID_IListView,0xE584B16A,0x6BDB,0x4afb,0x8B,0xCC,0x3A,0x1A,0xBA,0xCD,0x2F,0xE2);

ISouiFactory * s_souiFac =NULL;

//debug时方便调试设置当前目录以便从文件加载资源
void SetDefaultDir()
{
	TCHAR szCurrentDir[MAX_PATH] = { 0 };
	LPTSTR lpInsertPos;
	GetModuleFileName(NULL, szCurrentDir, sizeof(szCurrentDir));

	lpInsertPos = _tcsrchr(szCurrentDir, _T('\\'));
#ifdef _DEBUG
	_tcscpy(lpInsertPos + 1, _T("..\\demo2"));
#else
	_tcscpy(lpInsertPos + 1, _T("\0"));
#endif
	SetCurrentDirectory(szCurrentDir);
}
BOOL OnBtnClose(IEvtArgs *evt,void *Ctx)
{
	IHostWnd *pHostWnd = (IHostWnd*)Ctx;
	HWND hwnd = pHostWnd->lpVtbl->GetHwnd(pHostWnd);
	FlashWindow(hwnd,TRUE);
	return TRUE;
}

BOOL OnBtnEndDialog(IEvtArgs *evt,void *Ctx)
{
	IHostDialog *hostWnd = (IHostDialog *)Ctx;
	hostWnd->lpVtbl->EndDialog(hostWnd,IDOK);
	return TRUE;
}

BOOL OnHostMsg(const LPMSG pMsg,LRESULT *pRes,void *Ctx)
{
	if(pMsg->message != WM_INITDIALOG)
	{
		return FALSE;
	}
	{
		IEvtSlot *pSlot;
		IHostDialog *hostWnd = (IHostDialog *)Ctx;
		IWindow* root = hostWnd->lpVtbl->GetIRoot(hostWnd);
		IWindow* btnClose= root->lpVtbl->FindIChildByID(root,R.id.btn_close,-1);
		s_souiFac->lpVtbl->CreateFuncSlot(s_souiFac,OnBtnEndDialog,hostWnd,&pSlot);
		btnClose->lpVtbl->SubscribeEvent(btnClose,EVT_CMD,pSlot);
		pSlot->lpVtbl->Release(pSlot);
	}

	return TRUE;
}


BOOL LoadRenderFac(IRenderFactory ** ppRenderFac){
	BOOL bLoaded = TRUE;
	IImgDecoderFactory *pImgDecoderFactory;
	do{
		//使用GDI渲染界面
		bLoaded = LoadComObj(Render_Gdi,(IObjRef * *)ppRenderFac);
		SASSERT(bLoaded);
		if(!bLoaded) break;
		//设置图像解码引擎。默认为GDIP。基本主流图片都能解码。系统自带，无需其它库
		bLoaded = LoadComObj(Decoder_Gdip,(IObjRef * *)&pImgDecoderFactory);
		SASSERT(bLoaded);
		if(!bLoaded) break;

		(*ppRenderFac)->lpVtbl->SetImgDecoderFactory(*ppRenderFac,pImgDecoderFactory);
		pImgDecoderFactory->lpVtbl->Release(pImgDecoderFactory);
	}while(FALSE);
	return bLoaded;
}

//加载系统资源
BOOL LoadSystemRes(IApplication *pApp,ISouiFactory *pSourFac)
{
	HMODULE hModSysResource = LoadLibrary(SYS_NAMED_RESOURCE);
	if (hModSysResource)
	{
		IResProvider * sysResProvider;
		pSourFac->lpVtbl->CreateResProvider(pSourFac,RES_PE, (IObjRef * *)& sysResProvider);
		sysResProvider->lpVtbl->Init(sysResProvider,(WPARAM)hModSysResource, 0);
		pApp->lpVtbl->LoadSystemNamedResource(pApp,sysResProvider);
		sysResProvider->lpVtbl->Release(sysResProvider);
		FreeLibrary(hModSysResource);
		return TRUE;
	}
	else
	{
		SASSERT(FALSE);
		return FALSE;
	}
}


//加载用户资源
BOOL LoadUserRes(IApplication *pApp,ISouiFactory *pSourFac)
{
	IResProvider*   pResProvider;
	BOOL bLoaded = FALSE;
	do{
#ifdef _DEBUG		
		//选择了仅在Release版本打包资源则在DEBUG下始终使用文件加载
		{
			pSourFac->lpVtbl->CreateResProvider(pSourFac,RES_FILE, (IObjRef * *)& pResProvider);
			bLoaded = pResProvider->lpVtbl->Init(pResProvider,(LPARAM)_T("uires"), 0);
			SASSERT(bLoaded);
			if(!bLoaded) break;
		}
#else
		{
			pSourFac->lpVtbl->CreateResProvider(pSourFac,RES_PE, (IObjRef * *)& pResProvider);
			bLoaded = pResProvider->lpVtbl->Init(pResProvider,(WPARAM)pApp->lpVtbl->GetModule(pApp), 0);
			SASSERT(bLoaded);
			if(!bLoaded) break;
		}
#endif
		pApp->lpVtbl->InitXmlNamedID(pApp,(const LPCWSTR*)&R.name, (const int*)&R.id,sizeof(R.id)/sizeof(int));
		{
			IResProviderMgr *pResMgr = pApp->lpVtbl->GetResProviderMgr(pApp);
			pResMgr->lpVtbl->AddResProvider(pResMgr,pResProvider,_T("uidef:xml_init"));
		}
	}while(FALSE);
	return bLoaded;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
	int nRet = 0;
	HRESULT hRes = OleInitialize(NULL);
	SASSERT(SUCCEEDED(hRes));
	{
		ISouiFactory * souiFac =NULL;
		IApplication * theApp = NULL;
		IRenderFactory * pRenderFac = NULL;
		CreateSouiFactory((IObjRef**)&souiFac);

		s_souiFac = souiFac;

		SetDefaultDir();

		if(!LoadRenderFac(&pRenderFac))
			return -1;
		souiFac->lpVtbl->CreateApp(souiFac,&theApp,pRenderFac,hInstance,L"test",FALSE);
		if(!LoadSystemRes(theApp,souiFac))
			return -2;
		if(!LoadUserRes(theApp,souiFac))
			return -3;

		{
			//使用接口方式创建HostWnd。
			IHostWnd * hostWnd;
			IHostDialog* hostDialog;
			souiFac->lpVtbl->CreateHostWnd(souiFac,&hostWnd,UIRES.LAYOUT.XML_MAINWND);
			souiFac->lpVtbl->CreateHostDialog(souiFac,&hostDialog,UIRES.LAYOUT.DLG_TEST);

			hostWnd->lpVtbl->Create(hostWnd,NULL,0,0,0,0);
			{
				IListView *pLv =NULL;
				IWindow* root = hostWnd->lpVtbl->GetIRoot(hostWnd);
				IWindow *pLvTest = root->lpVtbl->FindIChildByName(root,L"lv_test",-1);
				if(S_OK==pLvTest->lpVtbl->QueryInterface(pLvTest,&IID_IListView,(IObjRef**)&pLv))
				{
					ILvAdapter *pAdapter=NULL;
					TestLvAdapter_Init(&pAdapter);
					pLv->lpVtbl->SetAdapter(pLv,pAdapter);
					pAdapter->lpVtbl->Release(pAdapter);

					pLv->lpVtbl->Release(pLv);
				}
			}
			hostWnd->lpVtbl->ShowWindow(hostWnd,SW_SHOWNORMAL);

			//使用事件订阅方式响应界面控件事件。
			{
				IEvtSlot *pSlot=NULL;
				IWindow* root = hostWnd->lpVtbl->GetIRoot(hostWnd);
				IWindow* btnClose = root->lpVtbl->FindIChildByID(root,R.id.btn_exit,-1);
				souiFac->lpVtbl->CreateFuncSlot(souiFac,OnBtnClose,hostWnd,&pSlot);
				btnClose->lpVtbl->SubscribeEvent(btnClose,EVT_CMD,pSlot);
				pSlot->lpVtbl->Release(pSlot);
			}

			hostDialog->lpVtbl->SetMsgHandler(hostDialog,OnHostMsg,hostDialog);
			//hostDialog->lpVtbl->DoModal(hostDialog,NULL);

			nRet = theApp->lpVtbl->Run(theApp,hostWnd->lpVtbl->GetHwnd(hostWnd));

			hostWnd->lpVtbl->Release(hostWnd);
			hostDialog->lpVtbl->Release(hostDialog);
		}
		theApp->lpVtbl->Release(theApp);
		souiFac->lpVtbl->Release(souiFac);
		
		s_souiFac = NULL;
	}
	OleUninitialize();
	return nRet;
}
