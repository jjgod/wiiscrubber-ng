#include "misc.h"
#include <iostream>
#include "WIIDisc.h"

using namespace std;


#ifdef WIN32
	#include "getopt-win32.h"
#else
	#include <sys/time.h>
	#include <getopt.h>
	#include <cstdlib>
#endif


/* Struct for program options */
struct options_s {
	char *scrub_file;
	char *output_file;
	char *diff_file;
	char *unscrub_file;
    char *extract_file;
	bool truchascrub;
	bool keep_headers;
	bool force_wii;
    long partition;
} options;


void welcome (void) {
	/* Welcome text */
	fprintf (stderr,
		PROGRAM_NAME " " PROGRAM_VERSION " - Copyright (C) 2008 Dack, SukkoPera\n"
		"This software comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; see COPYING for details.\n"
		"\n"
		"Official support forum: http://wii.console-tribe.com.\n"
		"\n"
		);
	fflush (stderr);

	return;
}

/*
 *	Examples:
 *	$ wiiscrubber -s mk.iso -o mk_scrubbed.iso
 *	$ wiiscrubber -s mk.iso -o mk_scrubbed.iso -d mk_scrubbed.diff
 *	$ wiiscrubber -u mk_scrubbed.iso -d mk_scrubbed.diff -o mk.iso
 */
void help (void) {
	/* 80 cols guide:
	 *      |-------------------------------------------------------------------------------|
	 */
	fprintf (stderr, "\n"
		"Available command line options:\n"
		" -h, --help			Show this help\n"
		" -s, --scrub <file>		Scrub the Wii image contained in <file>\n"
		" -e, --extract <file>		Extract the Wii image contained in <file>\n"
		" -o, --output <file>		Write the scrubbed/unscrubbed image to <file>\n"
		" -d, --diff <file>		Read/Write diff file from/to <file>\n"
		" -u, --unscrub <file>		Unscrub <file>, must be used with -d\n"
		" -t, --truchascrub		Use the \"Trucha\" scrubbing mode (See docs)\n"
		" -k, --keep-headers		Do not strip away file headers\n"
		" -w, --force-wii		Force Wii disc\n"
	);

	return;
}


bool optparse (int argc, char **argv) {
	bool out;
	int c;
	int option_index = 0;
	static struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"scrub", 1, 0, 's'},
		{"extract", 1, 0, 'e'},
		{"partition", 1, 0, 'p'},
		{"output", 1, 0, 'o'},
		{"diff", 1, 0, 'd'},
		{"unscrub", 1, 0, 'u'},
		{"truchascrub", 0, 0, 't'},
		{"keep-headers", 0, 0, 'k'},
		{"force-wii", 0, 0, 'w'},
		{0, 0, 0, 0}
	};

	if (argc == 1) {
		help ();
		exit (1);
	}
	
	/* Init options to default values */
	options.scrub_file = NULL;
	options.output_file = NULL;
	options.diff_file = NULL;
	options.unscrub_file = NULL;
	options.truchascrub = false;
	options.keep_headers = false;
	options.force_wii = false;
	
	do {
		c = getopt_long (argc, argv, "hs:e:o:d:u:tkw", long_options, &option_index);
		switch (c) {
			case 'h':
				help ();
				exit (1);
				break;

			case 's':
				my_strdup (options.scrub_file, optarg);
				break;

            case 'e':
                my_strdup (options.extract_file, optarg);
				break;

            case 'p':
                options.partition = strtol(optarg, NULL, 10);
                printf("partition: %d\n", options.partition);
                break;

			case 'o':
				my_strdup (options.output_file, optarg);
				break;
			case 'd':
				my_strdup (options.diff_file, optarg);
				break;
			case 'u':
				my_strdup (options.unscrub_file, optarg);
				break;
			case 't':
				options.truchascrub = true;
				break;
			case 'k':
				options.keep_headers = true;
				break;
			case 'w':
				options.force_wii = true;
				break;
			case -1:
				break;
			default:
// 				fprintf (stderr, "?? getopt returned character code 0%o ??\n", c);
				exit (7);
				break;
		}
	} while (c != -1);

	if (optind < argc) {
		/* Command-line arguments remaining. Ignore them, warning the user. */
		fprintf (stderr, "WARNING: Extra parameters ignored\n");
	}

	/* Sanity checks... */
	out = false;
	if (!options.scrub_file && !options.unscrub_file && !options.extract_file) {
		fprintf (stderr, "No operation specified. Please use the -s or -u options.\n");
	} else if (options.scrub_file && !options.output_file && !options.diff_file) {
		fprintf (stderr,
			"Please specify at least an output or a diff file!\n"
			"Take a look at the -o and -d options!\n"
		);
	} else if (options.unscrub_file && (!options.output_file || !options.diff_file)) {
		fprintf (stderr, "To unscrub an image you must specify both a diff and an output file.\n");
	} else {
		/* Specified options seem to make sense */
		out = true;
	}
		
	return (out);
}


int main (int argc, char *argv[]) {
	int ret;
	CWIIDisc *disc;
	struct image_file *image;			// FIXME Do not export!!!
	
	welcome ();
	
	if (optparse (argc, argv)) {
        if (options.extract_file) {
            disc = new CWIIDisc ();
			disc -> Reset ();

            if ((image = disc -> image_init (options.extract_file, options.force_wii))) {
				disc -> image_parse (image);
                disc -> ExtractPartitionFiles(image, 2, options.output_file);

                cout << "done." << endl;
                ret = 0;
            } else {
                cout << "Cannot init image" << endl;
				ret = 1;
            }

        }

        else if (options.scrub_file) {
			scrubHeadersMode hmode = options.keep_headers ? SCRUB_KEEP_HEADERS : SCRUB_REMOVE_HEADERS;
			
			disc = new CWIIDisc ();
		//	disc -> image_deinit(pImageFile);
			disc -> Reset ();
			if ((image = disc -> image_init (options.scrub_file, options.force_wii))) {
				disc -> image_parse (image);
				// disc -> CleanupISO (options.scrub_file, options.output_file, options.diff_file, hmode);
				
				//	u64 nTestCase;
				//	nTestCase = (u64) pcWiiDisc->CountBlocksUsed();
				//	nTestCase *= (u64)(0x8000);
				//	if (0==m_nHEADEROPTIONS)
				//		nTestCase = nTestCase + ((pcWiiDisc->nImageSize - nTestCase)/32);
				//	CString csTemp;
				//	csTemp.Format("Disc Data Size (%I64u) approx  %I64u MB", nTestCase, nTestCase/(1024 * 1024));
				
				ret = 0;
			} else {
				cout << "Cannot init image" << endl;
				ret = 1;
			}
		} else if (options.unscrub_file) {
			disc = new CWIIDisc ();
			disc -> Reset ();
			disc -> RecreateOriginalFile (options.unscrub_file, options.diff_file, options.output_file);
		}
	} else {
		ret = 1;
	}

	return (ret);
}
	
	
	
#if 0
	// we now have to copy the ISO over as well as create the files
	if (true ==
		pcWiiDisc->TruchaScrub (pImageFile, m_nActivePartition))
#endif

// Extract file
#if 0

		// get the text
		csText = m_cbDISCFILES.GetItemText (hItem);

		// now parse the fields out
		if (5 ==
			sscanf (csText, "%s [%I64X] [%I64X] [%I64X] [%d]",
				    cBuffer, &nPartitionOffset, &nFileOffset,
				    &nFileSize, &nFSTRef))
		{
			// then ask where to save it
			// create the save window 
			CFileDialog FileDialog (false, NULL, cBuffer);
			
			if (FileDialog.DoModal () == IDOK)
			{
				// now check to see if we need to change the offsets at all if a -VE reference
				switch (nFSTRef)
				{
					case -4:
						bOverRide = true;
						break;
					case -5:
					case -6:
					case -7:
						// one of the partition sub files
						nFileOffset = nFileOffset + pImageFile->parts[nPartitionOffset].offset;
						bOverRide = true;
						break;
						
					default:
						
						// do nothing
						break;
						
				}
				
				// then save if all ok
				if (true ==
					pcWiiDisc->SaveDecryptedFile(FileDialog.GetFileName (),
									   pImageFile,
									   (u_int32_t)(nPartitionOffset),
									   nFileOffset,
									   nFileSize,
									   bOverRide))
				{
					AddToLog ("File saved");
				}
				
				else
				{
					AddToLog ("Error in saving file");
				}
#endif

// Replace file
#if 0
			// now parse the fields out
			if (5 ==
				sscanf (csText,
					    "%s [%I64X] [%I64X] [%I64X] [%d]",
					    cBuffer, &nPartitionOffset,
					    &nFileOffset, &nFileSize,
					    &nFSTRef))
				
			{
				
				//then ask where to load from
				// create the load window 
				CFileDialog FileDialog (true, NULL,
										cBuffer);
				
				
				if (FileDialog.DoModal () == IDOK)
					
				{
					
					
					m_cbCLEANISO.EnableWindow (false);
					
					UpdateData (false);
					
					
					// then save if all ok
					if (true ==
						pcWiiDisc->
						LoadDecryptedFile
						(FileDialog.
						 GetFileName (),
						 pImageFile,
						 (u_int32_t)
						 (nPartitionOffset),
						 nFileOffset, nFileSize,
						 nFSTRef))
						
					{
						
						
						
						AfxMessageBox
						("Sucessfully replaced. Now reparsing");
						
						
						AddToLog ("File replaced");
						
						// we now need to reparse it
						ParseDiscDetails ();
		
#endif



#if 0
void CWIIScrubberDlg::ParseDiscDetails () 
{
	
	// load file
	if (true == m_bISOLoaded)
		
	{
		
		// free the old one first
		// cleanup
		pcWiiDisc->image_deinit (pImageFile);
		
	}
	
	pcWiiDisc->Reset ();
	
	pImageFile = pcWiiDisc->image_init (csFileName);
	
	if (NULL != pImageFile)
		
	{
		
		m_bISOLoaded = true;
		
		m_cbLISTLOG.DeleteAllItems ();
		
		m_cbDISCFILES.DeleteAllItems ();
		
		
		pcWiiDisc->image_parse (pImageFile);
		
		
		m_cbDISCFILES.Expand (m_cbDISCFILES.GetFirstVisibleItem (),
							  TVE_EXPAND);
		
		
		u_int64_t nTestCase;
		
		
		nTestCase = (u_int64_t) pcWiiDisc->CountBlocksUsed ();
		
		
		nTestCase *= (u_int64_t) (0x8000);
		
		
		// now if the headers button is clicked we need to add on the 1/32k per unmarked cluster
		
		if (0 == m_nHEADEROPTIONS)
			
		{
			
			nTestCase =
			nTestCase +
			((pcWiiDisc->nImageSize - nTestCase) / 32);
			
		}
		
		
		CString csTemp;
		
		
		csTemp.Format ("Disc Data Size (%I64u) approx  %I64u MB",
					   nTestCase, nTestCase / (1024 * 1024));
		
		AddToLog (csTemp);
		
		
		// find out the partition with data in
		unsigned int x = 0;
		
		
		while (x < pImageFile->nparts)
			
		{
			
			if (PART_DATA == pImageFile->parts[x].type)
				
			{
				
				break;
				
			}
			
			x++;
			
		}
		
		
		if (x == pImageFile->nparts)
			
		{
			
			// error as we have no data here?
			
		}
		
		else
			
		{
			
			// save the active partition info
			m_nActivePartition = x;
			
			// now clear out the junk data in the name
			for (int i = 0; i < 0x60; i++)
				
			{
				
				if (0 ==
					isprint ((int) pImageFile->parts[x].
							 header.name[i]))
					
				{
					
					pImageFile->parts[x].header.name[i] =
					' ';
					
				}
				
				
			} 
			
			
			m_csGAMENAME =
			pImageFile->parts[x].header.name;
			
			m_csGAMESIZE = csTemp;
			
			
			csTemp.Format ("Number of Files: %I64u",
						   pImageFile->nfiles);
			
			m_csNUMBEROFFILES = csTemp;
			
			
			switch (pImageFile->parts[x].header.region)
			
			{
					
				case 'P':
					
					csTemp = "Region Code: PAL";
					
					break;
					
				case 'E':
					
					csTemp = "Region Code: NTSC";
					
					break;
					
				case 'J':
					
					csTemp = "Region Code: JAP";
					
					break;
					
				default:
					
					// unknown or not quite right
					csTemp.Format ("Region Code: '%c'?",
								   pImageFile->parts[x].
								   header.region);
					
					break;
					
			}
			
			m_csREGIONCODE = csTemp;
			
			
			m_cbCLEANISO.EnableWindow (true);
			
			
			UpdateData (false);
			
		}
		
	}
	
	else
		
	{
		
		m_bISOLoaded = false;
		
		AddToLog ("Error on load");
		
	}
	
	
}

#endif



// Set boot mode
#if 0
void CWIIScrubberDlg::OnSystemboot () 
{
	
	// Set the window for changing the boot mode
	// simply call the disc function with the popup etc
	pcWiiDisc->SetBootMode (pImageFile);
	
} 
#endif


#if 0
//////////////////////////////////////////////////////////
// Delete the partition after getting confirmation      //
//////////////////////////////////////////////////////////
void CWIIScrubberDlg::OnDeletePartition () 
{
	
	if (AfxMessageBox
		("Are you sure you want to delete the partition?",
		 
		 MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2) == IDYES)
		
	{
		
		// we know the partition to delete
		// so we simply need to pass that parameter through to the disc
		// routine that will update it for us
		if (true ==
			pcWiiDisc->DeletePartition (pImageFile,
										m_nWorkingPartition))
			
		{
			
			AfxMessageBox ("Partition deleted - now reparsing");
			
			AddToLog ("Partition deleted. Now reparsing");
			
			ParseDiscDetails ();
			
		}
		
		
	}
	
	else
		
	{
		
		AddToLog ("Partition delete cancelled");
		
	}
	
	
}


//////////////////////////////////////////////////////////
// Show the current size of the partition and allow for //
// a change                                             //
//////////////////////////////////////////////////////////
void CWIIScrubberDlg::OnSizePartition () 
{
	
	// simply need to call the disc function with the correct parameters
	if (true ==
		pcWiiDisc->ResizePartition (pImageFile,
									m_nWorkingPartition))
		
	{
		
		AddToLog ("Partition data resized");
		
	}
	
	else
		
	{
		
		AddToLog ("Resize partition cancelled");
		
	}
	
	
}


//////////////////////////////////////////////////////////////
// Added so that a more accurate file size is shown for the //
// bytes that are used in the image                         //
//////////////////////////////////////////////////////////////


void CWIIScrubberDlg::OnScruboptions2 () 
{
	
	u_int64_t nTestCase;
	
	
	UpdateData (true);
	
	
	nTestCase = (u_int64_t) pcWiiDisc->CountBlocksUsed ();
	
	
	nTestCase *= (u_int64_t) (0x8000);
	
	
	// now if the headers button is clicked we need to add on the 1/32k per unmarked cluster
	
	if (0 == m_nHEADEROPTIONS)
		
	{
		
		nTestCase =
		nTestCase +
		((pcWiiDisc->nImageSize - nTestCase) / 32);
		
	}
	
	
	CString csTemp;
	
	
	csTemp.Format ("Disc Data Size (%I64u) approx  %I64u MB", nTestCase,
				   nTestCase / (1024 * 1024));
	
	AddToLog (csTemp);
	
	
	m_csGAMESIZE = csTemp;
	
	UpdateData (false);
	
	
}


void CWIIScrubberDlg::OnShrinkpartition () 
{
	
	// Here we get the partition info
	// and call the wiidisc function
	if (true ==
		pcWiiDisc->DoPartitionShrink (pImageFile,
									  m_nWorkingPartition))
		
	{
		
		ParseDiscDetails ();
		
		AddToLog ("Partition shrunk");
		
	}
	
	else
		
	{
		
		AddToLog ("Partition shrink cancelled");
		
	}
	
	
	
	
}


void CWIIScrubberDlg::OnLoadpartition () 
{
	
	// Prompt for filename
	CString csText;
	
	CString csName;
	
	char cBuffer[1000];
	
	
	
	sprintf (cBuffer, "Partition%d.img", m_nWorkingPartition);
	
	// then ask where to save it
	// create the save window 
	CFileDialog FileDialog (true, NULL, cBuffer);
	
	
	if (FileDialog.DoModal () == IDOK)
		
	{
		
		// then Load if all ok
		if (true ==
			pcWiiDisc->LoadDecryptedPartition (FileDialog.
											   GetFileName (),
											   pImageFile,
											   m_nWorkingPartition))
			
		{
			
			AddToLog ("Partition loaded");
			
			// reparse
			ParseDiscDetails ();
			
			
		}
		
		else
			
		{
			
			AddToLog ("Error in loading file");
			
		}
		
		
	}
	
	else
		
	{
		
		AddToLog ("Load partition cancelled");
		
	}
	
	
	
}


void CWIIScrubberDlg::OnSavepartition () 
{
	
	// Prompt for filename
	CString csText;
	
	CString csName;
	
	char cBuffer[1000];
	
	
	
	sprintf (cBuffer, "Partition%d.img", m_nWorkingPartition);
	
	// then ask where to save it
	// create the save window 
	CFileDialog FileDialog (false, NULL, cBuffer);
	
	
	if (FileDialog.DoModal () == IDOK)
		
	{
		
		// then save if all ok
		if (true ==
			pcWiiDisc->SaveDecryptedPartition (FileDialog.
											   GetFileName (),
											   pImageFile,
											   m_nWorkingPartition))
			
		{
			
			AddToLog ("Partition saved");
			
		}
		
		else
			
		{
			
			AddToLog ("Error in saving file");
			
		}
		
		
	}
	
	
	
}


void CWIIScrubberDlg::OnAddpartitionorchannel () 
{
	
	u_int64_t nFreeSize = 0;
	
	
	u_int64_t nPartStart = 0;
	
	
	CString csText;
	
	
	// check to see if we have free space to do it in the disc image
	
	nFreeSize = pcWiiDisc->GetFreeSpaceAtEnd (pImageFile);
	
	
	if (0x20000 <= nFreeSize)
		
	{
		
		// take off the header
		nFreeSize = nFreeSize - 0x20000;
		
		
		// space to do something
		
		nPartStart =
		pcWiiDisc->GetFreePartitionStart (pImageFile);
		
		
		// Get the correct parameters for the type
		// create partition by updateing the correct table
		// create fake entries in the partition so it can be used - fst.bin, main.dol, apploader.img, data size, name, correct type
		
		CAddPartition * pWindow = new CAddPartition ();
		
		
		pWindow->SetMaxSize (nFreeSize);
		
		
		if (IDOK == pWindow->DoModal ())
			
		{
			
			
			// use the values from the partition window
			pcWiiDisc->AddPartition (pImageFile,
									 (pWindow->
									  GetPartitionType ()
									  == 1), 
									 nPartStart,
									 pWindow->GetSize (),
									 pWindow->
									 GetPartitionName ());
			
			
			delete pWindow;
			
			
			// reparse
			ParseDiscDetails ();
			
			AddToLog ("Partition added");
			
		}
		
		else
			
		{
			
			AddToLog ("Partition add cancelled");
			
			delete pWindow;
			
			
		}
		
	}
	
	else
		
	{
		
		AfxMessageBox ("Not enough free space to add a partition");
		
	}
	
}


///////////////////////////////////////////////////////////////
// This goes through the image file and cause the partitions //
// to shuffle up. Moving all the free space to the end of    //
// disc                                                      //
///////////////////////////////////////////////////////////////
void CWIIScrubberDlg::OnShufflepartitions () 
{
	
	// Check to see we have partitions to play with
	if (1 != pImageFile->nparts)
		
	{
		
		// if so then get confirmation
		// call the function to validate the partitions and then
		// shuffle them all up to the start of the disc
		if (true == pcWiiDisc->DoTheShuffle (pImageFile))
			
		{
			
			// reparse it
			ParseDiscDetails ();
			
			AddToLog ("Disc image shuffled");
			
			
		}
		
		else
			
		{
			
			AddToLog ("Shuffle cancelled or none to move");
			
		}
		
	}
	
	else
		
	{
		
		AfxMessageBox ("No partitions to shuffle");
		
	}
	
}


////////////////////////////////////////////////////
// Loads a directory into the selected partition  //
// and generates the fst etc. for it              //
////////////////////////////////////////////////////
void CWIIScrubberDlg::OnCreatefst () 
{
	
	
	u_int64_t nFreeSize = 0;
	
	
	u_int64_t nPartStart = 0;
	
	
	CString csText;
	
	
	// check to see if we have free space to do it in the disc image
	
	nFreeSize = pcWiiDisc->GetFreeSpaceAtEnd (pImageFile);
	
	
	if (0x20000 <= nFreeSize)
		
	{
		
		// take off the header
		nFreeSize = nFreeSize - 0x20000;
		
		
		// space to do something
		// get a file name
		// check the size
		// has to be > 0x20000 and less than the size left after mangling
		// create  partition of the right size
		// load the file in
		
		// create the load window 
		CFileDialog FileDialog (true, NULL, NULL);
		
		
		if (FileDialog.DoModal () == IDOK)
			
		{
			
			// check the size
			FILE * fIn;
			
			fIn = fopen (FileDialog.GetFileName (), "rb");
			
			u_int64_t nFileSize =
			_lseeki64 (fIn->_file, 0L, SEEK_END);
			
			fclose (fIn);
			
			
			if (0x20000 > nFileSize)
				
			{
				
				AddToLog ("Partition file too small");
				
				AfxMessageBox ("Partition file too small");
				
			}
			
			else if (nFreeSize <
					 (((nFileSize - 0x20000) / 0x7c00) * 0x8000))
				
			{
				
				AddToLog
				("Free space left in ISO too small");
				
				AfxMessageBox
				("Not enough free space for this partition");
				
			}
			
			else
				
			{
				
				// create a partition
				nPartStart =
				pcWiiDisc->
				GetFreePartitionStart (pImageFile);
				
				// use the values from the partition window
				u_int8_t cGash[7] = {
				'R', '1', '2', '3', '4', '5'};
				
				
				pcWiiDisc->AddPartition (pImageFile, false,
										 nPartStart,
										 nFreeSize, cGash);
				
				
				pcWiiDisc->get_partitions (pImageFile);
				
				
				// then Load if all ok
				if (true ==
					pcWiiDisc->
					LoadDecryptedPartition
					(FileDialog.GetFileName (),
					 pImageFile,
					 pImageFile->PartitionCount))
					
				{
					
					AddToLog ("Partition imported");
					
					// reparse
					ParseDiscDetails ();
					
					
				}
				
				else
					
				{
					
					AddToLog ("Error in loading file");
					
				}
				
				
			}
			
		}
		
		else
			
		{
			
			AddToLog ("Import partition cancelled");
			
			
		}
		
	}
	
	else
		
	{
		
		AfxMessageBox ("Not enough free space to add a partition");
		
	}
	
}

/////////////////////////////////////////////////////////////////////
// Extract the current partitions data files to a passed directory //
/////////////////////////////////////////////////////////////////////
void CWIIScrubberDlg::OnPartitionextract () 
{
	
	// get directory name
	LPMALLOC pMalloc;
	
	
	UpdateData ();
	
	if (::SHGetMalloc (&pMalloc) == NOERROR)
		
	{
		
		BROWSEINFO bi;
		
		char pszBuffer[MAX_PATH];
		
		LPITEMIDLIST pidl;
		
		// Get help on BROWSEINFO struct - it's got all the bit settings.
		bi.hwndOwner = GetSafeHwnd ();
		
		bi.pidlRoot = NULL;
		
		bi.pszDisplayName = pszBuffer;
		
		bi.lpszTitle = _T ("Select a Save Directory");
		
		bi.ulFlags = 0x0040 | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;	// add on the BIF value thats not included in this header :)
		// To allow the new folder button
		bi.lpfn = NULL;
		
		bi.lParam = 0;
		
		// This next call issues the dialog box.
		if ((pidl =::SHBrowseForFolder (&bi)) != NULL)
			
		{
			
			if (::SHGetPathFromIDList (pidl, pszBuffer))
				
			{
				
				// At this point pszBuffer contains the selected path */.
				// so we call the disc save command with the image, active partition and filename
				if (true ==
					pcWiiDisc->
					ExtractPartitionFiles (pImageFile,
										   m_nWorkingPartition,
										   (u_int8_t
											*)
										   pszBuffer))
					
				{
					
					AddToLog ("Partition extracted");
					
				}
				
				else
					
				{
					
					AddToLog
					("Partition extraction error or cancelled");
					
				}
				
			}
			
			// Free the PIDL allocated by SHBrowseForFolder.
			pMalloc->Free (pidl);
			
		}
		
		// Release the shell's allocator.
		pMalloc->Release ();
		
	}
	
	
}


#endif
