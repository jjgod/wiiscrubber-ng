#if !defined(AFX_BOOTMODE_H__0EC30DA7_618A_4C03_8D06_DEBCEEAF1C96__INCLUDED_)
#define AFX_BOOTMODE_H__0EC30DA7_618A_4C03_8D06_DEBCEEAF1C96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BootMode.h : header file
//
	
/////////////////////////////////////////////////////////////////////////////
// CBootMode dialog
class CBootMode:public CDialog  {
	
// Construction
      public:unsigned char GetBootMode (void);
	void SetBootMode (unsigned int nVal);
	CBootMode (CWnd * pParent = NULL);	// standard constructor
	
// Dialog Data
		//{{AFX_DATA(CBootMode)
	enum { IDD = IDD_BOOTMODE };
	int m_nBootMode;
	
		//}}AFX_DATA
		
// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CBootMode)
      protected:virtual void DoDataExchange (CDataExchange * pDX);
								// DDX/DDV support
	//}}AFX_VIRTUAL
	
// Implementation
      protected:
		// Generated message map functions
		//{{AFX_MSG(CBootMode)
		// NOTE: the ClassWizard will add member functions here
		//}}AFX_MSG
DECLARE_MESSAGE_MAP () };

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
	
#endif // !defined(AFX_BOOTMODE_H__0EC30DA7_618A_4C03_8D06_DEBCEEAF1C96__INCLUDED_)
