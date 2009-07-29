#if !defined(AFX_RESIZEPARTITION_H__73649E2E_923F_471B_B8C3_5C09C1589042__INCLUDED_)
#define AFX_RESIZEPARTITION_H__73649E2E_923F_471B_B8C3_5C09C1589042__INCLUDED_

#include "global.h"		// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResizePartition.h : header file
//
	
/////////////////////////////////////////////////////////////////////////////
// CResizePartition dialog
class CResizePartition:public CDialog  {
	
// Construction
      public:	u_int64_t GetNewSize (void);
	void SetRanges (u_int64_t nMin, u_int64_t nCurrent, u_int64_t nMax);
	CResizePartition (CWnd * pParent = NULL);	// standard constructor
	
// Dialog Data
		//{{AFX_DATA(CResizePartition)
	enum { IDD = IDD_RESIZEPARTITION };
	CString m_csCurrentSize;
	CString m_csMAXSIZE;
	CString m_csMINISIZE;
	CString m_csMINITEXT;
	CString m_csMAXITEXT;
	
		//}}AFX_DATA
		
// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CResizePartition)
      protected:virtual void DoDataExchange (CDataExchange * pDX);
								// DDX/DDV support
	//}}AFX_VIRTUAL
	
// Implementation
      protected:
		// Generated message map functions
		//{{AFX_MSG(CResizePartition)
	virtual void OnOK ();
	virtual void OnCancel ();
	
		//}}AFX_MSG
      DECLARE_MESSAGE_MAP ()  private:u_int64_t nNewSize;
	u_int64_t nMinSize;
	u_int64_t nMaxSize;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
	
#endif // !defined(AFX_RESIZEPARTITION_H__73649E2E_923F_471B_B8C3_5C09C1589042__INCLUDED_)
