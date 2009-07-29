// AddPartition.cpp : implementation file
//

#include "WIIScrubber.h"
#include "AddPartition.h"
	
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif	/*  */
	
/////////////////////////////////////////////////////////////////////////////
// CAddPartition dialog
	CAddPartition::CAddPartition (CWnd * pParent /*=NULL*/ ) 
:CDialog (CAddPartition::IDD, pParent) 
{
	
		//{{AFX_DATA_INIT(CAddPartition)
		m_csPARTITIONSIZE = _T ("");
	m_nPARTITIONTYPE = 0;
	m_csPARTITIONNAME = _T ("");
	m_csPARTITIONMAX = _T ("");
	
		//}}AFX_DATA_INIT
} void CAddPartition::DoDataExchange (CDataExchange * pDX) 
{
	CDialog::DoDataExchange (pDX);
	
		//{{AFX_DATA_MAP(CAddPartition)
		DDX_Text (pDX, IDC_PARTITIONSIZE, m_csPARTITIONSIZE);
	DDX_Radio (pDX, IDC_PARTITION, m_nPARTITIONTYPE);
	DDX_Text (pDX, IDC_PARTITIONNAME, m_csPARTITIONNAME);
	DDV_MaxChars (pDX, m_csPARTITIONNAME, 6);
	DDX_Text (pDX, IDC_PARTITIONMAX, m_csPARTITIONMAX);
	
		//}}AFX_DATA_MAP
} BEGIN_MESSAGE_MAP (CAddPartition, CDialog) 
	//{{AFX_MSG_MAP(CAddPartition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP ()  
/////////////////////////////////////////////////////////////////////////////
// CAddPartition message handlers
     void CAddPartition::OnOK () 
{
	UpdateData (true);
	
		// convert length string to numeric
		if (0 ==
		    sscanf (m_csPARTITIONSIZE, "%I64X", &m_nPartitionSize))
		 {
		
			// no parse
			AfxMessageBox
			("Error: wrong format for partition size");
		return;
		}
	
		// now round size upto nearest 32k
		if (0 != (m_nPartitionSize % 0x8000))
		 {
		m_nPartitionSize = 0x8000 - (m_nPartitionSize % 0x8000);
		}
	if (m_nPartitionSize > m_nMaximumSize)
		 {
		AfxMessageBox ("Error: size is bigger than max");
		return;
		}
	
		// check string size
		if (6 != m_csPARTITIONNAME.GetLength ())
		 {
		AfxMessageBox ("Name must be 6 characters");
		return;
		}
	
		// check for alphanumerics
		for (int i = 0; i < 6; i++)
		 {
		if (!isprint (m_csPARTITIONNAME.GetAt (i)))
			 {
			AfxMessageBox
				("Error: all characters of name must be alphanumeric");
			return;
			}
		}
	CDialog::OnOK ();
}
void CAddPartition::SetMaxSize (u_int64_t nSize) 
{
	CString csText;
	csText.Format ("Maximum size = 0X%I64X", nSize);
	m_csPARTITIONMAX = csText;
	m_nMaximumSize = nSize;
} u_int64_t CAddPartition::GetSize () 
{
	return m_nPartitionSize;
}
int CAddPartition::GetPartitionType () 
{
	return m_nPARTITIONTYPE;
}
u_int8_t * CAddPartition::GetPartitionName () 
{
	memset (nData, 0, 8);
	for (int i = 0; i < 6; i++)
		 {
		nData[i] = m_csPARTITIONNAME.GetAt (i);
		} return nData;
}

