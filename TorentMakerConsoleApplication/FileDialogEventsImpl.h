#pragma once

#include <ShObjIdl.h>
#include <wrl.h>

class FileDialogEventsImpl : 
	public Microsoft::WRL::RuntimeClass<
	Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>, 
	IFileDialogEvents>
{
public:
	FileDialogEventsImpl();
	virtual ~FileDialogEventsImpl();

	virtual HRESULT STDMETHODCALLTYPE OnFileOk(
		/* [in] */ __RPC__in_opt IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnFolderChanging(
		/* [in] */ __RPC__in_opt IFileDialog *pfd,
		/* [in] */ __RPC__in_opt IShellItem *psiFolder) override;

	virtual HRESULT STDMETHODCALLTYPE OnFolderChange(
		/* [in] */ __RPC__in_opt IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnSelectionChange(
		/* [in] */ __RPC__in_opt IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnShareViolation(
		/* [in] */ __RPC__in_opt IFileDialog *pfd,
		/* [in] */ __RPC__in_opt IShellItem *psi,
		/* [out] */ __RPC__out FDE_SHAREVIOLATION_RESPONSE *pResponse) override;

	virtual HRESULT STDMETHODCALLTYPE OnTypeChange(
		/* [in] */ __RPC__in_opt IFileDialog *pfd) override;

	virtual HRESULT STDMETHODCALLTYPE OnOverwrite(
		/* [in] */ __RPC__in_opt IFileDialog *pfd,
		/* [in] */ __RPC__in_opt IShellItem *psi,
		/* [out] */ __RPC__out FDE_OVERWRITE_RESPONSE *pResponse) override;
};