// BootMode.cpp : implementation file
//

#include "stdafx.h"
#include "WIIScrubber.h"
#include "BootMode.h"
	
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif	/*  */
	
/////////////////////////////////////////////////////////////////////////////
// CBootMode dialog
	CBootMode::CBootMode (CWnd * pParent /*=NULL*/ ) 
:CDialog (CBootMode::IDD, pParent) 
{
	
		//{{AFX_DATA_INIT(CBootMode)
		m_nBootMode = -1;
	
		//}}AFX_DATA_INIT
} void CBootMode::DoDataExchange (CDataExchange * pDX) 
{
	CDialog::DoDataExchange (pDX);
	
		//{{AFX_DATA_MAP(CBootMode)
		DDX_CBIndex (pDX, IDC_BOOTMODE, m_nBootMode);
	
		//}}AFX_DATA_MAP
} BEGIN_MESSAGE_MAP (CBootMode, CDialog) 
	//{{AFX_MSG_MAP(CBootMode)
	// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP ()  
/////////////////////////////////////////////////////////////////////////////
// CBootMode message handlers
     void CBootMode::SetBootMode (unsigned int nVal) 
{
	m_nBootMode = nVal;
} unsigned char CBootMode::GetBootMode () 
{
	return (unsigned char) m_nBootMode;
} 
