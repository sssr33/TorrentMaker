#include "FileDialogEventsImpl.h"

FileDialogEventsImpl::FileDialogEventsImpl() {
}

FileDialogEventsImpl::~FileDialogEventsImpl() {
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnFileOk(
	/* [in] */ __RPC__in_opt IFileDialog *pfd)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnFolderChanging(
	/* [in] */ __RPC__in_opt IFileDialog *pfd,
	/* [in] */ __RPC__in_opt IShellItem *psiFolder)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnFolderChange(
	/* [in] */ __RPC__in_opt IFileDialog *pfd)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnSelectionChange(
	/* [in] */ __RPC__in_opt IFileDialog *pfd)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnShareViolation(
	/* [in] */ __RPC__in_opt IFileDialog *pfd,
	/* [in] */ __RPC__in_opt IShellItem *psi,
	/* [out] */ __RPC__out FDE_SHAREVIOLATION_RESPONSE *pResponse)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnTypeChange(
	/* [in] */ __RPC__in_opt IFileDialog *pfd)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT STDMETHODCALLTYPE FileDialogEventsImpl::OnOverwrite(
	/* [in] */ __RPC__in_opt IFileDialog *pfd,
	/* [in] */ __RPC__in_opt IShellItem *psi,
	/* [out] */ __RPC__out FDE_OVERWRITE_RESPONSE *pResponse)
{
	HRESULT hr = S_OK;
	return hr;
}