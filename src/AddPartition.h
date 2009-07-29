#if !defined(AFX_ADDPARTITION_H__0ED05D76_4A72_41EF_9848_56C3E99F3E52__INCLUDED_)
#define AFX_ADDPARTITION_H__0ED05D76_4A72_41EF_9848_56C3E99F3E52__INCLUDED_

#include "global.h"		// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddPartition.h : header file
//
	
/////////////////////////////////////////////////////////////////////////////
// CAddPartition dialog
class CAddPartition:public CDialog  {
	
// Construction
      public:	u_int8_t nData[8];
	u_int8_t * GetPartitionName (void);
	int GetPartitionType (void);
	u_int64_t GetSize (void);
	void SetMaxSize (u_int64_t nSize);
	CAddPartition (CWnd * pParent = NULL);	// standard constructor
	
// Dialog Data
		//{{AFX_DATA(CAddPartition)
	enum { IDD = IDD_ADDPARTITION };
	CString m_csPARTITIONSIZE;
	int m_nPARTITIONTYPE;
	CString m_csPARTITIONNAME;
	CString m_csPARTITIONMAX;
	
		//}}AFX_DATA
		
// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CAddPartition)
      protected:virtual void DoDataExchange (CDataExchange * pDX);
								// DDX/DDV support
	//}}AFX_VIRTUAL
	
// Implementation
      protected:
		// Generated message map functions
		//{{AFX_MSG(CAddPartition)
	virtual void OnOK ();
	
		//}}AFX_MSG
      DECLARE_MESSAGE_MAP ()  private:u_int64_t
		m_nPartitionSize;
	u_int64_t m_nMaximumSize;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
	
#endif // !defined(AFX_ADDPARTITION_H__0ED05D76_4A72_41EF_9848_56C3E99F3E52__INCLUDED_)
