/*
Launchy: Application Launcher
Copyright (C) 2005  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "CArchiveEx.h"
#include "zlib\zlib.h"

CString signature = _TEXT("akz");

CArchiveExt::CArchiveExt(CFile* pFile, UINT nMode, int nBufSize, void* lpBuf, CString Key, BOOL bCompress)
			:CArchive(pFile, nMode, nBufSize, lpBuf)
{
	m_strKey = Key;
	m_bCompress = bCompress;
	m_pFileTmp = m_pFile;

	if(!Key.IsEmpty())
		m_bCompress = TRUE;

	if(m_bCompress)
	{
		if(IsLoading())
		{
		    int iComprErr;
			CString Contents, Compress;
			_TCHAR sign[4];
			ULONGLONG dwFileSize = m_pFileTmp -> GetLength();
			LPWSTR szContents = Contents.GetBuffer((int) dwFileSize);
			ULONGLONG dwComprSize;

			m_pBuf = (BYTE *)new char[(size_t) dwFileSize];
			m_pFile = new CMemFile(m_pBuf, (uint) dwFileSize, 1024);

			m_pFileTmp -> SeekToBegin();

			ArcHeader ahead;
			m_pFileTmp -> Read(&ahead, sizeof(ahead));

			sign[0] = ahead.chSignature[0];
			sign[1] = ahead.chSignature[1];
			sign[2] = ahead.chSignature[2];
			sign[3] = _TEXT('\0');

			if(signature == sign)
			{
				dwFileSize = m_pFileTmp -> Read(szContents, (uint) dwFileSize);
				if(ahead.uchFlag == 3)
				{
					ULONG crc = adler32(0L, Z_NULL, 0);
//					Crypto(szContents, dwFileSize, m_strKey);
					crc = adler32(crc, (const Bytef*)szContents, (uInt) dwFileSize);
					if(crc != ahead.ulCRC)
					{
						AfxMessageBox(_T("Password incorrect!!!"));
						Abort();
						AfxThrowUserException();
					}
				}

				dwComprSize = ahead.dwOriginalSize;
				LPCTSTR szCompress = Compress.GetBuffer((int) dwComprSize);
				iComprErr = uncompress((LPBYTE)szCompress, (uLongf*) &dwComprSize, (LPBYTE)szContents, (uLong)dwFileSize);
				ASSERT(iComprErr == Z_OK);

				m_pFile -> Write(szCompress, (uint) dwComprSize);
			}
			else
			{
				m_pFileTmp -> SeekToBegin();
				dwFileSize = m_pFileTmp -> Read(szContents, (uint) dwFileSize);
				m_pFile -> Write(szContents, (uint) dwFileSize);
			}
			m_pFile -> SeekToBegin();
		}
		else
		{
			m_pBuf = (BYTE *)new char[1024];
			m_pFile = new CMemFile(m_pBuf, 1024, 1024);
		}
	}
}

CArchiveExt::~CArchiveExt()
{
	if (m_pFile != NULL && !(m_nMode & bNoFlushOnDelete))
		Close();

	Abort();    // abort completely shuts down the archive
}

void CArchiveExt::Abort()
{
	if(m_bCompress && m_pFile != NULL)
	{
		m_pBuf = (BYTE *)(((CMemFile*)m_pFile)->Detach());
		delete m_pBuf;
		m_pFile -> Close();
		delete m_pFile;
	}

	CArchive::Abort();
}

void CArchiveExt::Close()
{
	ASSERT_VALID(m_pFile);
	Flush();

	if(m_bCompress)
	{
		if(IsStoring())
		{
		    int iComprErr;
			CString Compress;
			ULONGLONG dwFileSize = m_pFile -> GetLength();
			DWORD dwComprSize;

			m_pBuf = (BYTE *)(((CMemFile*)m_pFile)->Detach());
			dwComprSize = (DWORD)(dwFileSize * 1.1) + 12;
			LPCTSTR szCompress = Compress.GetBuffer(dwComprSize);
			iComprErr = compress((LPBYTE)szCompress, &dwComprSize, m_pBuf, (uLong) dwFileSize);
			ASSERT(iComprErr == Z_OK);

			ULONG crc = adler32(0L, Z_NULL, 0);
			crc = adler32(crc, (const unsigned char*)szCompress, dwComprSize);
//			Crypto(szCompress, dwComprSize, m_strKey);

			ArcHeader ahead;
			ahead.chSignature[0] = _TEXT('a');
			ahead.chSignature[1] = _TEXT('k');
			ahead.chSignature[2] = _TEXT('z');
			ahead.dwOriginalSize = dwFileSize;
			ahead.uchFlag = m_strKey.IsEmpty()?1:3;
			ahead.ulCRC = crc;

			m_pFileTmp -> Write(&ahead, sizeof(ahead));
			m_pFileTmp -> Write(szCompress, dwComprSize);
		}
		else
		{
			m_pBuf = (BYTE *)(((CMemFile*)m_pFile)->Detach());
		}
		delete m_pBuf;
		m_pFile -> Close();
		delete m_pFile;
	}
	m_pFile = NULL;
}

bool CArchiveExt::Crypto(LPSTR lpstrBuffer, ULONG ulLen, CString strKey)
{
	if(strKey.IsEmpty())
		return FALSE;
	
	int nLenKey = strKey.GetLength();

	for(ULONG ii=0; ii<ulLen; ii++)
	{
		for(int jj=0; jj<nLenKey; jj++)
		{
			lpstrBuffer[ii] ^= strKey[jj];
		}
	}
	return TRUE;
}
