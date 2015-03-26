#include "CShellExt.h"
#include "Bitmap.h"
#include "resource.h"
#include <shellapi.h>
#include <algorithm>
#include <wchar.h>
#include <Strsafe.h>
#include <vector>

#ifndef RGBA
#define RGBA(r,g,b,a)        ((COLORREF)( (((DWORD)(BYTE)(a))<<24) |     RGB(r,g,b) ))
#endif

//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------
UINT _cRef = 0; // COM Reference count.
HINSTANCE _hModule = NULL; // DLL Module.

TCHAR szShellExtensionKey[] = TEXT("*\\shellex\\ContextMenuHandlers\\MiamPlayerShell");

#define szHelpTextA "Send to MiamPlayer"
#define szHelpTextW L"Send to MiamPlayer"

DWORD maxText = 25;

//Forward function declarations
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);

BOOL RegisterServer();
BOOL UnregisterServer();
void MsgBoxError(LPCTSTR lpszMsg);
void InvalidateIcon(HICON * iconSmall, HICON * iconLarge);

//Types
struct DOREGSTRUCT {
	HKEY	hRootKey;
	LPCTSTR	szSubKey;
	LPCTSTR	lpszValueName;
	DWORD	type;
	LPCTSTR	szData;
};

int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		_hModule = hInstance;
	}
	return TRUE;
}

STDAPI DllCanUnloadNow(void)
{
	return (_cRef == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
	*ppvOut = NULL;
	if (IsEqualIID(rclsid, CLSID_ShellExtension)) {
		CShellExtClassFactory *pcf = new CShellExtClassFactory;
		return pcf->QueryInterface(riid, ppvOut);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer(void)
{
	return (RegisterServer() ? S_OK : E_FAIL);
}


STDAPI DllUnregisterServer(void)
{
	return (UnregisterServer() ? S_OK : E_FAIL);
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR /*pszCmdLine*/)
{
	if (bInstall) {
		return S_OK;
	} else {
		return E_NOTIMPL;
	}
}

/**
 * \brief RegisterServer: Create registry entries and setup the shell extension
 * \return True if OK
 */
BOOL RegisterServer()
{
	DWORD    isActive = TRUE;
	DWORD    hasSubMenu = TRUE;
	DWORD    hasSendToCurrentPlaylist = TRUE;
	DWORD    hasSendToNewPlaylist = TRUE;
	DWORD    hasSendToTagEditor = TRUE;
	DWORD    hasAddToLibrary = TRUE;
	TCHAR    szSubKey[MAX_PATH];
	TCHAR    szModule[MAX_PATH];
	TCHAR    szDefaultPath[MAX_PATH];

	GetModuleFileName(_hModule, szDefaultPath, MAX_PATH);
	TCHAR* pDest = StrRChr(szDefaultPath, NULL, TEXT('\\'));
	pDest++;
	pDest[0] = 0;
	lstrcat(szDefaultPath, TEXT("MiamPlayer.exe"));

	auto miamShell = TEXT("Software\\MmeMiamMiam\\MiamPlayer\\MiamPlayerShell");

	//get this app's path and file name
	GetModuleFileName(_hModule, szModule, MAX_PATH);

	static DOREGSTRUCT ClsidEntries[] = {
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s"),					NULL,					REG_SZ,	TEXT("MiamPlayerShell")},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),	NULL,					REG_SZ,	szModule},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\InprocServer32"),	TEXT("ThreadingModel"),	REG_SZ,	TEXT("Apartment")},

		//Settings
		// Context menu
		{HKEY_CURRENT_USER,	miamShell,					TEXT("sendToCurrentPlaylist"),		REG_SZ,		TEXT("Send to current Playlist")},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("sendToNewPlaylist"),			REG_SZ,		TEXT("Send to new Playlist")},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("sendToTagEditor"),			REG_SZ,		TEXT("Send to Tag Editor")},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("addToLibrary"),				REG_SZ,		TEXT("Add to Library")},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("HasSendToCurrentPlaylist"),	REG_DWORD,	(LPTSTR)&hasSendToCurrentPlaylist},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("HasSendToNewPlaylist"),		REG_DWORD,	(LPTSTR)&hasSendToNewPlaylist},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("HasSendToTagEditor"),			REG_DWORD,	(LPTSTR)&hasSendToTagEditor},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("HasAddToLibrary"),			REG_DWORD,	(LPTSTR)&hasAddToLibrary},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),TEXT("Path"),						REG_SZ,		szDefaultPath},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("IsActive"),					REG_DWORD,	(LPTSTR)&isActive},
		{HKEY_CURRENT_USER,	miamShell,					TEXT("HasSubMenu"),					REG_DWORD,	(LPTSTR)&hasSubMenu},
		{HKEY_CLASSES_ROOT,	TEXT("CLSID\\%s\\Settings"),TEXT("Maxtext"),					REG_DWORD,	(LPTSTR)&maxText},

		//Registration
		// Context menu
		{HKEY_CLASSES_ROOT,	szShellExtensionKey, NULL, REG_SZ, szGUID},
		{NULL, NULL, NULL, REG_SZ, NULL}
	};

	// First clear any old entries
	UnregisterServer();

	// Register the CLSID entries
	for (int i = 0; ClsidEntries[i].hRootKey; i++) {
		wsprintf(szSubKey, ClsidEntries[i].szSubKey, szGUID);
		HKEY hKey;
		DWORD dwDisp;
		LRESULT lResult = RegCreateKeyEx(ClsidEntries[i].hRootKey, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
		if (NOERROR == lResult) {
			TCHAR szData[MAX_PATH];
			// If necessary, create the value string
			if (ClsidEntries[i].type == REG_SZ) {
				wsprintf(szData, ClsidEntries[i].szData, szModule);
				lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 0, ClsidEntries[i].type, (LPBYTE)szData, (lstrlen(szData) + 1) * sizeof(TCHAR));
			} else {
				lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 0, ClsidEntries[i].type, (LPBYTE)ClsidEntries[i].szData, sizeof(DWORD));
			}
			RegCloseKey(hKey);
		} else {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL UnregisterServer()
{
	TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];

	RegDeleteKey(HKEY_CLASSES_ROOT, szShellExtensionKey);

	wsprintf(szKeyTemp, TEXT("MiamPlayer_file\\shellex\\IconHandler"));
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("MiamPlayer_file\\shellex"));
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\InprocServer32"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);
	wsprintf(szKeyTemp, TEXT("CLSID\\%s"), szGUID);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyTemp);

	return TRUE;
}

void MsgBoxError(LPCTSTR lpszMsg)
{
	MessageBox(NULL, lpszMsg, TEXT("MiamPlayer Extension: Error"), MB_OK | MB_ICONWARNING);
}

CShellExtClassFactory::CShellExtClassFactory() :
	m_cRef(0L)
{
	_cRef++;
}

CShellExtClassFactory::~CShellExtClassFactory()
{
	_cRef--;
}

/** IUnknown methods */
STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
	*ppv = NULL;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
		*ppv = (LPCLASSFACTORY)this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
	if (--m_cRef)
		return m_cRef;
	delete this;
	return 0L;
}

/** IClassFactory methods */
STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
	*ppvObj = NULL;
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	CShellExt *pShellExt = new CShellExt();
	if (!pShellExt)
		return E_OUTOFMEMORY;
	return pShellExt->QueryInterface(riid, ppvObj);
}

STDMETHODIMP CShellExtClassFactory::LockServer(BOOL /*fLock*/)
{
	return NOERROR;
}

CShellExt::CShellExt() :
	m_cRef(0L),
	m_cbFiles(0),
	m_pDataObj(NULL),
	m_menuID(0),
	m_hMenu(NULL),
	m_isActive(true),
	m_hasSubMenu(true),
	m_hasSendToCurrentPlaylist(true),
	m_hasSendToNewPlaylist(true),
	m_hasSendToTagEditor(true),
	m_hasAddToLibrary(false),
	m_nameLength(0),
	m_nameMaxLength(maxText)
{
	ZeroMemory(&m_stgMedium, sizeof(m_stgMedium));
	_cRef++;

	GetModuleFileName(_hModule, m_szModule, MAX_PATH);

	OSVERSIONINFOEX inf;
	ZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	InitTheming();

	HKEY settingKey;
	LONG result;
	DWORD size = 0;
	DWORD siz = 0;

	result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\MmeMiamMiam\\MiamPlayer\\MiamPlayerShell"), 0, KEY_READ, &settingKey);
	if (result == ERROR_SUCCESS) {
		size = sizeof(TCHAR)*TITLE_SIZE;
		result = RegQueryValueEx(settingKey, TEXT("sendToCurrentPlaylist"), NULL, NULL, (LPBYTE)(m_szMenuSendToCurrentPlaylist), &size);
		if (result != ERROR_SUCCESS) {
			StringCchCopy(m_szMenuSendToCurrentPlaylist, TITLE_SIZE, TEXT("Send to current Playlist"));
		}

		result = RegQueryValueEx(settingKey, TEXT("sendToNewPlaylist"), NULL, NULL, (LPBYTE)(m_szMenuSendToNewPlaylist), &size);
		if (result != ERROR_SUCCESS) {
			StringCchCopy(m_szMenuSendToNewPlaylist, TITLE_SIZE, TEXT("Send to new Playlist"));
		}

		result = RegQueryValueEx(settingKey, TEXT("sendToTagEditor"), NULL, NULL, (LPBYTE)(m_szMenuSendToTagEditor), &size);
		if (result != ERROR_SUCCESS) {
			StringCchCopy(m_szMenuSendToTagEditor, TITLE_SIZE, TEXT("Send to Tag Editor"));
		}

		result = RegQueryValueEx(settingKey, TEXT("addToLibrary"), NULL, NULL, (LPBYTE)(m_szMenuAddToLibrary), &size);
		if (result != ERROR_SUCCESS) {
			StringCchCopy(m_szMenuAddToLibrary, TITLE_SIZE, TEXT("Add to Library"));
		}

		size = sizeof(DWORD);
		result = RegQueryValueEx(settingKey, TEXT("Maxtext"), NULL, NULL, (BYTE*)(&siz), &size);
		if (result == ERROR_SUCCESS) {
			m_nameMaxLength = std::max((DWORD)0, siz);
		}

		BOOL hasItemInContextMenu = TRUE;
		result = RegQueryValueEx(settingKey, TEXT("IsActive"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_isActive = (hasItemInContextMenu != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("HasSubMenu"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_hasSubMenu = (hasItemInContextMenu != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("HasSendToCurrentPlaylist"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_hasSendToCurrentPlaylist = (hasItemInContextMenu != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("HasSendToNewPlaylist"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_hasSendToNewPlaylist = (hasItemInContextMenu != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("HasSendToTagEditor"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_hasSendToTagEditor = (hasItemInContextMenu != 0);
		}

		result = RegQueryValueEx(settingKey, TEXT("HasAddToLibrary"), NULL, NULL, (BYTE*)(&hasItemInContextMenu), &size);
		if (result == ERROR_SUCCESS) {
			m_hasAddToLibrary = (hasItemInContextMenu != 0);
		}

		RegCloseKey(settingKey);
	}
}

CShellExt::~CShellExt()
{
	DeinitTheming();

	if (m_pDataObj)
		m_pDataObj->Release();
	_cRef--;
}

/** IUnknown methods */
STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
	*ppv = NULL;
	if (IsEqualIID(riid, IID_IUnknown)) {
		*ppv = (LPSHELLEXTINIT)this;
	} else if (IsEqualIID(riid, IID_IShellExtInit)) {
		*ppv = (LPSHELLEXTINIT)this;
	} else if (IsEqualIID(riid, IID_IContextMenu)) {
		*ppv = (LPCONTEXTMENU)this;
	} else if (IsEqualIID(riid, IID_IContextMenu2)) {
		*ppv = (LPCONTEXTMENU2)this;
	} else if (IsEqualIID(riid, IID_IContextMenu3)) {
		*ppv = (LPCONTEXTMENU3)this;
	} else if (IsEqualIID(riid, IID_IPersistFile)) {
		*ppv = (LPPERSISTFILE)this;
	} else if (IsEqualIID(riid, IID_IExtractIcon)) {
		*ppv = (LPEXTRACTICON)this;
	}
	if (*ppv) {
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{
	if (--m_cRef) {
		return m_cRef;
	}
	delete this;
	return 0L;
}

/** IShellExtInit methods */
STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST /*pIDFolder*/, LPDATAOBJECT pDataObj, HKEY /*hRegKey*/)
{
	if (m_pDataObj) {
		m_pDataObj->Release();
		m_pDataObj = NULL;
	}
	if (pDataObj) {
		m_pDataObj = pDataObj;
		pDataObj->AddRef();
	}
	return NOERROR;
}

/** IContextMenu methods */
STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT /*idCmdLast*/, UINT /*uFlags*/)
{
	// Context menu can be disabled with standard account if DLL has been registered with Admin rights after install /o/
	if (!m_isActive) {
		return SEVERITY_ERROR;
	}

	FORMATETC fmte = {
		CF_HDROP,
		(DVTARGETDEVICE FAR *)NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	HRESULT hres = m_pDataObj->GetData(&fmte, &m_stgMedium);

	if (SUCCEEDED(hres)) {
		if (m_stgMedium.hGlobal)
			m_cbFiles = DragQueryFile((HDROP)m_stgMedium.hGlobal, (UINT)-1, 0, 0);
	}

	UINT nIndex = indexMenu++;
	UINT uID = idCmdFirst;

	if (m_hasSubMenu) {
		HMENU hSubmenu = CreatePopupMenu();
		MENUITEMINFO menuItemInfo;
		ZeroMemory(&menuItemInfo, sizeof(menuItemInfo));
		int i = 0;
		if (m_hasSendToCurrentPlaylist) {
			InsertMenu(hSubmenu, i++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToCurrentPlaylist);
		}
		uID++;
		if (m_hasSendToNewPlaylist) {
			InsertMenu(hSubmenu, i++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToNewPlaylist);
		}
		uID++;
		if (m_hasSendToTagEditor) {
			InsertMenu(hSubmenu, i++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToTagEditor);
		}
		uID++;
		if (m_hasAddToLibrary) {
			InsertMenu(hSubmenu, i++, MF_STRING|MF_BYPOSITION, uID, m_szMenuAddToLibrary);
		}
		uID++;
		menuItemInfo.fType = MFT_STRING;
		menuItemInfo.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		menuItemInfo.wID = uID;
		menuItemInfo.hSubMenu = hSubmenu;

		wchar_t m[] = L"Miam-Player";
		// menuItemInfo.cch = 12;
		// wcscpy_s(menuItemInfo.dwTypeData, sizeof(m), m);
		menuItemInfo.dwTypeData = m;
		InsertMenuItem(hMenu, indexMenu, TRUE, &menuItemInfo);
	} else {
		if (m_hasSendToCurrentPlaylist) {
			InsertMenu(hMenu, nIndex++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToCurrentPlaylist);
		}
		uID++;
		if (m_hasSendToNewPlaylist) {
			InsertMenu(hMenu, nIndex++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToNewPlaylist);
		}
		uID++;
		if (m_hasSendToTagEditor) {
			InsertMenu(hMenu, nIndex++, MF_STRING|MF_BYPOSITION, uID, m_szMenuSendToTagEditor);
		}
		uID++;
		if (m_hasAddToLibrary) {
			InsertMenu(hMenu, nIndex++, MF_STRING|MF_BYPOSITION, uID, m_szMenuAddToLibrary);
		}
		uID++;
	}

	if (m_hasSubMenu) {
		HBITMAP icon = NULL;
		HICON hicon;
		DWORD menuIconWidth = GetSystemMetrics(SM_CXMENUCHECK);
		DWORD menuIconHeight = GetSystemMetrics(SM_CYMENUCHECK);
		HRESULT hr = LoadShellIcon(menuIconWidth, menuIconHeight, &hicon);
		if (SUCCEEDED(hr)) {
			icon = IconToBitmapPARGB32(hicon, menuIconWidth, menuIconHeight);
			DestroyIcon(hicon);
		}

		if (icon != NULL) {
			MENUITEMINFO mii;
			ZeroMemory(&mii, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_BITMAP;
			mii.hbmpItem = icon;

			SetMenuItemInfo(hMenu, ++nIndex, MF_BYPOSITION, &mii);

			//if (m_winVer >= WINVER_VISTA) {
			MENUINFO menuInfo;
			menuInfo.cbSize = sizeof(menuInfo);
			menuInfo.fMask = MIM_STYLE;
			menuInfo.dwStyle = MNS_CHECKORBMP;

			SetMenuInfo(hMenu, &menuInfo);
			//}
		}
	}
	m_hMenu = hMenu;
	m_menuID = uID;

	return ResultFromShort(uID - idCmdFirst);
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	if (!HIWORD(lpcmi->lpVerb)) {
		UINT idCmd = LOWORD(lpcmi->lpVerb);
		switch(idCmd) {
		case 0: {
			InvokeMiamPlayer(lpcmi->hwnd, lpcmi->lpDirectory, TEXT(" -f "), lpcmi->lpParameters, lpcmi->nShow);
			break;
		}
		case 1: {
			InvokeMiamPlayer(lpcmi->hwnd, lpcmi->lpDirectory, TEXT(" -n "), lpcmi->lpParameters, lpcmi->nShow);
			break;
		}
		case 2: {
			InvokeMiamPlayer(lpcmi->hwnd, lpcmi->lpDirectory, TEXT(" -t "), lpcmi->lpParameters, lpcmi->nShow);
			break;
		}
		case 3: {
			InvokeMiamPlayer(lpcmi->hwnd, lpcmi->lpDirectory, TEXT(" -l "), lpcmi->lpParameters, lpcmi->nShow);
			break;
		}
		default:
			break;
		}
	}
	return NOERROR;
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR, UINT uFlags, UINT FAR *, LPSTR pszName, UINT cchMax)
{
	LPWSTR wBuffer = (LPWSTR) pszName;
	if (uFlags == GCS_HELPTEXTA) {
		lstrcpynA(pszName, szHelpTextA, cchMax);
		return S_OK;
	} else if (uFlags == GCS_HELPTEXTW) {
		lstrcpynW(wBuffer, szHelpTextW, cchMax);
		return S_OK;
	}
	return E_NOTIMPL;
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, LRESULT *plResult)
{
	//Setup popup menu stuff (ownerdrawn)
	DWORD menuIconWidth = GetSystemMetrics(SM_CXMENUCHECK);
	DWORD menuIconHeight = GetSystemMetrics(SM_CYMENUCHECK);
	DWORD menuIconPadding = 2;	//+1 pixels on each side, is this fixed?

	switch(uMsg) {
	case WM_MEASUREITEM: {	//for owner drawn menu
		MEASUREITEMSTRUCT * lpdis = (MEASUREITEMSTRUCT*) lParam;

		if (lpdis == NULL)// || lpdis->itemID != m_menuID)
			break;
		lpdis->itemWidth = 0;	//0 seems to work for 98 and up
		if (lpdis->itemHeight < menuIconHeight)
			lpdis->itemHeight = menuIconHeight;
		if (plResult)
			*plResult = TRUE;
		break;
	}
	case WM_DRAWITEM: {		//for owner drawn menu
		//Assumes proper font already been set
		DRAWITEMSTRUCT * lpdis = (DRAWITEMSTRUCT*) lParam;
		if ((lpdis == NULL) || (lpdis->CtlType != ODT_MENU))
			break;
		HICON miamPlayerIcon = NULL;

		HRESULT hr = LoadShellIcon(menuIconWidth, menuIconHeight, &miamPlayerIcon);

		if (SUCCEEDED(hr)) {
			DrawIconEx(lpdis->hDC, menuIconPadding, menuIconPadding, miamPlayerIcon, menuIconWidth, menuIconHeight, 0, NULL, DI_NORMAL);
			DestroyIcon(miamPlayerIcon);
		}
		if (plResult)
			*plResult = TRUE;
		break;
	}
	default:
		break;
	}
	return S_OK;
}

// *** IPersistFile methods ***
HRESULT STDMETHODCALLTYPE CShellExt::Load(LPCOLESTR pszFileName, DWORD /*dwMode*/)
{
	LPTSTR file[MAX_PATH];
	StringCchCopy((LPWSTR)file, MAX_PATH, pszFileName);
	m_szFilePath[0] = 0;

	LPTSTR ext = PathFindExtension((LPTSTR)file);
	if (ext[0] == '.') {
		ext++;
	}
	int copySize = std::min(m_nameMaxLength + 1, MAX_PATH);	//+1 to take zero terminator in account
	StringCchCopy(m_szFilePath, copySize, ext);
	m_nameLength = lstrlen(m_szFilePath);
	CharUpperBuff(m_szFilePath, m_nameLength);
	return S_OK;
}

// *** IExtractIcon methods ***
STDMETHODIMP CShellExt::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
	*pwFlags = 0;
	if (uFlags & GIL_DEFAULTICON || m_szFilePath[0] == 0) {	//return regular MiamPlayer icon if requested OR the extension is bad OR static icon
		StringCchCopy(szIconFile, cchMax, m_szModule);
		*piIndex = 0;
		return S_OK;
	}

	if(cchMax > 0) {
		StringCchCopy(szIconFile, cchMax, TEXT("MiamPlayerShellIcon"));
		int len = lstrlen(szIconFile);
		StringCchCopy(szIconFile, cchMax-len, m_szFilePath);
	}
	*piIndex = 0;
	*pwFlags |= GIL_NOTFILENAME;//|GIL_DONTCACHE|GIL_PERINSTANCE;

	return S_OK;
}

STDMETHODIMP CShellExt::Extract(LPCTSTR /*pszFile*/, UINT /*nIconIndex*/, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize) {
	WORD sizeSmall = HIWORD(nIconSize);
	WORD sizeLarge = LOWORD(nIconSize);
	ICONINFO iconinfo;
	BOOL res;
	HRESULT hrSmall = S_OK, hrLarge = S_OK;

	if (phiconSmall)
		hrSmall = LoadShellIcon(sizeSmall, sizeSmall, phiconSmall);
	if (phiconLarge)
		hrLarge = LoadShellIcon(sizeLarge, sizeLarge, phiconLarge);

	if (FAILED(hrSmall) || FAILED(hrLarge)) {
		InvalidateIcon(phiconSmall, phiconLarge);
		return S_FALSE;
	}

	if (!phiconLarge || sizeLarge < 32)	//No modifications required
		return S_OK;

	HDC dcEditColor, dcEditMask, dcEditTemp;
	HFONT font;
	HBRUSH brush;
	HPEN pen;
	BITMAPINFO bmi;
	HBITMAP hbm;
	LPDWORD pPix;

	res = GetIconInfo(*phiconLarge, &iconinfo);
	if (!res)
		return S_OK;	//abort, the icon is still valid

	res = DestroyIcon(*phiconLarge);
	if (!res)
		return S_OK;
	else
		*phiconLarge = NULL;

	dcEditColor = CreateCompatibleDC(GetDC(0));
	dcEditMask = CreateCompatibleDC(GetDC(0));
	dcEditTemp = CreateCompatibleDC(GetDC(0));

	// Create temp bitmap to render rectangle to
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = sizeLarge;
	bmi.bmiHeader.biHeight = sizeLarge;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hbm = CreateDIBSection(dcEditTemp, &bmi, DIB_RGB_COLORS, (VOID**)&pPix, NULL, 0);
	memset(pPix, 0x00FFFFFF, sizeof(DWORD)*sizeLarge*sizeLarge);	//initialize to white pixels, no alpha

	SelectObject(dcEditColor, iconinfo.hbmColor);
	SelectObject(dcEditMask, iconinfo.hbmMask);
	SelectObject(dcEditTemp, hbm);

	LONG calSize = (LONG)(sizeLarge*2/5);

	LOGFONT lf = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}};
	lf.lfHeight = calSize;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, TEXT("Courier New"));
	RECT rectText = {0, 0, 0, 0};
	RECT rectBox = {0, 0, 0, 0};
	COLORREF backGround = RGB(1, 1, 60);
	COLORREF textColor = RGB(250,250,250);

	font = CreateFontIndirect(&lf);
	brush = CreateSolidBrush(backGround);
	pen = CreatePen(PS_NULL, 0, backGround);
	SelectObject(dcEditTemp, font);
	SelectObject(dcEditTemp, brush);
	SelectObject(dcEditTemp, pen);
	SetBkMode(dcEditTemp, TRANSPARENT);	//dont clear background when drawing text
	SetBkColor(dcEditTemp,  backGround);
	SetTextColor(dcEditTemp, textColor);

	//Calculate size of the displayed string
	SIZE stringSize;
	GetTextExtentPoint32(dcEditTemp, m_szFilePath, m_nameLength, &stringSize);
	stringSize.cx = std::min(stringSize.cx, (LONG)sizeLarge-2);
	stringSize.cy = std::min(stringSize.cy, (LONG)sizeLarge-2);

	rectText.top = sizeLarge - stringSize.cy - 1;
	rectText.left = sizeLarge - stringSize.cx - 1;
	rectText.bottom = sizeLarge - 1;
	rectText.right = sizeLarge - 1;

	rectBox.top = sizeLarge - stringSize.cy - 2;
	rectBox.left = sizeLarge - stringSize.cx - 2;
	rectBox.bottom = sizeLarge;
	rectBox.right = sizeLarge;

	//Draw the background (rounded) rectangle
	int elipsSize = calSize/3;
	RoundRect(dcEditTemp, rectBox.left, rectBox.top, rectBox.right, rectBox.bottom, elipsSize, elipsSize);
	//Draw text in the rectangle
	DrawText(dcEditTemp, m_szFilePath, m_nameLength, &rectText, DT_BOTTOM|DT_SINGLELINE|DT_LEFT);

	//set alpha of non white pixels back to 255
	//premultiply alpha
	//Fill in the mask bitmap (anything not 100% alpha is transparent)
	int red, green, blue, alpha;
	for(int y = 0; y < sizeLarge; y++) {
		for(int x = 0; x < sizeLarge; x++) {
			DWORD * pix = pPix+(y*sizeLarge+x);
			red = *pix & 0xFF;
			green = *pix >> 8 & 0xFF;
			blue = *pix >> 16 & 0xFF;
			alpha = *pix >> 24 & 0xFF;
			if ((*pix << 8) == 0xFFFFFF00)
				alpha = 0x00;
			else
				alpha = 0xFF;
			red = (red*alpha)/0xFF;
			green = (green*alpha)/0xFF;
			blue = (blue*alpha)/0xFF;
			*pix = RGBA(red, green, blue, alpha);
		}
	}

	BLENDFUNCTION ftn = { AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA };
	int width = rectBox.right - rectBox.left;
	int height = rectBox.bottom - rectBox.top;
	AlphaBlend(dcEditColor, rectBox.left, rectBox.top, stringSize.cx, stringSize.cy, dcEditTemp, rectBox.left, rectBox.top, width, height, ftn);

	//Adjust the mask image: simply draw the rectangle to it
	backGround = RGB(0, 0, 0);
	DeleteBrush(brush);
	DeletePen(pen);
	brush = CreateSolidBrush(backGround);
	pen = CreatePen(PS_NULL, 0, backGround);
	SelectObject(dcEditMask, brush);
	SelectObject(dcEditMask, pen);
	RoundRect(dcEditMask, rectBox.left, rectBox.top, rectBox.right, rectBox.bottom, elipsSize, elipsSize);

	DeleteDC(dcEditColor);
	DeleteDC(dcEditMask);
	DeleteDC(dcEditTemp);
	DeleteBrush(brush);
	DeletePen(pen);
	DeleteFont(font);
	DeleteBitmap(hbm);

	*phiconLarge = CreateIconIndirect(&iconinfo);
	DeleteBitmap(iconinfo.hbmColor);
	DeleteBitmap(iconinfo.hbmMask);

	if (*phiconLarge == NULL) {
		InvalidateIcon(phiconSmall, phiconLarge);
		return S_FALSE;
	}
	return S_OK;
}

void InvalidateIcon(HICON * iconSmall, HICON * iconLarge) {
	if (iconSmall && *iconSmall) {
		DestroyIcon(*iconSmall);
		*iconSmall = NULL;
	}
	if (iconLarge && *iconLarge) {
		DestroyIcon(*iconLarge);
		*iconLarge = NULL;
	}
}



bool sortFile(LPWSTR i, LPWSTR j) {
	return (StrCmpLogicalW(i, j) < 0);
}

// *** Private methods ***
void CShellExt::InvokeMiamPlayer(HWND /*hParent*/, LPCSTR /*pszWorkingDir*/, LPCWSTR pszCmd, LPCSTR /*pszParam*/, int iShowCmd) {
	TCHAR szMiamExecutableFilename[3 * MAX_PATH]; // Should be able to contain szFilename plus szCustom plus some additional characters.
	LPTSTR pszCommand;
	size_t bytesRequired = 1;

	memset(szMiamExecutableFilename, 0, sizeof(TCHAR) * 3 * MAX_PATH);

	TCHAR szKeyTemp[MAX_PATH + GUID_STRING_SIZE];
	DWORD regSize = 0;
	HKEY settingKey;
	LONG result;

	wsprintf(szKeyTemp, TEXT("CLSID\\%s\\Settings"), szGUID);
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyTemp, 0, KEY_READ, &settingKey);
	if (result != ERROR_SUCCESS) {
		return;
	}

	result = RegQueryValueEx(settingKey, TEXT("Path"), NULL, NULL, NULL, &regSize);
	if (result == ERROR_SUCCESS) {
		bytesRequired += regSize + 2;
	} else {
		RegCloseKey(settingKey);
		return;
	}

	for (UINT i = 0; i < m_cbFiles; i++) {
		bytesRequired += DragQueryFile((HDROP)m_stgMedium.hGlobal, i, NULL, 0);
		bytesRequired += 3;
	}

	bytesRequired *= sizeof(TCHAR);
	pszCommand = (LPTSTR)CoTaskMemAlloc(bytesRequired);
	if (!pszCommand) {
		RegCloseKey(settingKey);
		return;
	}
	*pszCommand = 0;

	regSize = (DWORD)MAX_PATH*sizeof(TCHAR);
	TCHAR szFilename[MAX_PATH];
	result = RegQueryValueEx(settingKey, TEXT("Path"), NULL, NULL, (LPBYTE)(szFilename), &regSize);
	szFilename[MAX_PATH-1] = 0;
	lstrcat(szMiamExecutableFilename, TEXT("\""));
	lstrcat(szMiamExecutableFilename, szFilename);
	lstrcat(szMiamExecutableFilename, TEXT("\""));
	RegCloseKey(settingKey);

	// We have to open the files in batches. A command on the command-line can be at most
	// 2048 characters in XP and 32768 characters in Win7. In the degenerate case where all
	// paths are of length MAX_PATH, we can open at most x files at once, where:
	// 260 * (x + 2) = 2048 or 32768 <=> x = 5 or x = 124.
	// Note the +2 to account for the path to MiamPlayer.exe.
	// http://stackoverflow.com/questions/3205027/maximum-length-of-command-line-string

	const UINT kiBatchSize = 100;

	UINT iFileIndex = 0;
	while (iFileIndex < m_cbFiles) {
		memset(pszCommand, 0, bytesRequired);
		StringCchCat(pszCommand, MAX_PATH, szMiamExecutableFilename);
		StringCchCat(pszCommand, MAX_PATH, pszCmd);

		std::vector<LPWSTR> files;
		for (UINT iBatchSizeCounter = 0; iFileIndex < m_cbFiles && iBatchSizeCounter < kiBatchSize; iBatchSizeCounter++) {
			LPWSTR file = new TCHAR[MAX_PATH];
			DragQueryFile((HDROP)m_stgMedium.hGlobal, iFileIndex, file, MAX_PATH);
			files.push_back(file);
			iFileIndex++;
		}
		std::sort(files.begin(), files.end(), sortFile);
		for (std::vector<LPWSTR>::iterator it = files.begin(); it != files.end(); ++it) {
			lstrcat(pszCommand, TEXT(" \""));
			lstrcat(pszCommand, *it);
			lstrcat(pszCommand, TEXT("\""));
		}

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = (WORD)iShowCmd;	//SW_RESTORE;
		if (!CreateProcess (NULL, pszCommand, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_ELEVATION_REQUIRED) {	//Fallback to shellexecute
				CoInitializeEx(NULL, 0);
				HINSTANCE execVal = ShellExecute(NULL, TEXT("runas"), pszCommand, NULL, NULL, iShowCmd);
				CoUninitialize();
				if (execVal <= (HINSTANCE)32) {
					TCHAR * message = new TCHAR[512+bytesRequired];
					wsprintf(message, TEXT("ShellExecute failed (%d): Is this command correct?\r\n%s"), execVal, pszCommand);
					delete [] message;
				}
			} else {
				TCHAR * message = new TCHAR[512+bytesRequired];
				wsprintf(message, TEXT("Error in CreateProcess (%d): Is this command correct?\r\n%s"), errorCode, pszCommand);
				MsgBoxError(message);
				delete [] message;
			}
		}
	}
	CoTaskMemFree(pszCommand);
}

STDMETHODIMP CShellExt::LoadShellIcon(int cx, int cy, HICON * phicon)
{
	HRESULT hr = E_OUTOFMEMORY;
	HICON hicon = NULL;

	//Either no custom defined, or failed and use fallback
	if (hicon == NULL) {
		hicon = (HICON)LoadImage(_hModule, MAKEINTRESOURCE(IDI_ICON_MIAMPLAYER), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
	}

	if (hicon == NULL) {
		hr = E_OUTOFMEMORY;
		*phicon = NULL;
	} else {
		hr = S_OK;
		*phicon = hicon;
	}
	return hr;
}
