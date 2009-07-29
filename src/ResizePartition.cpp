// ResizePartition.cpp : implementation file
//

#include "stdafx.h"
#include "WIIScrubber.h"
#include "ResizePartition.h"
	
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif	/*  */
	
/////////////////////////////////////////////////////////////////////////////
// CResizePartition dialog
	CResizePartition::CResizePartition (CWnd * pParent /*=NULL*/ ) 
:CDialog (CResizePartition::IDD, pParent) 
{
	
		//{{AFX_DATA_INIT(CResizePartition)
		m_csCurrentSize = _T ("");
	m_csMAXSIZE = _T ("");
	m_csMINISIZE = _T ("");
	m_csMINITEXT = _T ("");
	m_csMAXITEXT = _T ("");
	
		//}}AFX_DATA_INIT
		nNewSize = 0;
	nMaxSize = 0;
	nMinSize = 0;
} void CResizePartition::DoDataExchange (CDataExchange * pDX) 
{
	CDialog::DoDataExchange (pDX);
	
		//{{AFX_DATA_MAP(CResizePartition)
		DDX_Text (pDX, IDC_CURRENTSIZE, m_csCurrentSize);
	DDX_Text (pDX, IDC_MAXSIZE, m_csMAXSIZE);
	DDX_Text (pDX, IDC_MINISIZE, m_csMINISIZE);
	DDX_Text (pDX, IDC_MINITEXT, m_csMINITEXT);
	DDX_Text (pDX, IDC_MAXITEXT, m_csMAXITEXT);
	
		//}}AFX_DATA_MAP
} BEGIN_MESSAGE_MAP (CResizePartition, CDialog) 
	//{{AFX_MSG_MAP(CResizePartition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP ()  
/////////////////////////////////////////////////////////////////////////////
// CResizePartition message handlers
     void CResizePartition::OnOK () 
{
	u_int64_t nTempVal = 0;
	UpdateData (true);
	
		// get the value out
		sscanf (m_csCurrentSize, "%I64X", &nTempVal);
	
		// check its >=Min and < max
		if ((nTempVal != 0) && 
		    ((nTempVal >= nMinSize) && (nTempVal <= nMaxSize)))
		 {
		
			// if so then OK
			// we need to set it to a 32k boundary
			if (0 != (nTempVal % 0x8000))
			 {
			nNewSize =
				((nTempVal / (u_int64_t) 0x8000) +
				 (u_int64_t) 1) * (u_int64_t) 0x8000;
			}
		
		else
			 {
			nNewSize = nTempVal;
			}
		CDialog::OnOK ();
		}
	
		// else error
		else
		 {
		AfxMessageBox ("Error: value is out of range");
		}
}
void CResizePartition::OnCancel () 
{
	
		// TODO: Add extra cleanup here
		CDialog::OnCancel ();
} void CResizePartition::SetRanges (u_int64_t nMin, u_int64_t nCurrent,
				       u_int64_t nMax) 
{
	m_csMINISIZE.Format ("0x%I64X", nMin);
	m_csMAXSIZE.Format ("0x%I64X", nMax);
	m_csCurrentSize.Format ("0x%I64X", nCurrent);
	m_csMAXITEXT.Format ("(%I64uk)", nMax / (1024));
	m_csMINITEXT.Format ("(%I64uk)", nMin / (1024));
	nNewSize = nCurrent;
	nMinSize = nMin;
	nMaxSize = nMax;
} u_int64_t CResizePartition::GetNewSize () 
{
	return nNewSize;
}

