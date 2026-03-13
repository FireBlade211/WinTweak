///
///	psystem.h:
///		Provides signatures for functions for the System property sheet page.
/// 

#pragma once
#include <windows.h>
#include <shlobj.h>
#include <winsatcominterfacei.h>

INT_PTR CALLBACK SystemPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class WinSatProgressDialogEvents : public IWinSATInitiateEvents
{
private:
	IProgressDialog* pProgressDialog;
	HWND hOwner;
	LONG m_refCount;
public:
    WinSatProgressDialogEvents() : pProgressDialog(nullptr), hOwner(nullptr), m_refCount(1) {}
    virtual ~WinSatProgressDialogEvents()
    {
        if (pProgressDialog)
        {
            pProgressDialog->StopProgressDialog();
            pProgressDialog->Release();
            pProgressDialog = nullptr;
        }
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;

        if (riid == IID_IUnknown || riid == __uuidof(IWinSATInitiateEvents))
        {
            *ppv = static_cast<IWinSATInitiateEvents*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0)
            delete this;

        return ref;
    }

	void Init(HWND hwndOwner);
	STDMETHODIMP WinSATComplete(HRESULT hresult, LPCWSTR strDescription);
	STDMETHODIMP WinSATUpdate(UINT uCurrentTick, UINT uTickTotal, LPCWSTR strCurrentState);
};