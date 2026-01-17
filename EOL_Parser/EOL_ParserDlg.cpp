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

	// [추가] 새로 만든 통계 리스트 연결
	DDX_Control(pDX, IDC_LIST_STATS, m_listStats);
}

// [이벤트 맵] 버튼 클릭 등의 동작 연결
BEGIN_MESSAGE_MAP(CEOLParserDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_FolderOpen, &CEOLParserDlg::OnBnClickedBtnFolderopen)
	ON_EN_CHANGE(IDC_FolderPath, &CEOLParserDlg::OnEnChangeFolderpath)
	ON_CBN_SELCHANGE(IDC_COMBO_MODEL, &CEOLParserDlg::OnCbnSelchangeComboModel)

	ON_NOTIFY(LVN_ITEMCHANGED, IIDC_LIST_STATS, &CEOLParserDlg::OnLvnItemchangedListStats)
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

	// [통계 리스트 초기화]
	m_listStats.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 1. 컬럼 생성 (0번: 구분, 1~10번: 데이터)
	m_listStats.InsertColumn(0, _T("구분"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(1, _T("LPE 48k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(2, _T("LPE 50k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(3, _T("LPE 52k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(4, _T("LPE 54k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(5, _T("LPE 59k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(6, _T("LPS 46k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(7, _T("LPS 49k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(8, _T("LPS 52k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(9, _T("LPS 55k"), LVCFMT_LEFT, 80);
	m_listStats.InsertColumn(10, _T("LPS 58k"), LVCFMT_LEFT, 80);

	// 2. [추가] 빈 행(Row) 미리 생성!
	// 프로그램 시작 시 바로 보이게 됩니다.
	m_listStats.InsertItem(0, _T("MIN (최소)"));
	m_listStats.InsertItem(1, _T("MEAN (평균)"));
	m_listStats.InsertItem(2, _T("MAX (최대)"));
	m_listStats.InsertItem(3, _T("RANGE (범위)"));
	m_listStats.InsertItem(4, _T("STD DEV (편차)"));
	m_listStats.InsertItem(5, _T("MEDIAN (중앙)"));

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
		// [추가] 통계 계산 및 출력 함수 호출!
		UpdateStatistics();
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

void CEOLParserDlg::UpdateStatistics()
{
	int rowCount = m_listData.GetItemCount();
	if (rowCount == 0) return;

	for (int i = 0; i < 10; ++i)
	{
		std::vector<double> values;
		int targetCol = i + 1;

		// 데이터 수집
		for (int row = 0; row < rowCount; ++row) {
			CString strVal = m_listData.GetItemText(row, targetCol);
			values.push_back(_tstof(strVal));
		}

		if (values.empty()) continue;

		// 1. 기초 통계 (Sum, Min, Max)
		double sum = 0;
		double minVal = values[0];
		double maxVal = values[0];

		for (double v : values) {
			sum += v;
			if (v < minVal) minVal = v;
			if (v > maxVal) maxVal = v;
		}
		double mean = sum / values.size();

		// 2. 추가 통계 계산

		// [Range] 범위
		double range = maxVal - minVal;

		// [Std Dev] 표준편차
		double sumSqDiff = 0.0;
		for (double v : values) {
			sumSqDiff += pow(v - mean, 2);
		}
		double stdDev = sqrt(sumSqDiff / values.size());

		// [Median] 중앙값 (정렬 필요)
		std::vector<double> sortedVal = values; // 원본 보존을 위해 복사
		std::sort(sortedVal.begin(), sortedVal.end());
		double median = 0.0;
		size_t n = sortedVal.size();
		if (n % 2 == 0)
			median = (sortedVal[n / 2 - 1] + sortedVal[n / 2]) / 2.0;
		else
			median = sortedVal[n / 2];


		// 3. 리스트에 값 입력 (0~5번 행)
		CString strTemp;

		// 기존
		strTemp.Format(_T("%.2f"), minVal);
		m_listStats.SetItemText(0, targetCol, strTemp);

		strTemp.Format(_T("%.2f"), mean);
		m_listStats.SetItemText(1, targetCol, strTemp);

		strTemp.Format(_T("%.2f"), maxVal);
		m_listStats.SetItemText(2, targetCol, strTemp);

		// [추가]
		strTemp.Format(_T("%.2f"), range);
		m_listStats.SetItemText(3, targetCol, strTemp); // RANGE

		strTemp.Format(_T("%.3f"), stdDev); // 편차는 소수점 3자리 추천
		m_listStats.SetItemText(4, targetCol, strTemp); // STD DEV

		strTemp.Format(_T("%.2f"), median);
		m_listStats.SetItemText(5, targetCol, strTemp); // MEDIAN
	}
}

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
	// 데이터 분포에 따라 유동적으로 범위 설정
	minX = floor(minX) - 1.0;
	maxX = ceil(maxX) + 1.0;

	int binCount = 50; // 네모 개수를 줄여서 가독성 확보
	std::vector<double> bins(binCount, 0.0);
	double binWidth = (maxX - minX) / binCount;

	for (double nv : normData) {
		int idx = (int)((nv - minX) / binWidth);
		if (idx >= 0 && idx < binCount) bins[idx]++;
	}

	double maxDensity = 0;
	for (int i = 0; i < binCount; i++) {
		bins[i] = bins[i] / (data.size() * binWidth);
		if (bins[i] > maxDensity) maxDensity = bins[i];
	}
	if (maxDensity < 0.4) maxDensity = 0.4;

	// --- [1] 히스토그램 그리기 ---
	CBrush histBrush(RGB(100, 180, 100));
	CPen nullPen(PS_NULL, 0, RGB(0, 0, 0));
	dc.SelectObject(&histBrush);
	dc.SelectObject(&nullPen);

	for (int i = 0; i < binCount; i++) {
		if (bins[i] <= 0) continue;
		int x1 = rect.left + (int)((i * binWidth) / (maxX - minX) * rect.Width());
		int x2 = rect.left + (int)(((i + 1) * binWidth) / (maxX - minX) * rect.Width());
		int y = rect.bottom - (int)(bins[i] / maxDensity * rect.Height() * 0.85);
		dc.Rectangle(x1, y, x2, rect.bottom);
	}

	// --- [2] X축 숫자 눈금 그리기 (예: -2, 0, 2, 4...) ---
	dc.SetTextColor(RGB(0, 0, 0));
	for (double val = minX; val <= maxX; val += 1.0) {
		int x = rect.left + (int)((val - minX) / (maxX - minX) * rect.Width());
		CString strVal;
		strVal.Format(_T("%.0f"), val);
		dc.TextOutW(x - 5, rect.bottom + 5, strVal);

		// 작은 눈금선
		dc.MoveTo(x, rect.bottom);
		dc.LineTo(x, rect.bottom + 3);
	}

	// --- [3] Y축 숫자 눈금 그리기 (0.0, 0.2, 0.4...) ---
	for (double d = 0.0; d <= maxDensity; d += 0.1) {
		int y = rect.bottom - (int)(d / maxDensity * rect.Height() * 0.85);
		CString strDensity;
		strDensity.Format(_T("%.1f"), d);
		dc.TextOutW(rect.left - 30, y - 8, strDensity);
	}

	// --- [4] 빨간색 곡선 그리기 ---
	CPen curvePen(PS_DASH, 2, RGB(255, 0, 0));
	dc.SelectObject(&curvePen);
	bool first = true;
	for (double tx = minX; tx <= maxX; tx += 0.1) {
		int x = rect.left + (int)((tx - minX) / (maxX - minX) * rect.Width());
		double ty = NormalPDF(tx);
		int y = rect.bottom - (int)(ty / maxDensity * rect.Height() * 0.85);
		if (first) { dc.MoveTo(x, y); first = false; }
		else { dc.LineTo(x, y); }
	}
}

// 4. OnPaint 함수 완성본
void CEOLParserDlg::OnPaint()
{
	CPaintDC dc(this);
	if (IsIconic()) { /* ... */ }
	else {
		CDialogEx::OnPaint();

		CWnd* pStatic = GetDlgItem(IDC_STATIC_GRAPH);
		if (!pStatic) return;
		CRect graphRect;
		pStatic->GetWindowRect(&graphRect);
		ScreenToClient(&graphRect);

		// 그래프 가독성을 위해 여백(Margin) 확보
		CRect drawRect = graphRect;
		drawRect.DeflateRect(40, 40, 20, 40); // 좌, 상, 우, 하 여백

		dc.FillSolidRect(graphRect, RGB(255, 255, 255));

		// 기본 축 그리기
		CPen axisPen(PS_SOLID, 1, RGB(0, 0, 0));
		dc.SelectObject(&axisPen);
		dc.MoveTo(drawRect.left, drawRect.top);
		dc.LineTo(drawRect.left, drawRect.bottom); // Y축
		dc.LineTo(drawRect.right, drawRect.bottom); // X축

		int selIdx = m_comboModel.GetCurSel();
		if (selIdx == CB_ERR) return;

		std::vector<double> columnData;
		for (int i = 0; i < m_listData.GetItemCount(); i++) {
			columnData.push_back(_tstof(m_listData.GetItemText(i, selIdx + 1)));
		}

		if (!columnData.empty()) {
			DrawNormalDistribution(dc, drawRect, columnData, RGB(255, 0, 0));

			// 제목 및 축 라벨
			dc.SetBkMode(TRANSPARENT);
			CString title;
			m_comboModel.GetLBText(selIdx, title);
			title += _T(" Distribution");
			dc.TextOutW(drawRect.left + 50, drawRect.top - 30, title);

			// 축 이름 표시
			dc.TextOutW(drawRect.CenterPoint().x - 40, drawRect.bottom + 25, _T("Normalized Value"));

			// Y축 이름 (세로 쓰기 대신 상단에 표시)
			dc.TextOutW(drawRect.left - 35, drawRect.top - 20, _T("Density"));
		}
	}
}
void CEOLParserDlg::OnLvnItemchangedListStats(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}
