//---------------------------------------------------------------------------
// Copyright 2002-2003 by Andre Burgaud <andre@burgaud.com>
// Copyright 2009 by Harry <harrybharry@users.sourceforge.net>
// See license.txt
//---------------------------------------------------------------------------

#ifndef STRICT
#define STRICT
#endif

#define INC_OLE2
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>

#define WINVER_VISTA 0x600
#define WINVER_XP	 0x0501

//This is not ideal, but missing from current mingw
#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

#define GIL_DEFAULTICON 0x0040

#define GUID_SIZE			128
#define GUID_STRING_SIZE	40
#define TITLE_SIZE			64

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define ARRAYSIZE(a)		(sizeof(a)/sizeof(a[0]))

#define MAX_CMD_LENGTH		32767
#define ResultFromShort(i)	ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

#define INITGUID
#include <initguid.h>
#include <shlguid.h>

DEFINE_GUID(CLSID_ShellExtension, 0xD38AFDB7, 0xE7BA, 0x4F02, 0xA1, 0xEF, 0xD8, 0xDC, 0xBA, 0x4E, 0x71, 0x35);
namespace {
	TCHAR szGUID[] = TEXT("{D38AFDB7-E7BA-4F02-A1EF-D8DCBA4E7135}");
}

class CShellExtClassFactory : public IClassFactory
{
protected:
	ULONG m_cRef;

public:
	CShellExtClassFactory();
	virtual ~CShellExtClassFactory();

	// *** IUnknown methods ***
	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IClassFactory methods ***
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
	STDMETHODIMP LockServer(BOOL);
};

class CShellExt : public IContextMenu3, IShellExtInit, IPersistFile, IExtractIcon
{
private:
	//
	ULONG m_cRef;

	// Menu variables
	UINT m_cbFiles;
	STGMEDIUM m_stgMedium;
	LPDATAOBJECT m_pDataObj;
	TCHAR m_szDllDir[MAX_PATH];
	TCHAR m_szMenuTitle[TITLE_SIZE];
	UINT m_menuID;
	HMENU m_hMenu;
	bool m_showIcon;

	// Icon variables
	TCHAR m_szFilePath[MAX_PATH];
	TCHAR m_szModule[MAX_PATH];
	TCHAR m_szCustomPath[MAX_PATH];
	bool m_useCustom;
	int m_nameLength;
	int m_nameMaxLength;
	bool m_isDynamic;

	DWORD m_winVer;	//current windows version

	// *** Private methods ***
	STDMETHODIMP InvokeMiamPlayer(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd, LPCSTR pszParam, int iShowCmd);
	STDMETHODIMP LoadShellIcon(int cx, int cy, HICON * phicon);

public:
	CShellExt();
	virtual ~CShellExt();

	// *** IUnknown methods ***
	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IShellExtInit methods ***
	STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);

	// *** IContextMenu methods ***
	STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
	STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
	STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { return HandleMenuMsg2(uMsg, wParam, lParam, NULL); }
	STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

	// *** IPersistFile methods ***
	STDMETHODIMP GetClassID(CLSID *) { return E_NOTIMPL; }
	STDMETHODIMP IsDirty(void) { return E_NOTIMPL; }
	STDMETHODIMP Save(LPCOLESTR, BOOL) { return E_NOTIMPL; }
	STDMETHODIMP SaveCompleted(LPCOLESTR) { return E_NOTIMPL; }
	STDMETHODIMP GetCurFile(LPOLESTR *) { return E_NOTIMPL; }
	STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);

	// *** IExtractIcon methods ***
	STDMETHODIMP GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
	STDMETHODIMP Extract(LPCTSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);

	/// TEST
	void toggleSubMenu(bool disabled);
};
