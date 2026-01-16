#include "pch.h"
#include "framework.h"
#include "EOL_Parser.h"
#include "EOL_ParserDlg.h"
#include "afxdialogex.h"
#include "sqlite3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// [전역 변수] DB 핸들
static sqlite3* g_db = nullptr;

// ==========================================================
// 1. 기본 대화 상자 관리 (MFC 생성 코드)
// ==========================================================

// [정보 창] 클래스 및 메시지 맵
class CAboutDlg : public CDialogEx {
public:
	CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// [메인 창] 생성자
CEOLParserDlg::CEOLParserDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EOL_PARSER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// [연결] UI 컨트롤 ID와 코드 상의 변수 연결
void CEOLParserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DB_LIST, m_listData); // 리스트 컨트롤 연결
	DDX_Control(pDX, IDC_DB_LIST, m_listData);
}

// [이벤트 맵] 버튼 클릭 등의 동작 연결
BEGIN_MESSAGE_MAP(CEOLParserDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_FolderOpen, &CEOLParserDlg::OnBnClickedBtnFolderopen)
	ON_EN_CHANGE(IDC_FolderPath, &CEOLParserDlg::OnEnChangeFolderpath)
END_MESSAGE_MAP()

// ==========================================================
// 2. 초기화 및 시스템 이벤트 (OnInitDialog, OnPaint 등)
// ==========================================================

BOOL CEOLParserDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴 설정 (AboutBox)
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		CString strAboutMenu;
		if (strAboutMenu.LoadString(IDS_ABOUTBOX) && !strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 아이콘 설정
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// [리스트 컨트롤 초기화] 제목줄 생성 및 격자선 설정
	m_listData.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listData.InsertColumn(0, _T("Serial Number"), LVCFMT_LEFT, 170);
	m_listData.InsertColumn(1, _T("LPE 48k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(2, _T("LPE 50k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(3, _T("LPE 52k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(4, _T("LPE 54k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(5, _T("LPE 59k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(6, _T("LPS 46k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(7, _T("LPS 49k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(8, _T("LPS 52k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(9, _T("LPS 55k"), LVCFMT_LEFT, 80);
	m_listData.InsertColumn(10, _T("LPS 58k"), LVCFMT_LEFT, 80);

	return TRUE;
}

// 정보창 호출 처리
void CEOLParserDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 창 그리기 (최소화 시 아이콘 처리)
void CEOLParserDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		dc.DrawIcon((rect.Width() - cxIcon + 1) / 2, (rect.Height() - cyIcon + 1) / 2, m_hIcon);
	}
	else {
		CDialogEx::OnPaint();
	}
}

HCURSOR CEOLParserDlg::OnQueryDragIcon() { return static_cast<HCURSOR>(m_hIcon); }

// ==========================================================
// 3. 사용자 정의 핵심 기능 (파싱, DB 저장, 출력)
// ==========================================================

// [유틸리티] HTML 태그 사이의 문자열을 잘라오는 도구
CString CEOLParserDlg::ExtractHtmlValue(const CString& source, CString startTag, CString endTag, int& searchPos) {
	int start = source.Find(startTag, searchPos);
	if (start == -1) return _T("");
	start += startTag.GetLength();
	int end = source.Find(endTag, start);
	if (end == -1) return _T("");
	searchPos = end;
	return source.Mid(start, end - start);
}

// [메인 로직] 폴더 선택 -> 파일 탐색 -> 데이터 추출 -> DB 저장
void CEOLParserDlg::OnBnClickedBtnFolderopen()
{
	CFolderPickerDialog dlg(NULL, OFN_FILEMUSTEXIST, this);

	if (dlg.DoModal() == IDOK)
	{
		CString folderPath = dlg.GetPathName();
		SetDlgItemText(IDC_FolderPath, folderPath); // 에디트 박스에 경로 표시

		// 0. 기존 DB 파일이 있으면 삭제하여 초기화
		CString dbPath = folderPath + _T("\\result.db"); //DB 경로 설정
		if (PathFileExists(dbPath)) { // 파일이 존재하는지 확인
			DeleteFile(dbPath);       // 파일 삭제
		}

		// 1. DB 연결 및 테이블 생성
		if (sqlite3_open(CT2A(dbPath), &g_db) != SQLITE_OK) {
			AfxMessageBox(_T("DB 생성에 실패했습니다."));
			return;
		}

		const char* createSql = "CREATE TABLE IF NOT EXISTS EOL_DATA ("
			"ID INTEGER PRIMARY KEY AUTOINCREMENT, SN TEXT, "
			"LPE48 REAL, LPE50 REAL, LPE52 REAL, LPE54 REAL, LPE59 REAL, "
			"LPS46 REAL, LPS49 REAL, LPS52 REAL, LPS55 REAL, LPS58 REAL);";
		sqlite3_exec(g_db, createSql, nullptr, nullptr, nullptr);

		// 2. HTML 파일들을 찾아서 데이터 파싱
		CFileFind finder;
		CString searchPath = folderPath + _T("\\*.html");
		BOOL bWorking = finder.FindFile(searchPath);
		int count = 0;

		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			if (finder.IsDots() || finder.IsDirectory()) continue;

			CStdioFile file;
			if (file.Open(finder.GetFilePath(), CFile::modeRead | CFile::typeText)) {
				CString line, fullText;
				while (file.ReadString(line)) fullText += line;
				file.Close();

				// 파싱 시작
				TestResult res;
				int pos = 0;

				//erialNumber 파싱
				int snLabelPos = fullText.Find(_T("<td>Serial Number:</td>"));
				if (snLabelPos != -1) {
					// 2. 그 라벨 바로 다음에 오는 <td> 태그를 찾아서 그 안의 값을 가져옵니다.
					int nextTd = fullText.Find(_T("<td>"), snLabelPos + 20); // 20은 라벨 길이 대략 계산
					pos = nextTd; // ExtractHtmlValue가 검색을 시작할 위치 지정
					res.serialNumber = ExtractHtmlValue(fullText, _T("<td>"), _T("</td>"), pos);
				}

				for (int i = 0; i < 10; ++i) {
					double val = _tstof(ExtractHtmlValue(fullText, _T("<td class=\"value\">"), _T("</td>"), pos));
					if (i < 5) res.lpe.push_back(val); else res.lps.push_back(val);
				}

				// 3. DB에 쿼리로 저장
				const char* insSql = "INSERT INTO EOL_DATA (SN, LPE48, LPE50, LPE52, LPE54, LPE59, LPS46, LPS49, LPS52, LPS55, LPS58) VALUES (?,?,?,?,?,?,?,?,?,?,?);";
				sqlite3_stmt* stmt;
				if (sqlite3_prepare_v2(g_db, insSql, -1, &stmt, nullptr) == SQLITE_OK) {
					sqlite3_bind_text(stmt, 1, CT2A(res.serialNumber), -1, SQLITE_TRANSIENT);
					for (int j = 0; j < 5; ++j) {
						sqlite3_bind_double(stmt, j + 2, res.lpe[j]);
						sqlite3_bind_double(stmt, j + 7, res.lps[j]);
					}
					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
					count++;
				}
			}
		}
		sqlite3_close(g_db); g_db = nullptr;

		// 4. 화면 리스트 컨트롤에 결과 출력
		ShowDatabaseContents(folderPath);

		CString msg;
		msg.Format(_T("%d개의 파일을 파싱하여 DB에 저장했습니다."), count);
		AfxMessageBox(msg);
	}
}

// [출력] DB에서 데이터를 읽어 리스트 컨트롤(표)에 표시
void CEOLParserDlg::ShowDatabaseContents(CString folderPath) {
	m_listData.DeleteAllItems(); // 기존 화면 지우기

	CString dbPath = folderPath + _T("\\result.db");
	if (sqlite3_open(CT2A(dbPath), &g_db) != SQLITE_OK) return;

	const char* selSql = "SELECT SN, LPE48, LPE50, LPE52, LPE54, LPE59, LPS46, LPS49, LPS52, LPS55, LPS58 FROM EOL_DATA;";
	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(g_db, selSql, -1, &stmt, nullptr) == SQLITE_OK) {
		int row = 0;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			CString sn = CA2T((const char*)sqlite3_column_text(stmt, 0));
			int nIndex = m_listData.InsertItem(row++, sn);

			for (int col = 1; col <= 10; col++) {
				CString val;
				val.Format(_T("%.2f"), sqlite3_column_double(stmt, col));
				m_listData.SetItemText(nIndex, col, val);
			}
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(g_db); g_db = nullptr;
}

// [기타] 경로 변경 시 처리 (현재 비어있음)
void CEOLParserDlg::OnEnChangeFolderpath() {}

// ==========================================================
// 4. 그래프
// ==========================================================

