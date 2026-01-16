#include "pch.h"
#include "framework.h"
#include "EOL_Parser.h"
#include "EOL_ParserDlg.h"
#include "afxdialogex.h"
#include "sqlite3.h"
#include <vector>
#include <cmath>
#include <algorithm>

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

	DDX_Control(pDX, IDC_COMBO_MODEL, m_comboModel); // [추가] 콤보박스 ID 확인!
}

// [이벤트 맵] 버튼 클릭 등의 동작 연결
BEGIN_MESSAGE_MAP(CEOLParserDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_FolderOpen, &CEOLParserDlg::OnBnClickedBtnFolderopen)
	ON_EN_CHANGE(IDC_FolderPath, &CEOLParserDlg::OnEnChangeFolderpath)
	ON_CBN_SELCHANGE(IDC_COMBO_MODEL, &CEOLParserDlg::OnCbnSelchangeComboModel)

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

	// 폰트 크기 조절 (크기 22, 굵게 설정)
	m_font.CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("맑은 고딕"));
	GetDlgItem(IDC_BTN_FolderOpen)->SetFont(&m_font);

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


	m_comboModel.ResetContent();
	m_comboModel.AddString(_T("LPE 48k"));
	m_comboModel.AddString(_T("LPE 50k"));
	m_comboModel.AddString(_T("LPE 52k"));
	m_comboModel.AddString(_T("LPE 54k"));
	m_comboModel.AddString(_T("LPE 59k"));
	m_comboModel.AddString(_T("LPS 46k"));
	m_comboModel.AddString(_T("LPS 49k"));
	m_comboModel.AddString(_T("LPS 52k"));
	m_comboModel.AddString(_T("LPS 55k"));
	m_comboModel.AddString(_T("LPS 58k"));

	m_comboModel.SetCurSel(0); // 기본값 48k 선택

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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 1. 수학 및 통계 계산 함수
double CEOLParserDlg::GetMean(const std::vector<double>& data) {
	if (data.empty()) return 0.0;
	double sum = 0.0;
	for (double d : data) sum += d;
	return sum / data.size();
}

double CEOLParserDlg::GetStdDev(const std::vector<double>& data, double mean) {
	if (data.empty()) return 0.0;
	double sum = 0.0;
	for (double d : data) sum += pow(d - mean, 2);
	return sqrt(sum / data.size());
}

// 파이썬의 stats.norm.pdf(x, 0, 1)와 동일한 결과
double CEOLParserDlg::NormalPDF(double x) {
	return (1.0 / sqrt(2.0 * M_PI)) * exp(-0.5 * x * x);
}

// 2. 콤보박스 변경 시 호출
void CEOLParserDlg::OnCbnSelchangeComboModel() {
	Invalidate(); // OnPaint 강제 호출
}

// 3. 실제 PDF 그래프 그리기 로직
void CEOLParserDlg::DrawNormalDistribution(CPaintDC& dc, CRect rect, std::vector<double>& data, COLORREF color)
{
	if (data.size() < 2) return;

	// 1. 통계치 및 정규화 데이터 생성
	double mean = GetMean(data);
	double stdDev = GetStdDev(data, mean);
	if (stdDev == 0) stdDev = 0.00001;

	std::vector<double> normData;
	double minX = 0, maxX = 0;
	for (double v : data) {
		double nv = (v - mean) / stdDev;
		normData.push_back(nv);
		if (nv < minX) minX = nv;
		if (nv > maxX) maxX = nv;
	}
	// 여유분 추가 (이미지처럼 -2 ~ 8 근처까지 보이게)
	minX -= 0.5; maxX += 0.5;

	// 2. 히스토그램 데이터 계산 (100 Bins)
	int binCount = 100;
	std::vector<double> bins(binCount, 0.0);
	double binWidth = (maxX - minX) / binCount;

	for (double nv : normData) {
		int idx = (int)((nv - minX) / binWidth);
		if (idx >= 0 && idx < binCount) bins[idx]++;
	}

	// Y축 밀도(Density) 계산: 빈도수 / (전체수 * bin너비)
	double maxDensity = 0;
	for (int i = 0; i < binCount; i++) {
		bins[i] = bins[i] / (data.size() * binWidth);
		if (bins[i] > maxDensity) maxDensity = bins[i];
	}
	if (maxDensity < 0.4) maxDensity = 0.4; // 곡선이 잘리지 않게 최소 높이 확보

	// 3. 초록색 히스토그램 그리기 (파이썬 스타일)
	CBrush histBrush(RGB(100, 180, 100)); // 이미지와 유사한 초록색
	CPen nullPen(PS_NULL, 0, RGB(0, 0, 0));
	dc.SelectObject(&histBrush);
	dc.SelectObject(&nullPen);

	for (int i = 0; i < binCount; i++) {
		if (bins[i] <= 0) continue;
		int x1 = rect.left + (int)((i * binWidth) / (maxX - minX) * rect.Width());
		int x2 = rect.left + (int)(((i + 1) * binWidth) / (maxX - minX) * rect.Width());
		int y = rect.bottom - (int)(bins[i] / maxDensity * rect.Height() * 0.9); // 상단 10% 여유
		dc.Rectangle(x1, y, x2 + 1, rect.bottom);
	}

	// 4. 빨간색 정규분포 곡선 (점선) 그리기
	CPen curvePen(PS_DASH, 2, RGB(255, 0, 0)); // 빨간색 점선
	dc.SelectObject(&curvePen);

	bool first = true;
	for (double tx = minX; tx <= maxX; tx += 0.1) {
		int x = rect.left + (int)((tx - minX) / (maxX - minX) * rect.Width());
		double ty = NormalPDF(tx); // 표준정규분포 PDF
		int y = rect.bottom - (int)(ty / maxDensity * rect.Height() * 0.9);

		if (first) { dc.MoveTo(x, y); first = false; }
		else { dc.LineTo(x, y); }
	}
}

// 4. OnPaint 함수 완성본
void CEOLParserDlg::OnPaint()
{
	CPaintDC dc(this);
	if (IsIconic()) {
		// ... (아이콘 생략) ...
	}
	else {
		CDialogEx::OnPaint();

		CWnd* pStatic = GetDlgItem(IDC_STATIC_GRAPH);
		if (!pStatic) return;
		CRect graphRect;
		pStatic->GetWindowRect(&graphRect);
		ScreenToClient(&graphRect);

		dc.FillSolidRect(graphRect, RGB(255, 255, 255));

		// 외곽 틀 그리기
		CPen axisPen(PS_SOLID, 1, RGB(0, 0, 0));
		dc.SelectObject(&axisPen);
		dc.MoveTo(graphRect.left, graphRect.top);
		dc.LineTo(graphRect.left, graphRect.bottom);
		dc.LineTo(graphRect.right, graphRect.bottom);

		int selIdx = m_comboModel.GetCurSel();
		if (selIdx == CB_ERR) return;

		std::vector<double> columnData;
		for (int i = 0; i < m_listData.GetItemCount(); i++) {
			columnData.push_back(_tstof(m_listData.GetItemText(i, selIdx + 1)));
		}

		if (!columnData.empty()) {
			DrawNormalDistribution(dc, graphRect, columnData, RGB(255, 0, 0));

			// 라벨 및 제목 출력
			dc.SetBkMode(TRANSPARENT);
			CString title;
			m_comboModel.GetLBText(selIdx, title);
			title += _T(" Normalized Data Distribution");

			CFont titleFont;
			titleFont.CreatePointFont(120, _T("Arial"));
			CFont* pOldFont = dc.SelectObject(&titleFont);
			dc.TextOutW(graphRect.left + (graphRect.Width() / 4), graphRect.top - 25, title);
			dc.SelectObject(pOldFont);

			// X축 하단 설명
			dc.TextOutW(graphRect.CenterPoint().x - 40, graphRect.bottom + 20, _T("Normalized Value"));
			// Y축 좌측 설명 (세로 쓰기는 복잡하므로 간략히)
			dc.TextOutW(graphRect.left - 45, graphRect.CenterPoint().y, _T("Density"));
		}
	}
}